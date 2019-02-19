// demo pour la fenetre de transcript + menu bar
// il y a une entree de "ligne de commande" en bas

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>

#include "transcript.h"

using namespace std;
#include <string>
#include <vector>
#include <map>

#include <windows.h>
#include "spawn_w.h"

#include "target.h"
#include "mi_parse.h"


#include "gui.h"

/*
void auto_test( glostru * glo )
{
// compter les lignes
GtkTextIter iter; int linum;
gtk_text_buffer_get_end_iter( glo->t.lebuf, &iter );
linum = gtk_text_iter_get_line( &iter );
glo->t.printf("%5d : ", linum );
// l'heure
struct tm *t; time_t it; char sep='-';
time( &it );
t = localtime( &it );
glo->t.printf("%02d%c%02d%c%4d ", t->tm_mday, sep, t->tm_mon+1, sep, t->tm_year+1900 );
glo->t.printf("%02dh%02dmn%02d\n", t->tm_hour, t->tm_min, t->tm_sec );
}
*/

void list_store_resize( GtkListStore * mod, unsigned int size );
unsigned int list_store_cnt( GtkListStore * mod );
void refresh( glostru * glo );

void some_stats( glostru * glo )
{
glo->t.printf("%d (%d) asm lines\n", glo->targ->asmmap.size(), glo->targ->asmstock.size() );
//for	( unsigned int i = 0; i < glo->targ->asmstock.size(); ++i )
//	glo->targ->asmstock[i].dump();
for	( unsigned int i = 0; i < glo->targ->filestock.size(); ++i )
	printf("fichier src : %s %s\n", glo->targ->filestock[i].relpath.c_str(), glo->targ->filestock[i].abspath.c_str() );
unsigned int qlist = glo->targ->liststock[glo->ilist].lines.size();
glo->t.printf("%d listing lines\n", qlist );
glo->t.printf("%d disass model rows\n", list_store_cnt( glo->tmodl ) );
glo->t.printf("%d breakpoints\n", glo->targ->breakpoints.size() );
glo->t.printf("RAM data %u words @ 0x%08X\n", (unsigned int)glo->targ->ramstock[0].w32.size(), (unsigned int)glo->targ->ramstock[0].adr0 );
glo->t.printf("%d RAM model rows\n", list_store_cnt( glo->tmodm ) );
}


// envoyer une commande a GDB, avec echo optionnel dans le transcript (N.B. LF sera ajoute automatiquement)
void send_cmd( glostru * glo, const char * cmd )
{
if	( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog1) ) )
	glo->t.printf("> %s\n", cmd );
glo->dad->send_cmd( cmd );
glo->dad->send_cmd( "\n" );
}

void expa( glostru * glo )
{
refresh( glo );
send_cmd( glo, "-break-delete" );
send_cmd( glo, "-break-list" );
glo->targ->status_set( Breaks );
}


void expb( glostru * glo )
{
glo->targ->status = Ready;
send_cmd( glo, "-break-list" );
glo->targ->status_set( Breaks );
some_stats( glo );
}

// fonction a appeler chaque fois que ip a change
void update_disass( glostru * glo )
{
// notre ip est-il deja desassemble ? cherchons son index dans asmstock
int ip_asm_line = glo->targ->get_ip_asm_line();
if	( ip_asm_line >= 0 )
	{
	// glo->t.printf("line %d in asmstock\n", ip_asm_line );
	// OUI alors est-il dans le listing courant ?
	int ip_in_list = glo->targ->liststock[glo->ilist].search_line( ip_asm_line, glo->ip_in_list );
	if	( ip_in_list < 0 )
		{ // NON refaisons le listing (pour le moment on ne sait que travailler sur 1 seul)
		// N.B. fill_listing ecrase le contenu anterieur
		// ici ce serait interessant de voir si on ne peut pas plutot prolonger le listing
		glo->targ->fill_listing( glo->ilist, glo->targ->get_ip() );
		// on a besoin du compte des lignes pour mettre a jour le tree model
		unsigned int qlist = glo->targ->liststock[glo->ilist].lines.size();
		list_store_resize( glo->tmodl, qlist );
		}
	ip_in_list = glo->targ->liststock[glo->ilist].search_line( ip_asm_line, 0 );
	if	( ip_in_list >= 0 )
		{ // OUI alors le scroll doit marcher
		// glo->t.printf("line %d in list\n", ip_in_list );
		glo->ip_in_list = ip_in_list;
		// scroll sur ip
		GtkTreePath * lepath;
		lepath = gtk_tree_path_new_from_indices( glo->ip_in_list, -1 );
		// char *pipo = gtk_tree_path_to_string( lepath );
		// glo->t.printf( "scroll to %d (%s)\n", glo->ip_in_list, pipo );
		gtk_tree_view_scroll_to_cell( (GtkTreeView *)glo->tlisl,
			lepath,	// GtkTreePath *path,
			NULL, 	// *column,
			TRUE,	// use_align,
			0.5,	// row_align,
			0.0 );	// col_align
		gtk_tree_path_free( lepath );
		}
	}
else	{ // NON allons desassembler si possible
	if	( glo->targ->status != Ready )
		glo->t.printf( "il faut attendre pour desassembler\n");
	else	{
		unsigned long long adr;
		adr = glo->targ->get_ip();
		if	( adr )
			{
			char tbuf[64];
			if	( glo->option_flavor )
				send_cmd( glo, "-gdb-set disassembly-flavor intel");
			else	send_cmd( glo, "-gdb-set disassembly-flavor att");
			snprintf( tbuf, sizeof(tbuf), "-data-disassemble -s 0x%x -e 0x%x -- 5",
							(unsigned int)adr, glo->exp_N + (unsigned int)adr );
			glo->timor = 60; send_cmd( glo, tbuf );
			glo->targ->status_set( Disas );
			}
		}
	}
}

