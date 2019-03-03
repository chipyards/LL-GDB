// here below is the main(), mother of everything

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>


using namespace std;
#include <string>
#include <vector>
#include <map>

#include <windows.h>

#include "modpop2.h"
#include "transcript.h"
#include "spawn_w.h"
#include "target.h"
#include "mi_parse.h"

#include "gui.h"

/** ============================ list store utilities ======================= */

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

/** ============================ action functions ======================= */

void refresh( glostru * glo );
void init_step( glostru * glo );

void some_stats( glostru * glo )
{
glo->t.printf("%d (%d) asm lines\n", glo->targ->asmmap.size(), glo->targ->asmstock.size() );
// for	( unsigned int i = 0; i < glo->targ->asmstock.size(); ++i )
//	glo->targ->asmstock[i].dump();
for	( unsigned int i = 0; i < glo->targ->filestock.size(); ++i )
	glo->t.printf("fichier src : %s %s\n", glo->targ->filestock[i].relpath.c_str(), glo->targ->filestock[i].abspath.c_str() );
unsigned int qlist = glo->targ->liststock[glo->ilist].lines.size();
glo->t.printf("%d listing lines\n", qlist );
glo->t.printf("%d disass model rows\n", list_store_cnt( glo->tmodl ) );
glo->t.printf("%d breakpoints\n", glo->targ->breakpoints.size() );
glo->t.printf("RAM data %u words @ 0x%08X\n", (unsigned int)glo->targ->ramstock[0].w32.size(), (unsigned int)glo->targ->ramstock[0].adr0 );
glo->t.printf("%d RAM model rows\n", list_store_cnt( glo->tmodm ) );
glo->t.printf("job_status=%010llX\n", glo->targ->job_status );
glo->targ->job_dump();
glo->t.printf("iip=%d, isp=%d, ibp=%d\n", glo->targ->regs.iip, glo->targ->regs.isp, glo->targ->regs.ibp );
}


// envoyer une commande a GDB, avec echo optionnel dans le transcript (N.B. LF sera ajoute automatiquement)
void send_cmd( glostru * glo, const char * cmd )
{
//if	( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog1) ) )
//	glo->t.printf("> %s\n", cmd );
//glo->dad->send_cmd( cmd );
//glo->dad->send_cmd( "\n" );

}

// mettre dans la queue une commande pour GDB, avec echo optionnel dans le transcript (N.B. LF sera ajoute automatiquement)
void queue_cmd( glostru * glo, const char * cmd, job_enum job  )
{
if	( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog1) ) )
	glo->t.printf(">q %s\n", cmd );
glo->targ->job_queue_cmd( cmd, job );
}

// demarrer le prochain job de la queue, s'il existe et si c'est possible
void next_run( glostru * glo )
{
if	( glo->targ->job_isanyrunning() )
	return;
if	( glo->targ->job_isanyerror() )
	{
	if	(
		( glo->targ->reason == string("exited-normally") ) ||
		( glo->targ->reason == string("exited") )
		)
		{
		glo->t.printf(": %s\n", glo->targ->reason.c_str() );
		modpop( "Info", "program exited normally", GTK_WINDOW(glo->wmain) );
		}
	else	{
		glo->t.printf("E %s\n", glo->targ->error_msg.c_str() );
		modpop( "Error", glo->targ->error_msg.c_str(), GTK_WINDOW(glo->wmain) );
		}
	glo->targ->job_status &= (~(ERROR_MASK));
	return;
	}
if	( glo->targ->job_isanyqueued() == 0 )
	return;
int ijob = glo->targ->job_nextqueued();
if	( ijob < 0 )
	return;
const char * cmd = glo->targ->job_cmd[ijob].c_str();
glo->targ->job_set_running( (job_enum)ijob );
if	( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog1) ) )
	glo->t.printf(">r %s\n", cmd );
glo->dad->send_cmd( cmd );
glo->dad->send_cmd( "\n" );
}

void expa( glostru * glo )
{
refresh( glo );
glo->targ->job_status &= (~RUNNING_MASK);	// PROVIZOAR
}


