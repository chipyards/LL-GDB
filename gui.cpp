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
}


void expb( glostru * glo )
{
send_cmd( glo, "-thread-info");
}

void update_disass( glostru * glo )
{
// notre ip est-il deja desassemble ? cherchons son index dans asmstock
int ip_asm_line = glo->targ->get_ip_asm_line();
if	( ip_asm_line >= 0 )
	{
	// glo->t.printf("line %d in asmstock\n", ip_asm_line );
	// est-il dans le listing courant ?
	int ip_in_list = glo->targ->liststock[glo->ilist].search_line( ip_asm_line, glo->ip_in_list );
	if	( ip_in_list >= 0 )
		{
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
	else	{
		glo->t.printf("ici il faudrait completer le listing\n");
		}
	}
else	{
	glo->t.printf( "ici il faudrait completer le desassemblage\n");
	}
}

void refresh( glostru *glo )
{
update_disass( glo );
gtk_widget_queue_draw( glo->tlisl );
gtk_widget_queue_draw( glo->tlisr );
}

// cette fonction pretend assurer les etapes de demarrage...en les testant a l'envers
// on ne doit l'appeler que si glo->timor est nul
void init_step( glostru *glo )
{
if	( glo->targ->liststock[0].lines.size() < 1 )
	{			// listing vide ! a-t-on du disass ?
	if	( glo->targ->asmmap.size() < 2 )
		{		// pas de desass ! a-t-on une valeur plausible de ip ?
		if	( (unsigned int)glo->targ->get_ip() == 0 )
			{	// pas de ip ! a-t-on des noms de registres ?
			if	( glo->targ->regs.regs.size() < 9 )
				{ glo->timor = 60; send_cmd( glo, "-data-list-register-names"); }
			else	{	// on a des noms de registres, lancer le prog pour avoir ip
				glo->timor = 180;
				if	( glo->option_child_console )
					send_cmd( glo, "-gdb-set new-console on");
				// send_cmd( glo, "-gdb-set mi-async on");	// marche PO sous windows
				send_cmd( glo, "-exec-run --start");	// ici GDB met un bk sur main
				send_cmd( glo, "-data-list-register-values x");
				}
			}
		else	{	// on a un ip pour desassembler, alors on y va
			unsigned long long adr;
			adr = glo->targ->get_ip();
			if	( adr )
				{
				char tbuf[64];
				snprintf( tbuf, sizeof(tbuf), "-data-disassemble -s 0x%x -e 0x%x -- 5",
								(unsigned int)adr, glo->exp_N + (unsigned int)adr );
				glo->timor = 60; send_cmd( glo, tbuf );
				}
			}
		}
	else	{		// on a du desass, faisons un listing
		glo->t.printf("%d (%d) asm lines\n", glo->targ->asmmap.size(), glo->targ->asmstock.size() );
		//for	( unsigned int i = 0; i < glo->targ->asmstock.size(); ++i )
		//	glo->targ->asmstock[i].dump();
		for	( unsigned int i = 0; i < glo->targ->filestock.size(); ++i )
			printf("fichier src : %s %s\n", glo->targ->filestock[i].relpath.c_str(), glo->targ->filestock[i].abspath.c_str() );
		glo->ilist = 0;		// pour le moment on ne sait que travailler sur 1 seul listing...
					// glo->ilist = glo->targ->add_listing( glo->targ->get_ip() );
		glo->targ->fill_listing( glo->ilist, glo->targ->get_ip() );
		unsigned int qlist = glo->targ->liststock[glo->ilist].lines.size();
		glo->t.printf("%d listing lines\n", qlist );
		// glo->t.printf("dump: voir stdout\n");
		// glo->targ->dump_listing( glo->ilist );
		if	( qlist > 0 )
			{
			list_store_resize( glo->tmodl, qlist );
			glo->t.printf("%d tree model rows\n", list_store_cnt( glo->tmodl ) );
			init_step( glo );		// tres audacieuse recursion UWAGA !!!
			}
		else	{
			// notre listing est encore vide, c'est ennuyeux on va boucler sur le disass..
			// cependant notre list_store contien 1 ligne, et la datafunc saura afficher genre invalid
			}
		}
	}
else	{
	// ben c'est bon on fait plus rien
	}
}

/** ============================ call backs ======================= */

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
	else	{					// >0 = fin de quelque chose, a extraire pour peupler dans l'objet target
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
		}
	}
if	( glo->timor )
	{
	static GdkColor laranja = { 0, 0xFF00, 0xA000, 0x4000 };
	gtk_widget_modify_base( glo->ecmd, GTK_STATE_NORMAL, &laranja );
	--glo->timor;
	}