void refresh( glostru *glo )
{
update_disass( glo );
gtk_widget_queue_draw( glo->tlisl );
gtk_widget_queue_draw( glo->tlisr );
unsigned int s = glo->targ->ramstock[0].w32.size();	// hum ce n'est pas l'endroit ou faire ça
if	( s > 0 )
	list_store_resize( glo->tmodm, s );
gtk_widget_queue_draw( glo->tlism );
}

// cette fonction pretend assurer les etapes de demarrage
// on ne doit l'appeler que si glo->targ->status == Init
void init_step( glostru *glo )
{
send_cmd( glo, "-data-list-register-names");
if	( glo->option_child_console )
	send_cmd( glo, "-gdb-set new-console on");
//if	( glo->option_flavor )
//	send_cmd( glo, "-gdb-set disassembly-flavor intel");	// else att
// send_cmd( glo, "-gdb-set mi-async on");	// marche PO sous windows
send_cmd( glo, "-exec-run --start");	// ici GDB met un bk temporaire sur main
send_cmd( glo, "-data-list-register-values x");
glo->targ->status_set( Registers );
}

/** ============================ widget call backs ======================= */

gint close_event_call( GtkWidget *widget,
                        GdkEvent  *event,
                        gpointer   data )
{ return(FALSE); }

void quit_call( GtkWidget *widget, glostru * glo )
{
gtk_widget_destroy( glo->wmain );
}

void cmd_call( GtkWidget *widget, glostru * glo )
{
send_cmd( glo, gtk_entry_get_text( GTK_ENTRY(widget) ) );
gtk_entry_set_text( GTK_ENTRY(widget), "" );
}

int idle_call( glostru * glo )
{
int retval;
char tbuf[1024];
int d;
int tog1 = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog1) );
int tog2 = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog2) );
int tog3 = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog3) );
int tog4 = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog4) );

if	( tog4 )
	return -1;	// i.e. meme le timer est en pause

// la boucle de lecture de la pipe de sortie de GDB
while	( ( d = glo->dad->child_getc() ) >= 0 )
	{
	if	( ( tog2 ) && ( tog3 ) )
		glo->t.printf("%c", d );
	retval = glo->mipa->proc1char( d );
	if	( retval == 0 )				// 0 = un char banal, on continue
		continue;
	else if	( retval < 0 )				// <0 = erreur detectee par parseur de MI
		{
		glo->t.printf("Err %d\n", -retval ); break;
		}
	else	{					// >0 = fin de quelque chose, a extraire seulement si necessaire
		glo->mipa->extract( retval, glo->targ );
		if	( retval == 9 )
			{					// 9 = fin de stream-output
			if	( tog1 )
				{	// dump du prompt ou stream
				if	( glo->mipa->dump( retval, tbuf, sizeof(tbuf) ) )
					glo->t.printf("%s\n", tbuf );
				}
			if	( glo->mipa->nam.c_str()[0] == '(' )		// prompt de GDB
				{
				refresh( glo );
				}
			}
		if	(
			( ( tog2 ) && ( !tog3 ) ) &&
			( ( retval >= 1 ) && ( retval <= 8 ) )
			)
			{	// dump du MI indente
			if	( glo->mipa->dump( retval, tbuf, sizeof(tbuf) ) )
				glo->t.printf("%s\n", tbuf );
			}
		if	( ( !tog2 ) && ( !tog3 ) && ( retval == 8 ) )
			{
			// PROVISOAR : *stopped devrait etre interprete par mi_parse et reporte dans targ->status
			if	( glo->mipa->dump( retval, tbuf, sizeof(tbuf) ) )
				{	// hum! human readable code interpreted by machine !
				if	( strncmp( tbuf, "fin report *stopped", 19 ) == 0 )
					glo->t.printf("stopped : %s\n", glo->targ->reason.c_str() );
				}
			}
		}
	}	// while child_getc()