void expb( glostru * glo )
{
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
	unsigned long long adr;
	adr = glo->targ->get_ip();
	if	( ( adr ) && ( !glo->targ->job_is_queued(Disass) ) )
		{
		char tbuf[128];	// this fmt var below is to avoid bogus MinGW warnings about %llX
		const char * fmt = "-data-disassemble -s 0x" OPT_FMT " -e 0x" OPT_FMT " -- 5";
		snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)adr, glo->exp_N + (opt_type)adr );
		queue_cmd( glo, tbuf, Disass );
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
	list_store_resize( glo->tmodm, ((glo->ram_format==64)?(s/2):(s)) );
gtk_widget_queue_draw( glo->scwl );
gtk_widget_queue_draw( glo->wmain );
}

// cette fonction pretend assurer les etapes de demarrage
void init_step( glostru *glo )
{
queue_cmd( glo, "-data-list-register-names", RegNames );
if	( glo->option_child_console )
	queue_cmd( glo, "-gdb-set new-console on", GDBSet );
// send_cmd( glo, "-gdb-set mi-async on");	// marche PO sous windows
queue_cmd( glo, "-exec-run --start", Run );	// ici GDB met un bk temporaire sur main
queue_cmd( glo, "-data-list-register-values x", RegVal );
glo->targ->job_dump();
}

/** ============================ widget call backs ======================= */

void cmd_call( GtkWidget *widget, glostru * glo )
{
const char * cmd;
cmd = gtk_entry_get_text( GTK_ENTRY(widget) );
if	( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog1) ) )
	glo->t.printf("> %s\n", cmd );
glo->dad->send_cmd( cmd );
glo->dad->send_cmd( "\n" );
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
		modpop( "erreur", "erreur MI parser", GTK_WINDOW(glo->wmain) );
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
				{
				glo->t.printf("%s\n", tbuf );
				}
			}
		if	( ( !tog2 ) && ( !tog3 ) && ( retval == 8 ) )
			{
			// PROVISOAR : *stopped devrait etre interprete par mi_parse et reporte dans targ->job_status
			if	( glo->mipa->dump( retval, tbuf, sizeof(tbuf) ) )
				{	// hum! human readable code interpreted by machine !
				if	( strncmp( tbuf, "fin report *stopped", 19 ) == 0 )
					glo->t.printf("stopped : %s\n", glo->targ->reason.c_str() );
				}
			}
		}
	}	// while child_getc()
if	( glo->targ->job_isanyrunning()  )
	{
	static GdkColor laranja = { 0, 0xFF00, 0xA000, 0x4000 };
	gtk_widget_modify_base( glo->ecmd, GTK_STATE_NORMAL, &laranja );
	}
else	{
	gtk_widget_modify_base( glo->ecmd, GTK_STATE_NORMAL, NULL );
	}
next_run( glo );
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
			case 'e' : queue_cmd( glo, "-exec-run --start", Run );		break;
			case 'u' : queue_cmd( glo, "-exec-continue", Continue );		break;
			// case 'p' : queue_cmd( glo, "-exec-interrupt");		break;
			case 'c' : break;
			} get++; break;
	case 's' :			// steps
		switch	( aname[1] )
			{
			case 'i' : queue_cmd( glo, "-exec-step-instruction", Continue );	break;
			case 'v' : queue_cmd( glo, "-exec-next-instruction", Continue );	break;
			case 'o' : queue_cmd( glo, "-exec-finish", Continue );			break;
			} get++; break;
	case 'b' :			// break
		switch	( aname[1] )
			{
			case 't' : queue_cmd( glo, "-break-insert main", BreakSetKill );	break;
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
	queue_cmd( glo, "-data-list-register-values x", RegVal );
	if	( glo->targ->ramstock[0].adr0 > 0 )
		{
		char tbuf[128]; const char * fmt = "-data-read-memory-bytes 0x" OPT_FMT " %u";
		snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)glo->targ->ramstock[0].adr0, glo->option_ramblock );
		queue_cmd( glo, tbuf, RAMRead );
		}
	}
}

void ram_adr_call( GtkWidget *widget, glostru * glo )
{
char tbuf[128]; const char * fmt;
unsigned long long adr;
adr = strtoull( gtk_entry_get_text( GTK_ENTRY(widget) ), NULL, 16 );	// accepte 0x ou hex brut
adr &= (~(unsigned long long)7);	// alignement autoritaire sur 64 bits
fmt = "0x" OPT_FMT;
snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)adr );
gtk_entry_set_text( GTK_ENTRY(widget), tbuf );
if	( adr > 0 )			// un peu foolproof mais pas trop
	{
	fmt = "-data-read-memory-bytes 0x" OPT_FMT " %u";
	snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)adr, glo->option_ramblock );
	queue_cmd( glo, tbuf, RAMRead );
	}
}

