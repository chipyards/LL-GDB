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

#include "mi_parse.h"

#include <windows.h>
#include "spawn_w.h"

#include "gui.h"

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
if	( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog) ) )
	auto_test( glo );

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
		if	( glo->mipa->dump( retval, tbuf, sizeof(tbuf) ) )
			glo->t.printf("%s\n", tbuf );
		}
	}
return( -1 );
}

static void action_call( GtkAction *action, glostru * glo )
{
const char * aname;
aname = gtk_action_get_name( action );
switch	( aname[0] )
	{
	case 'r' :			// run
		switch	( aname[1] )
			{
			case 'e' : glo->dad->send_cmd("-gdb-set new-console on\n");
				   glo->dad->send_cmd("-exec-run --start\n");		break;
			case 'u' : glo->dad->send_cmd("-exec-continue\n");		break;
			case 'p' : glo->dad->send_cmd("-exec-interrupt\n");		break;
			case 'c' : break;
			} break;
	case 's' :			// steps
		switch	( aname[1] )
			{
			case 'i' : glo->dad->send_cmd("-exec-step-instruction\n");	break;
			case 'v' : glo->dad->send_cmd("-exec-next-instruction\n");	break;
			case 'o' : glo->dad->send_cmd("-exec-finish\n");		break;
			} break;
	case 'b' :			// break
		switch	( aname[1] )
			{
			case 't' : glo->dad->send_cmd("-break-insert main\n"); break;
			case 'e' : break;
			} break;
	default :
		glo->t.printf("action %s\n", aname );
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
  { "open", 		NULL, "Open",			"<control>O",	NULL, G_CALLBACK(action_call) },
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
};

// menu descriptions
static const gchar * ui_xml =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='FileMenu'>"
"      <menuitem action='open'/>"
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

// =================== THE MAIN =======================

int main( int argc, char *argv[] )
{
GtkWidget * curwidg;
glostru theglo;
#define glo (&theglo)

daddy ledad;
mi_parse lemipa;

glo->dad = &ledad;
glo->mipa = &lemipa;

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
gtk_widget_set_size_request (curwidg, 60, 100);
glo->scw1 = curwidg;

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
curwidg = gtk_check_button_new_with_label ("auto-test");
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

gtk_main();

return 0;
}