if	( glo->targ->status != Ready  )
	{
	static GdkColor laranja = { 0, 0xFF00, 0xA000, 0x4000 };
	gtk_widget_modify_base( glo->ecmd, GTK_STATE_NORMAL, &laranja );
	if	( glo->targ->status == Init  )
		init_step( glo );
	}
else	{
	gtk_widget_modify_base( glo->ecmd, GTK_STATE_NORMAL, NULL );
	}
return( -1 );
}

static void action_call( GtkAction *action, glostru * glo )
{
const char * aname;
aname = gtk_action_get_name( action );
int get = 0;
switch	( aname[0] )
	{
	case 'f' :			// file
		switch	( aname[1] )
			{
			case 'c' : glo->t.clear();				break;
			} break;
	case 'r' :			// run
		switch	( aname[1] )
			{
			case 'e' : send_cmd( glo, "-exec-run --start");		break;
			case 'u' : send_cmd( glo, "-exec-continue");		break;
			case 'p' : send_cmd( glo, "-exec-interrupt");		break;
			case 'c' : break;
			} get++; break;
	case 's' :			// steps
		switch	( aname[1] )
			{
			case 'i' : send_cmd( glo, "-exec-step-instruction");	break;
			case 'v' : send_cmd( glo, "-exec-next-instruction");	break;
			case 'o' : send_cmd( glo, "-exec-finish");		break;
			} get++; break;
	case 'b' :			// break
		switch	( aname[1] )
			{
			case 't' : send_cmd( glo, "-break-insert main");	break;
			case 'e' : break;
			} break;
	case 'e' :			// experimental
		switch	( aname[3] )
			{
			case 'a' : expa( glo ); break;
			case 'b' : expb( glo ); break;
			} break;
	default :
		glo->t.printf("action %s\n", aname );
	}
if	( get )
	{
	send_cmd( glo, "-data-list-register-values x");
	glo->targ->status_set( Registers );
	char tbuf[64];
	if	( glo->targ->ramstock[0].adr0 > 0 )
		{
		snprintf( tbuf, sizeof(tbuf), "-data-read-memory-bytes 0x%x %u",
			  (unsigned int)glo->targ->ramstock[0].adr0, 128 );
		send_cmd( glo, tbuf);
		glo->targ->status_set( RAM );
		}
	}
}

void ram_adr_call( GtkWidget *widget, glostru * glo )
{
char tbuf[64];
unsigned long long adr;
adr = strtoull( gtk_entry_get_text( GTK_ENTRY(widget) ), NULL, 16 );	// accepte 0x ou hex brut
adr &= (~(unsigned long long)7);	// alignement autoritaire sur 64 bits
snprintf( tbuf, sizeof(tbuf), "0x%08x", (unsigned int)adr );
gtk_entry_set_text( GTK_ENTRY(widget), tbuf );
if	( adr > 0 )			// un peu foolproof mais pas trop
	{
	snprintf( tbuf, sizeof(tbuf), "-data-read-memory-bytes 0x%x %u", (unsigned int)adr, 128 );
	send_cmd( glo, tbuf);
	glo->targ->status_set( RAM );
	}
}

/** ================= context menus call backs ======================= */

static void disa_call_bk( GtkWidget *widget, glostru * glo )
{
char tbuf[64];
unsigned long long adr = glo->bkadr;
if	( adr )
	{
	if	( glo->targ->is_break( adr ) )
		{
		unsigned int bknum = glo->targ->breakpoints[adr];
		snprintf( tbuf, sizeof(tbuf), "-break-delete %u", bknum );
		}
	else	snprintf( tbuf, sizeof(tbuf), "-break-insert *0x%08x", (unsigned int)adr );
	send_cmd( glo, tbuf );
	}
send_cmd( glo, "-break-list" );
glo->targ->status_set( Breaks );
}

static void disa_call_flavor( GtkWidget *widget, glostru * glo )
{
glo->option_flavor ^= 1;
if	( glo->option_flavor )
	gtk_tree_view_column_set_title( glo->asmcol, "Source Code (Intel flavor)" );
else	gtk_tree_view_column_set_title( glo->asmcol, "Source Code (AT&T flavor)" );
glo->targ->asm_init();
update_disass( glo );
}