else	{
	init_step( glo );
	gtk_widget_modify_base( glo->ecmd, GTK_STATE_NORMAL, NULL );
	refresh( glo );
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
	glo->targ->regs.reset_reg_changes();
	send_cmd( glo, "-data-list-register-values x");
	}
}

/** ================= Gtk Tree View call backs ======================= */

// N.B.
// on peut identifier la treeview avec gtk_tree_view_column_get_tree_view()
// on peut identifier la colonne avec gtk_tree_view_column_get_title ()

void disa_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
listing * list;
const char *col;
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
// identifier la colonne
col = gtk_tree_view_column_get_title( tree_column );
// recuperer la reference codee de ligne dans l'objet target
if	( i < list->lines.size() )
	ref = list->lines[i];
else	ref = 0;
// decoder
if	( ref < 0 )
	{		// ligne de code source
	unsigned int ilin = listing::decode_line_number(ref);
	unsigned int ifil = listing::decode_file_index(ref);
	if	( col[0] == 'A' )
		{
		snprintf( text, sizeof(text), "  %d", ilin );
		g_object_set( rendy, "text", text, NULL );
		}
	else if	( col[0] == 'C' )
		{
		g_object_set( rendy, "text", glo->targ->get_src_line( ifil, ilin ), NULL );
		}
	}
else	{		// ligne asm
	asmline * daline = &(glo->targ->asmstock[(unsigned int)ref]);
	if	( col[0] == 'A' )
		{
		unsigned long long adr = daline->adr;
		if	( adr == glo->targ->get_ip() )
			{
			snprintf( text, sizeof(text), "<span background=\"" IP_COLOR "\">%08X</span>",
			(unsigned int)adr );
			g_object_set( rendy, "markup", text, NULL );
			// glo->ip_in_list = i;  // ne marche que si ip est dans la fenetre :-((
			}
		else	{
			snprintf( text, sizeof(text), "%08X", (unsigned int)adr );
			g_object_set( rendy, "text", text, NULL );
			}
		}
	else if	( col[0] == 'C' )
		{
		g_object_set( rendy, "text", daline->asmsrc.c_str(), NULL );
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
else	bgcolor = "#DDDDDD";
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

/** ============================ widgets ======================= */

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

// Create a listing view
GtkWidget * mk_list_view( glostru * glo )
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

// la colonne nom de registre, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, "Address" );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         (GtkTreeCellDataFunc)disa_data_call,
                                         (gpointer)glo, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );

// la colonne valeur, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, "Code" );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         (GtkTreeCellDataFunc)disa_data_call,
                                         (gpointer)glo, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );

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


// =================== THE MAIN =======================

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
curwidg = gtk_hpaned_new ();
gtk_paned_pack1( GTK_PANED(glo->vpan), curwidg, TRUE, FALSE );
glo->hpan = curwidg;

curwidg = gtk_notebook_new();
gtk_paned_pack1 (GTK_PANED (glo->hpan), curwidg, FALSE, FALSE );
glo->notl = curwidg;

curwidg = gtk_notebook_new();
gtk_paned_pack2 (GTK_PANED (glo->hpan), curwidg, TRUE, FALSE );
glo->notr = curwidg;

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
gtk_notebook_append_page( GTK_NOTEBOOK( glo->notl ), curwidg, gtk_label_new("Stack") );
gtk_widget_set_size_request (curwidg, 60, 100);
glo->scw2 = curwidg;

curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( curwidg), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
gtk_notebook_append_page( GTK_NOTEBOOK( glo->notr ), curwidg, gtk_label_new("Disassembly") );
gtk_widget_set_size_request (curwidg, 200, 100);
glo->scwl = curwidg;

curwidg = mk_list_view( glo );
gtk_container_add( GTK_CONTAINER( glo->scwl ), curwidg );
glo->tlisl = curwidg;

curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( curwidg), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
gtk_notebook_append_page( GTK_NOTEBOOK( glo->notr ), curwidg, gtk_label_new("Memory") );
gtk_widget_set_size_request (curwidg, 200, 100);
glo->scw4 = curwidg;

// scrolled window pour le transcript dans la moitie inferieure de la paned verticale
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

// option de CLI
glo->option_child_console = 1;
glo->exp_N = 512;
if	( argc > 2 )
	{
	if	( argv[2][0] == '-' )
		glo->option_child_console = 0;
	else	glo->exp_N = atoi( argv[2] );
	}
if	( glo->exp_N == 0 )
	glo->exp_N = 16;

gtk_main();

return 0;
}
