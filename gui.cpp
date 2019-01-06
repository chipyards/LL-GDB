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

void expa( glostru * glo )
{
unsigned long long adr;
adr = glo->targ->regs.get_rip()->val;
if	( adr )
	{
	char tbuf[64];
	snprintf( tbuf, sizeof(tbuf), "-data-disassemble -s 0x%x -e 0x%x -- 5\n",
					(unsigned int)adr, 128 + (unsigned int)adr );
	glo->dad->send_cmd( tbuf );
	}
}

void expb( glostru * glo )
{
glo->t.printf("voir stdout\n");
printf("%d (%d) asm lines\n", glo->targ->asmmap.size(), glo->targ->asmstock.size() );
for	( unsigned int i = 0; i < glo->targ->asmstock.size(); ++i )
	glo->targ->asmstock[i].dump();
for	( unsigned int i = 0; i < glo->targ->filestock.size(); ++i )
	printf("%s %s\n", glo->targ->filestock[i].relpath.c_str(), glo->targ->filestock[i].abspath.c_str() );

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
glo->t.printf("> %s\n", gtk_entry_get_text( GTK_ENTRY(widget) ) );
glo->dad->send_cmd( gtk_entry_get_text( GTK_ENTRY(widget) ) );
glo->dad->send_cmd( "\n" );
gtk_entry_set_text( GTK_ENTRY(widget), "" );
}

int idle_call( glostru * glo )
{
int retval;
char tbuf[1024];
int d;
int tog = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog) );

while	( ( d = glo->dad->child_getc() ) >= 0 )
	{
	retval = glo->mipa->proc1char( d );
	if	( retval == 0 )
		continue;
	else if	( retval < 0 )
		{
		glo->t.printf("Err %d\n", -retval );
		}
	else	{
		if	( ( tog ) || ( retval == 9 ) )
			{
			if	( glo->mipa->dump( retval, tbuf, sizeof(tbuf) ) )
				glo->t.printf("%s\n", tbuf );
			}
		if	( ( retval == 9 ) && ( glo->mipa->nam.c_str()[0] == '(' ) )
			{			// ici petit report de debug
			if	( glo->targ->regs.regs.size() >= 9 )
				{
				registro * r;
				r = glo->targ->regs.get_rsp();
				glo->t.printf( "%c %s = %08x\n", (r->changed)?':':' ',
						r->name.c_str(), (unsigned int)r->val );
				r = glo->targ->regs.get_rip();
				glo->t.printf( "%c %s = %08x\n", (r->changed)?':':' ',
						r->name.c_str(), (unsigned int)r->val );
				gtk_widget_queue_draw( glo->tlisr );
				}
			}
		glo->mipa->extract( retval, glo->targ );
		}
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
			case 'c' : glo->t.clear();					break;
			} break;
	case 'r' :			// run
		switch	( aname[1] )
			{
			case 'e' : glo->dad->send_cmd("-gdb-set new-console on\n");
				   glo->dad->send_cmd("-data-list-register-names\n");
				   glo->dad->send_cmd("-exec-run --start\n");
				   break;
			case 'u' : glo->dad->send_cmd("-exec-continue\n");		break;
			case 'p' : glo->dad->send_cmd("-exec-interrupt\n");		break;
			case 'c' : break;
			} get++; break;
	case 's' :			// steps
		switch	( aname[1] )
			{
			case 'i' : glo->dad->send_cmd("-exec-step-instruction\n");	break;
			case 'v' : glo->dad->send_cmd("-exec-next-instruction\n");	break;
			case 'o' : glo->dad->send_cmd("-exec-finish\n");		break;
			} get++; break;
	case 'b' :			// break
		switch	( aname[1] )
			{
			case 't' : glo->dad->send_cmd("-break-insert main\n"); break;
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
	glo->dad->send_cmd("-data-list-register-values x\n");
	}
}

void regname_data_call( GtkTreeViewColumn * tree_column,	// sert pas !
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
unsigned int i;
const char *text;
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
if	( i < glo->targ->regs.regs.size() )
	{
	text = glo->targ->regs.regs[i].name.c_str();
	}
else	text = "?";
g_object_set( rendy, "text", text, NULL );
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

// Create the register view
GtkWidget * mk_reg_view( glostru * glo )
{
GtkWidget *curwidg;
GtkCellRenderer *renderer;
GtkTreeViewColumn *curcol;
GtkTreeSelection* cursel;
GtkTreeIter curiter;

// le modele : minimal, 1 colonne de type int
glo->tmodr = gtk_list_store_new( 1, G_TYPE_INT );
for ( int i = 0; i < 9; i++ )
    {
    gtk_list_store_append( glo->tmodr, &curiter );
    gtk_list_store_set( glo->tmodr, &curiter, 0, i, -1 );
    }

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

setlocale( LC_ALL, "C" );	// question de survie

gtk_init(&argc,&argv);

// principale fenetre
curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( close_event_call ), glo );
gtk_signal_connect( GTK_OBJECT(curwidg), "destroy",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );

gtk_window_set_title( GTK_WINDOW(curwidg), "JLNs Transzkrypt" );

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
glo->scw3 = curwidg;

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
curwidg = gtk_check_button_new_with_label ("full dump");
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( curwidg ), FALSE );
glo->btog = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Quit ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( quit_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->bqui = curwidg;

gtk_widget_show_all( glo->wmain );

gtk_timeout_add( 100, (GtkFunction)(idle_call), (gpointer)glo );

if	( argc > 1 )
	{
	char tbuf[128]; int retval;
	snprintf( tbuf, sizeof(tbuf), "gdb --interpreter=mi %s", argv[1] );

	retval = glo->dad->start_child( tbuf );
	if	( retval )
	glo->t.printf("Error daddy %d\n", retval );
	}
else	return 1;

// tentative de demarrage auto
glo->dad->send_cmd("-gdb-set new-console on\n");
glo->dad->send_cmd("-data-list-register-names\n");
glo->dad->send_cmd("-exec-run --start\n");

gtk_main();

return 0;
}