// une call back pour le right-clic
gboolean disa_right_call( GtkWidget *curwidg, GdkEventButton *event, glostru * glo )
{
/* single click with the right mouse button? */
if	( ( event->type == GDK_BUTTON_PRESS ) && ( event->button == 3 ) )
	{
	// glo->t.printf("right ");
	// tentative d'identifier la row :
        GtkTreePath *path;
        if	( gtk_tree_view_get_path_at_pos( GTK_TREE_VIEW(curwidg),
						event->x, event->y,
						&path, NULL, NULL, NULL) )
		{
		// on a un path, on en extrait un indice
		unsigned int * indices; int ref;
		int depth;
		indices = (unsigned int *)gtk_tree_path_get_indices_with_depth( path, &depth );
		// ici on pourrait forcer la selection mais bof
		gtk_tree_path_free(path);
		// glo->t.printf( "d=%d, i=%d \n", depth, indices[0] );
		// N.B. code ci-dessous a refactoriser vs disa_data_call()
		// recuperer la reference codee de ligne dans l'objet target
		listing * list = &(glo->targ->liststock[glo->ilist]);
		if	( *indices < list->lines.size() )
			ref = list->lines[*indices];
		else	ref = 0;
		// decoder
		if	( ref < 0 )
			{		// ligne de code source
			glo->bkadr = 0;
			glo->t.printf( "src line %u\n", listing::decode_line_number(ref) );
			gtk_menu_item_set_label( (GtkMenuItem *)glo->itbk, "-");
			}
		else	{		// ligne asm
			unsigned long long adr = glo->targ->asmstock[(unsigned int)ref].adr;
			glo->bkadr = adr;
			// on copie l'adresse en ascii dans le label de l'item !
			char tbuf[32];
			if	( glo->targ->is_break( adr ) )
				snprintf( tbuf, sizeof(tbuf), "kill breakpoint at 0x%08X", (unsigned int)adr );
			else	snprintf( tbuf, sizeof(tbuf), "set breakpoint at 0x%08X", (unsigned int)adr );
			gtk_menu_item_set_label( (GtkMenuItem *)glo->itbk, tbuf);
			}
		gtk_menu_popup( (GtkMenu *)glo->mdisa, NULL, NULL, NULL, NULL, event->button, event->time );
		}
	return TRUE;			/* we handled this */
	}
return FALSE;				/* we did not handle this */
}

/** ================= Gtk Tree View call backs ======================= */

// N.B.
// on peut identifier la treeview avec gtk_tree_view_column_get_tree_view()
// on peut identifier la colonne avec gtk_tree_view_column_get_title() si son nom est unique
static void disa_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
listing * list;
unsigned int i;
int ref;
char text[128];
// recuperer le listing associe
if	( glo->ilist < glo->targ->liststock.size() )
	list = &(glo->targ->liststock[glo->ilist]);
else	{
	g_object_set( rendy, "text", "no data", NULL );
	return;
	}
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
// recuperer la reference codee de ligne dans l'objet target
if	( i < list->lines.size() )
	ref = list->lines[i];
else	ref = 0;
// decoder
if	( ref < 0 )
	{		// ligne de code source
	unsigned int ilin = listing::decode_line_number(ref);
	unsigned int ifil = listing::decode_file_index(ref);
	if	( tree_column == glo->adrcol )
		{
		snprintf( text, sizeof(text), "  %d", ilin );
		g_object_set( rendy, "text", text, NULL );
		}
	else if	( tree_column == glo->asmcol )
		{
		g_object_set( rendy, "text", glo->targ->get_src_line( ifil, ilin ), NULL );
		}
	}
else	{		// ligne asm
	asmline * daline = &(glo->targ->asmstock[(unsigned int)ref]);
	if	( tree_column == glo->adrcol )
		{
		unsigned long long adr = daline->adr;
		if	( adr == glo->targ->get_ip() )
			{
			if	( glo->targ->is_break( adr ) )
				snprintf( text, sizeof(text), MARGIN_BKIP "%08X", (unsigned int)adr );
			else	snprintf( text, sizeof(text), MARGIN_IP "%08X",   (unsigned int)adr );
			}
		else	{
			if	( glo->targ->is_break( adr ) )
				snprintf( text, sizeof(text), MARGIN_BK "%08X",   (unsigned int)adr );
			else	snprintf( text, sizeof(text), MARGIN_NONE "%08X", (unsigned int)adr );
			}
		g_object_set( rendy, "markup", text, NULL );
		}
	else if	( tree_column == glo->asmcol )
		{
		g_object_set( rendy, "text", daline->asmsrc.c_str(), NULL );
		}
	}
}

static void regname_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
unsigned int i;
const char * text;
const char * bgcolor;
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
// recuperer la donnee dans l'objet target
if	( i < glo->targ->regs.regs.size() )
	text = glo->targ->regs.regs[i].name.c_str();