/** ================= context menus call backs ======================= */

void disa_call_bk( GtkWidget *widget, glostru * glo )
{
char tbuf[128];
unsigned long long adr = glo->bkadr;
if	( adr )
	{
	if	( glo->targ->is_break( adr ) )
		{
		unsigned int bknum = glo->targ->breakpoints[adr];
		snprintf( tbuf, sizeof(tbuf), "-break-delete %u", bknum );
		}
	else	{
		const char * fmt = "-break-insert *0x" OPT_FMT;
		snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)adr );
		}
	queue_cmd( glo, tbuf, BreakSetKill );
	}
queue_cmd( glo, "-break-list", BreakList );
}

void disa_call_bk_all( GtkWidget *widget, glostru * glo )
{
queue_cmd( glo, "-break-delete", BreakSetKill );
queue_cmd( glo, "-break-list", BreakList );
}

void disa_call_flavor( GtkWidget *widget, glostru * glo )
{
glo->option_flavor ^= 1;
if	( glo->option_flavor )
	{
	gtk_tree_view_column_set_title( glo->asmcol, "Source Code (Intel flavor)" );
	queue_cmd( glo, "-gdb-set disassembly-flavor intel", GDBSet );
	}
else	{
	gtk_tree_view_column_set_title( glo->asmcol, "Source Code (AT&T flavor)" );
	queue_cmd( glo, "-gdb-set disassembly-flavor att", GDBSet );
	}
glo->targ->asm_init();	// effacer tout le disassembly
update_disass( glo );
}

void disa_call_binvis( GtkWidget *widget, glostru * glo )
{
glo->targ->option_binvis ^= 1;
if	( glo->targ->option_binvis )
	{
	gtk_tree_view_column_set_visible( glo->bincol, TRUE );
	}
else	{
	gtk_tree_view_column_set_visible( glo->bincol, FALSE );
	}
glo->targ->asm_init();	// effacer tout le disassembly
update_disass( glo );
}

// une call back pour le right-clic --> context menu
gboolean disa_right_call( GtkWidget *curwidg, GdkEventButton *event, glostru * glo )
{
/* single click with the right mouse button? */
if	( ( event->type == GDK_BUTTON_PRESS ) && ( event->button == 3 ) )
	{
	// glo->t.printf("right ");
	// tentative d'identifier la row :
        GtkTreePath *path;
        if	( gtk_tree_view_get_path_at_pos( GTK_TREE_VIEW(curwidg), event->x, event->y, &path, NULL, NULL, NULL) )
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
			char tbuf[128]; const char * fmt;
			if	( glo->targ->is_break( adr ) )
				fmt = "kill breakpoint at 0x" OPT_FMT;
			else	fmt = "set breakpoint at 0x" OPT_FMT;
			snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)adr );
			gtk_menu_item_set_label( (GtkMenuItem *)glo->itbk, tbuf);
			}
		gtk_menu_popup( (GtkMenu *)glo->mdisa, NULL, NULL, NULL, NULL, event->button, event->time );
		}
	return TRUE;			/* we handled this */
	}
return FALSE;				/* we did not handle this */
}

void ram_call_fmt( GtkWidget *widget, glostru * glo )
{
if	( widget == glo->itram8 )
	glo->ram_format = 8;
else if	( widget == glo->itram16 )
	glo->ram_format = 16;
else if	( widget == glo->itram32 )
	glo->ram_format = 32;
else if	( widget == glo->itram64 )
	glo->ram_format = 64;
refresh( glo );
}

// une call back pour le right-clic --> context menu
gboolean ram_right_call( GtkWidget *curwidg, GdkEventButton *event, glostru * glo )
{
/* single click with the right mouse button? */
if	( ( event->type == GDK_BUTTON_PRESS ) && ( event->button == 3 ) )
	{
	gtk_menu_popup( (GtkMenu *)glo->mram, NULL, NULL, NULL, NULL, event->button, event->time );
	return TRUE;			/* we handled this */
	}
return FALSE;				/* we did not handle this */
}