else	text = "?";
g_object_set( rendy, "text", text, NULL );
if	( i == glo->targ->regs.iip ) bgcolor = IP_COLOR;
else	bgcolor = "#DDDDDD";
g_object_set( rendy, "cell-background", bgcolor, "cell-background-set", TRUE, NULL );
}

static void regval_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
unsigned int i;
int chng = 0;
char text[64];
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
// elaborer la donnee
#define MARKUP_REG	// 2 styles disponibles pour highlight du reg qui a change
if	( i < glo->targ->regs.regs.size() )
	{
	chng = glo->targ->regs.regs[i].changed;
	#ifdef MARKUP_REG
	snprintf( text, sizeof(text), "<span background=\"%s\">%08X</span>",
		  chng?"#88DDFF":"#FFFFFF", (unsigned int)glo->targ->regs.regs[i].val );
	#else
	snprintf( text, sizeof(text), "%08X", (unsigned int)glo->targ->regs.regs[i].val );
	#endif
	}
else	snprintf( text, sizeof(text), "?" );
#ifdef MARKUP_REG
g_object_set( rendy, "markup", text, NULL );
#else
g_object_set( rendy, "text", text, NULL );
// la couleur ne revient pas toute seule au defaut (blanc)
g_object_set( rendy, "cell-background", chng?"#88DDFF":"#FFFFFF", "cell-background-set", TRUE, NULL );
#endif
}

static void ram_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
unsigned int i;
char text[128];
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
if	( tree_column == glo->madrcol )
	{
	unsigned long long adr = glo->targ->ramstock[0].adr0;
	adr += i * 4;
	snprintf( text, sizeof(text), "%08X", (unsigned int)adr );
	g_object_set( rendy, "text", text, NULL );
	}
else if	( tree_column == glo->mdatcol )
	{
	if	( i < glo->targ->ramstock[0].w32.size() )
		snprintf( text, sizeof(text), "%08X", glo->targ->ramstock[0].w32[i] );
	else	snprintf( text, sizeof(text), "no data" );
	g_object_set( rendy, "text", text, NULL );
	}
}

/** ============================ menus std ======================= */

// tableau d'actions initialise
static GtkActionEntry ui_entries[] = {
  // name,    stock id,  label
  { "FileMenu", NULL,	"_File" },
  { "RunMenu", NULL, 	"_Run" },
  { "BreakMenu", NULL, 	"_Breakpoints" },
  // name,  stock id,   label, accel, tooltip, callback
  { "fopn", 		NULL, "Open",			"<control>O",	NULL, G_CALLBACK(action_call) },
  { "fclr", 		NULL, "Clear Console",		"<control>K",	NULL, G_CALLBACK(action_call) },
  { "quit", 		NULL, "Quit",			"<alt>F4", 	NULL, G_CALLBACK(action_call) },
  { "res", 		NULL, "Restart",		"<shift>F5",	NULL, G_CALLBACK(action_call) },
  { "run", 		NULL, "Run/Continue",		"F5",		NULL, G_CALLBACK(action_call) },
  { "rpa", 		NULL, "Pause",  		"<control>F5",	NULL, G_CALLBACK(action_call) },
  { "si", 		NULL, "Step In",  		"F11",		NULL, G_CALLBACK(action_call) },
							// windows bug here : F10 intercepted
  { "sv", 		NULL, "Step Over",  		"<shift>F11",	NULL, G_CALLBACK(action_call) },
  { "so", 		NULL, "Step Out",		"<control>F11",	NULL, G_CALLBACK(action_call) },
  { "rcu",	 	NULL, "Run to Cursor",		"<alt>F5",	NULL, G_CALLBACK(action_call) },
  { "btog", 		NULL, "Toggle Breakpoint",	"F9",		NULL, G_CALLBACK(action_call) },
  { "bena",	 	NULL, "Enable/Disable Break",	"<control>F9",	NULL, G_CALLBACK(action_call) },
  { "expa",	 	NULL, "Exp A",			"<control>A",	NULL, G_CALLBACK(action_call) },
  { "expb",	 	NULL, "Exp B",			"<control>B",	NULL, G_CALLBACK(action_call) },
};

// menu descriptions
static const gchar * ui_xml =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='FileMenu'>"
"      <menuitem action='fopn'/>"
"      <menuitem action='fclr'/>"
"      <menuitem action='quit'/>"
"    </menu>"
"    <menu action='RunMenu'>"
"      <menuitem action='res'/>"
"      <menuitem action='run'/>"
"      <menuitem action='rpa'/>"
"      <menuitem action='si'/>"
"      <menuitem action='sv'/>"
"      <menuitem action='so'/>"
"      <menuitem action='rcu'/>"
"    </menu>"
"    <menu action='BreakMenu'>"
"      <menuitem action='btog'/>"
"      <menuitem action='bena'/>"
"      <menuitem action='expa'/>"
"      <menuitem action='expb'/>"
"    </menu>"
"  </menubar>"
"</ui>";

// Create the menubar
GtkWidget * mk_bars( glostru * glo )
{
GtkActionGroup *action_group;
GtkUIManager *manui;
GtkWidget * labar;

// on cree un GtkActionGroup qui va contenir la liste d'actions avec leurs bindkeys et callbacks
// l'argument glo est ici pour etre repasse aux callbacks
action_group = gtk_action_group_new( "NoName" );
gtk_action_group_add_actions( action_group, ui_entries, G_N_ELEMENTS(ui_entries), glo );

// on cree un GtkUIManager dont on pourra extraire les bondkeys et le widget menubar
manui = gtk_ui_manager_new();
// g_object_set_data_full( G_OBJECT(glo->wmain), "pipo-ui-manager", manui, g_object_unref );
gtk_ui_manager_insert_action_group( manui, action_group, 0 );

// on en extrait le bondkeys qu'on attribue a la fenetre principale
gtk_window_add_accel_group( GTK_WINDOW(glo->wmain), gtk_ui_manager_get_accel_group( manui )  );

// on parse le XML pour creer les menus (et toolbars le cas echeant)
gtk_ui_manager_add_ui_from_string( manui, ui_xml, -1, NULL );

labar = gtk_ui_manager_get_widget( manui, "/MenuBar" );
return labar;
}

/** ============================ make widgets ======================= */

void list_store_resize( GtkListStore * mod, unsigned int size )
{	// modele minimal, 1 colonne de type int
GtkTreeIter curiter;
gtk_list_store_clear( mod );
for ( unsigned int i = 0; i < size; i++ )
    {
    gtk_list_store_append( mod, &curiter );
    gtk_list_store_set( mod, &curiter, 0, i, -1 );
    }
}

unsigned int list_store_cnt( GtkListStore * mod )
{
GtkTreeIter curiter;
GtkTreePath * curpath;
int * indices;
int depth;
gtk_list_store_append( mod, &curiter );
curpath = gtk_tree_model_get_path( (GtkTreeModel *)mod, &curiter );
gtk_list_store_remove ( mod, &curiter );
indices = gtk_tree_path_get_indices_with_depth( curpath, &depth );
gtk_tree_path_free( curpath );
if	( depth == 1 )	return indices[0];
else			return -depth;
}

// create the disassembly view context menu
GtkWidget * mk_disa_menu( glostru * glo )
{
GtkWidget * curmenu;
GtkWidget * curitem;

curmenu = gtk_menu_new ();    // Don't need to show menus, use gtk_menu_popup
// gtk_menu_popup( (GtkMenu *)menu1_x, NULL, NULL, NULL, NULL, event->button, event->time );


curitem = gtk_menu_item_new_with_label("Breakpoint");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( disa_call_bk ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
glo->itbk = curitem;
gtk_widget_show ( curitem );

curitem = gtk_separator_menu_item_new();
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("Change disassembly flavor");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( disa_call_flavor ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

return curmenu;
}

// Create the disassembly view
GtkWidget * mk_disa_view( glostru * glo )
{
GtkWidget *curwidg;
GtkCellRenderer *renderer;
GtkTreeViewColumn *curcol;
GtkTreeSelection* cursel;

// le modele : minimal, 1 colonne de type int
glo->tmodl = gtk_list_store_new( 1, G_TYPE_INT );
list_store_resize( glo->tmodl, 1 );

// la vue
curwidg = gtk_tree_view_new();

// la colonne adresse, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, "Address" );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         (GtkTreeCellDataFunc)disa_data_call,
                                         (gpointer)glo, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );
glo->adrcol = curcol;

// la colonne valeur, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         (GtkTreeCellDataFunc)disa_data_call,
                                         (gpointer)glo, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );
glo->asmcol = curcol;
if	( glo->option_flavor )
	gtk_tree_view_column_set_title( glo->asmcol, "Source Code (Intel flavor)" );
else	gtk_tree_view_column_set_title( glo->asmcol, "Source Code (AT&T flavor)" );

// configurer la selection
cursel = gtk_tree_view_get_selection( (GtkTreeView*)curwidg );
gtk_tree_selection_set_mode( cursel, GTK_SELECTION_NONE );

// connecter modele
gtk_tree_view_set_model( (GtkTreeView*)curwidg, GTK_TREE_MODEL( glo->tmodl ) );

// Change default font throughout the widget
PangoFontDescription * font_desc;
font_desc = pango_font_description_from_string("Monospace 10");
gtk_widget_modify_font( curwidg, font_desc );
pango_font_description_free( font_desc );

// connecter callback pour right-clic (c'est la callback qui va identifier le right)
g_signal_connect( curwidg, "button-press-event", (GCallback)disa_right_call, (gpointer)glo );

return(curwidg);
}