/** ================= Gtk Tree View call backs ======================= */

// N.B.
// on peut identifier la treeview avec gtk_tree_view_column_get_tree_view()
// on peut identifier la colonne avec gtk_tree_view_column_get_title() si son nom est unique
void disa_data_call( GtkTreeViewColumn * tree_column,
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
		// snprintf( text, sizeof(text), "  %4d    ", ilin );
		snprintf( text, sizeof(text), "  %d", ilin );
		g_object_set( rendy, "text", text, NULL );
		}
	else if	( tree_column == glo->asmcol )
		{
		g_object_set( rendy, "text", glo->targ->get_src_line( ifil, ilin ), NULL );
		}
	else if	( tree_column == glo->bincol )
		{
		g_object_set( rendy, "text", "", NULL );
		}
	}
else	{		// ligne asm
	asmline * daline = &(glo->targ->asmstock[(unsigned int)ref]);
	if	( tree_column == glo->adrcol )
		{
		unsigned long long adr = daline->adr;
		const char * fmt;
		if	( adr == glo->targ->get_ip() )
			{
			if	( glo->targ->is_break( adr ) )
				fmt = MARGIN_BKIP OPT_FMT;
			else	fmt = MARGIN_IP OPT_FMT;
			}
		else	{
			if	( glo->targ->is_break( adr ) )
				fmt = MARGIN_BK OPT_FMT;
			else	fmt = MARGIN_NONE OPT_FMT;
			}
		snprintf( text, sizeof(text), fmt, (opt_type)adr );
		g_object_set( rendy, "markup", text, NULL );
		}
	else if	( tree_column == glo->asmcol )
		{
		g_object_set( rendy, "text", daline->asmsrc.c_str(), NULL );
		}
	else if	( tree_column == glo->bincol )
		{
		for	( unsigned int ib = 0; ib < daline->qbytes; ++ib )
			snprintf( text + (ib*3), sizeof(text) - (ib*3), "%02X ", daline->bytes[ib] );
		g_object_set( rendy, "text", text, NULL );
		}
	}
}

void regname_data_call( GtkTreeViewColumn * tree_column,
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
else if	( i == glo->targ->regs.isp ) bgcolor = SP_COLOR;
else if	( i == glo->targ->regs.ibp ) bgcolor = BP_COLOR;
else	bgcolor = "#E4E4E4";
g_object_set( rendy, "cell-background", bgcolor, "cell-background-set", TRUE, NULL );
}

void regval_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
unsigned int i;
int chng = 0;
char text[64]; const char * fmt;
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
// elaborer la donnee
#define MARKUP_REG	// 2 styles disponibles pour highlight du reg qui a change
if	( i < glo->targ->regs.regs.size() )
	{
	chng = glo->targ->regs.regs[i].changed;
	#ifdef MARKUP_REG
	fmt = "<span background=\"%s\">" OPT_FMT "</span>";
	snprintf( text, sizeof(text), fmt, chng?CHREG_COLOR:"#FFFFFF", (opt_type)glo->targ->regs.regs[i].val );
	#else
	fmt = OPT_FMT;
	snprintf( text, sizeof(text), fmt, (unsigned int)glo->targ->regs.regs[i].val );
	#endif
	}
else	snprintf( text, sizeof(text), "?" );
#ifdef MARKUP_REG
g_object_set( rendy, "markup", text, NULL );
#else
g_object_set( rendy, "text", text, NULL );
// la couleur ne revient pas toute seule au defaut (blanc)
g_object_set( rendy, "cell-background", chng?CHREG_COLOR:"#FFFFFF", "cell-background-set", TRUE, NULL );
#endif
}

void ram_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
unsigned int i;
char text[128]; const char * fmt;
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
if	( tree_column == glo->madrcol )
	{
	unsigned long long adr = glo->targ->ramstock[0].adr0;
	adr += i * ((glo->ram_format==64)?(8):(4));
	if	( adr == glo->targ->get_sp() )
		fmt = MARGIN_SP OPT_FMT;
	else if	( adr == glo->targ->get_bp() )
		fmt = MARGIN_BP OPT_FMT;
	else	fmt = MARGIN_NOSP OPT_FMT;
	snprintf( text, sizeof(text), fmt, (opt_type)adr );
	g_object_set( rendy, "markup", text, NULL );
	}