// Create the register view
GtkWidget * mk_reg_view( glostru * glo )
{
GtkWidget *curwidg;
GtkCellRenderer *renderer;
GtkTreeViewColumn *curcol;
GtkTreeSelection* cursel;

// le modele : minimal, 1 colonne de type int
glo->tmodr = gtk_list_store_new( 1, G_TYPE_INT );
list_store_resize( glo->tmodr, 9 );

// la vue
curwidg = gtk_tree_view_new();

// la colonne nom de registre, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, " Reg " );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         (GtkTreeCellDataFunc)regname_data_call,
                                         (gpointer)glo, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );

// la colonne valeur, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, " Value " );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         (GtkTreeCellDataFunc)regval_data_call,
                                         (gpointer)glo, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );

// configurer la selection
cursel = gtk_tree_view_get_selection( (GtkTreeView*)curwidg );
gtk_tree_selection_set_mode( cursel, GTK_SELECTION_NONE );

// connecter modele
gtk_tree_view_set_model( (GtkTreeView*)curwidg, GTK_TREE_MODEL( glo->tmodr ) );

return(curwidg);
}

// Create the memory view
GtkWidget * mk_ram_view( glostru * glo )
{
GtkWidget *curwidg;
GtkCellRenderer *renderer;
GtkTreeViewColumn *curcol;
GtkTreeSelection* cursel;

// le modele : minimal, 1 colonne de type int
glo->tmodm = gtk_list_store_new( 1, G_TYPE_INT );
list_store_resize( glo->tmodm, 1 );

// la vue
curwidg = gtk_tree_view_new();

// la colonne adresse, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, "Address" );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         (GtkTreeCellDataFunc)ram_data_call,
                                         (gpointer)glo, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );
glo->madrcol = curcol;

// la colonne valeur, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, "Data" );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         (GtkTreeCellDataFunc)ram_data_call,
                                         (gpointer)glo, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );
glo->mdatcol = curcol;

// configurer la selection
cursel = gtk_tree_view_get_selection( (GtkTreeView*)curwidg );
gtk_tree_selection_set_mode( cursel, GTK_SELECTION_NONE );

// connecter modele
gtk_tree_view_set_model( (GtkTreeView*)curwidg, GTK_TREE_MODEL( glo->tmodm ) );

// Change default font throughout the widget
PangoFontDescription * font_desc;
font_desc = pango_font_description_from_string("Monospace 10");
gtk_widget_modify_font( curwidg, font_desc );
pango_font_description_free( font_desc );

// connecter callback pour right-clic (c'est la callback qui va identifier le right)
// g_signal_connect( curwidg, "button-press-event", (GCallback)ram_right_call, (gpointer)glo );

return(curwidg);
}

/** =================== THE MAIN ======================= */

// the big storage of everything
static glostru theglo;
static daddy ledad;
static mi_parse lemipa;
static target latarget;

int main( int argc, char *argv[] )
{
GtkWidget * curwidg;
#define glo (&theglo)

glo->dad = &ledad;
glo->mipa = &lemipa;
glo->targ = &latarget;
glo->timor = 0;
glo->ilist = 0;

// option de CLI
glo->option_child_console = 1;
glo->option_flavor = 0;
glo->exp_N = 512;
if	( argc > 2 )
	{
	if	( argv[2][0] == '-' )
		glo->option_child_console = 0;
	else	glo->exp_N = atoi( argv[2] );
	}
if	( glo->exp_N == 0 )
	glo->exp_N = 16;

setlocale( LC_ALL, "C" );	// question de survie

gtk_init(&argc,&argv);

// principale fenetre
curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( close_event_call ), glo );
gtk_signal_connect( GTK_OBJECT(curwidg), "destroy",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );

gtk_window_set_title( GTK_WINDOW(curwidg), "EasyGDB" );

gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 2 );
glo->wmain = curwidg;

// boite verticale
curwidg = gtk_vbox_new( FALSE, 2 );
gtk_container_add( GTK_CONTAINER( glo->wmain ), curwidg );
glo->vmain = curwidg;

// la barre de menu
curwidg = mk_bars( glo );
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->mbar = curwidg;

// paire verticale "paned"
curwidg = gtk_vpaned_new ();
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );
// gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 5 );	// le tour exterieur
glo->vpan = curwidg;

// paire horizontale "paned" dans la moitie superieure de la paned verticale
// avec 2 notebooks
curwidg = gtk_hpaned_new ();
gtk_paned_pack1( GTK_PANED(glo->vpan), curwidg, TRUE, FALSE );
glo->hpan = curwidg;