else if	( tree_column == glo->mdatcol )
	{
	switch	( glo->ram_format )
		{
		case 64 :
			if	( (1+(2*i)) < glo->targ->ramstock[0].w32.size() )
				{
				fmt = "%08X%08X";
				snprintf( text, sizeof(text), fmt,
					glo->targ->ramstock[0].w32[1+(2*i)], glo->targ->ramstock[0].w32[2*i]  );
				}
			else	snprintf( text, sizeof(text), "no data" );
			break;
		case 8 :
			if	( i < glo->targ->ramstock[0].w32.size() )
				{
				unsigned char * bytes = (unsigned char *)&(glo->targ->ramstock[0].w32[i]);
				fmt = "%02X %02X %02X %02X";
				snprintf( text, sizeof(text), fmt, bytes[0], bytes[1], bytes[2], bytes[3] );
				}
			else	snprintf( text, sizeof(text), "no data" );
			break;
		case 16 :
			if	( i < glo->targ->ramstock[0].w32.size() )
				{
				unsigned short * shorts = (unsigned short *)&(glo->targ->ramstock[0].w32[i]);
				fmt = "%04X %04X";
				snprintf( text, sizeof(text), fmt, shorts[0], shorts[1] );
				}
			else	snprintf( text, sizeof(text), "no data" );
			break;
		case 32 :
		default:
			if	( i < glo->targ->ramstock[0].w32.size() )
				{
				fmt = "%08X";
				snprintf( text, sizeof(text), fmt, glo->targ->ramstock[0].w32[i] );
				}
			else	snprintf( text, sizeof(text), "no data" );
			break;
		}
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
  { "expa",	 	NULL, "Exp A",			"A",		NULL, G_CALLBACK(action_call) },
  { "expb",	 	NULL, "Exp B",			"B",		NULL, G_CALLBACK(action_call) },
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

// on en extrait les bindkeys qu'on attribue a la fenetre principale
gtk_window_add_accel_group( GTK_WINDOW(glo->wmain), gtk_ui_manager_get_accel_group( manui )  );

// on parse le XML pour creer les menus (et toolbars le cas echeant)
gtk_ui_manager_add_ui_from_string( manui, ui_xml, -1, NULL );

labar = gtk_ui_manager_get_widget( manui, "/MenuBar" );
return labar;
}

/** =================== THE MAIN ======================= */

// the big storage of everything
static glostru theglo;
static daddy ledad;
static mi_parse lemipa;
static target latarget;
// pour exception gasp() de modpop2.c
// ceci cree bien le storage de la variable et l'exporte
// pour un programme C (a savoir modpop2.c) !!
extern "C" { GtkWindow * global_main_window; };

int main( int argc, char *argv[] )
{
#define glo (&theglo)

glo->dad = &ledad;
glo->mipa = &lemipa;
glo->targ = &latarget;
glo->ilist = 0;

// option de CLI
glo->option_child_console = 1;
glo->option_flavor = 0;
glo->targ->option_binvis = 0;
#ifdef PRINT_64
glo->targ->regs.option_qregs = 18;
#else
glo->targ->regs.option_qregs = 10;
#endif
glo->option_ramblock = 128;
glo->option_toggles = 1;

glo->exp_N = 256;
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

// main window layout
mk_the_gui( glo );
global_main_window = GTK_WINDOW(glo->wmain);

if	( glo->option_toggles & 1 )
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( glo->btog1 ), TRUE );
if	( glo->option_toggles & 2 )
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( glo->btog2 ), TRUE );
if	( glo->option_toggles & 4 )
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( glo->btog3 ), TRUE );
if	( glo->option_toggles & 8 )
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( glo->btog4 ), TRUE );

gtk_timeout_add( 31, (GtkFunction)(idle_call), (gpointer)glo );

if	( argc > 1 )
	{
	char tbuf[128]; int retval;
	snprintf( tbuf, sizeof(tbuf), "gdb --interpreter=mi %s", argv[1] );

	retval = glo->dad->start_child( tbuf );
	if	( retval )
		glo->t.printf("Error daddy %d\n", retval );
	else	init_step( glo );
	}
else	glo->t.printf("need an executable file name");

// modpop( "test", "before gtk_main", GTK_WINDOW(glo->wmain) );

gtk_main();

return 0;
}