curwidg = gtk_notebook_new();
gtk_paned_pack1 (GTK_PANED (glo->hpan), curwidg, FALSE, FALSE );
glo->notl = curwidg;

curwidg = gtk_notebook_new();
gtk_paned_pack2 (GTK_PANED (glo->hpan), curwidg, TRUE, FALSE );
glo->notr = curwidg;

// notebook de gauche : registres
curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( curwidg), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
gtk_notebook_append_page( GTK_NOTEBOOK( glo->notl ), curwidg, gtk_label_new("Registers") );
gtk_widget_set_size_request (curwidg, 110, 260);
glo->scwr = curwidg;

curwidg = mk_reg_view( glo );
gtk_container_add( GTK_CONTAINER( glo->scwr ), curwidg );
glo->tlisr = curwidg;

curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( curwidg), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
gtk_notebook_append_page( GTK_NOTEBOOK( glo->notl ), curwidg, gtk_label_new("Thing") );
gtk_widget_set_size_request (curwidg, 60, 100);
glo->scw2 = curwidg;

// notebook de droite : disassembly + RAM
// tab de disassembly
curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( curwidg), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
gtk_notebook_append_page( GTK_NOTEBOOK( glo->notr ), curwidg, gtk_label_new("Disassembly") );
gtk_widget_set_size_request (curwidg, 200, 100);
glo->scwl = curwidg;

glo->mdisa = mk_disa_menu( glo );
curwidg = mk_disa_view( glo );
gtk_container_add( GTK_CONTAINER( glo->scwl ), curwidg );
glo->tlisl = curwidg;

// tab de RAM
curwidg = gtk_vbox_new( FALSE, 2 );
gtk_notebook_append_page( GTK_NOTEBOOK( glo->notr ), curwidg, gtk_label_new("Memory") );
glo->vram = curwidg;

curwidg = gtk_entry_new();
gtk_signal_connect( GTK_OBJECT( curwidg ), "activate",
                    GTK_SIGNAL_FUNC( ram_adr_call ), (gpointer)glo );
gtk_entry_set_editable( GTK_ENTRY(curwidg), TRUE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "" );
gtk_box_pack_start( GTK_BOX( glo->vram ), curwidg, FALSE, FALSE, 0 );
glo->eram = curwidg;

curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( curwidg), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
gtk_box_pack_start( GTK_BOX( glo->vram ), curwidg, TRUE, TRUE, 0 );
gtk_widget_set_size_request (curwidg, 200, 100);
glo->scwm = curwidg;

curwidg = mk_ram_view( glo );
gtk_container_add( GTK_CONTAINER( glo->scwm ), curwidg );
glo->tlism = curwidg;




// Le transcript dans la moitie inferieure de la paned verticale
curwidg = glo->t.create();
gtk_paned_pack2( GTK_PANED(glo->vpan), curwidg, TRUE, FALSE );
gtk_widget_set_size_request( curwidg, 700, 200 );
// gtk_widget_set_usize( curwidg, 700, 200 );
glo->wtran = curwidg;

// boite horizontale
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 2);
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->hbut = curwidg;

/* entree editable */
curwidg = gtk_entry_new();
gtk_signal_connect( GTK_OBJECT( curwidg ), "activate",
                    GTK_SIGNAL_FUNC( cmd_call ), (gpointer)glo );
gtk_entry_set_editable( GTK_ENTRY(curwidg), TRUE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "" );
// gtk_widget_set_usize( curwidg, 250, 0 );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0);
glo->ecmd = curwidg;

// check bouton
curwidg = gtk_check_button_new_with_label ("cmd dump");
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( curwidg ), FALSE );
glo->btog1 = curwidg;

// check bouton
curwidg = gtk_check_button_new_with_label ("MI dump");
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( curwidg ), FALSE );
glo->btog2 = curwidg;

// check bouton
curwidg = gtk_check_button_new_with_label ("raw opt");
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( curwidg ), FALSE );
glo->btog3 = curwidg;

// check bouton
curwidg = gtk_check_button_new_with_label ("idle pause");
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( curwidg ), FALSE );
glo->btog4 = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Quit ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( quit_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->bqui = curwidg;

gtk_widget_show_all( glo->wmain );

gtk_timeout_add( 31, (GtkFunction)(idle_call), (gpointer)glo );

if	( argc > 1 )
	{
	char tbuf[128]; int retval;
	snprintf( tbuf, sizeof(tbuf), "gdb --interpreter=mi %s", argv[1] );

	retval = glo->dad->start_child( tbuf );
	if	( retval )
	glo->t.printf("Error daddy %d\n", retval );
	}
else	return 1;

gtk_main();

return 0;
}
