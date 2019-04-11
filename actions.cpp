#include <gtk/gtk.h>

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

#include "arch_type.h"
#include "gui.h"
#include "actions.h"

static void action_call( GtkAction *action, glostru * glo );

// tableau d'actions  : c'est la que tout commence
// associe opcode, texte, bindkey, callback
// N.B.	- on peut utiliser une simple lettre comme bindkey,
// 	  mais alors cette lettre ne peut plus etre utilisee pour remplir une entry!!!!
//	- callback unique (sauf exception) c'est notre choix
static GtkActionEntry ui_entries[] = {
  // name,		stock id,  label
  { "FileMenu",		NULL, "_File" },
  { "RunMenu",		NULL, "_Run" },
  { "ExpMenu",		NULL, "_Experiment" },
  // name,		stock id,  label,		accel,		tooltip, callback
//{ "fopn", 		NULL, "Open",			"<control>O",	NULL, G_CALLBACK(action_call) },
  { "fclr", 		NULL, "Clear Console",		"<control>K",	NULL, G_CALLBACK(action_call) },
  { "quit", 		NULL, "Quit",			"<alt>F4", 	NULL, G_CALLBACK(gtk_main_quit) },
  { "res", 		NULL, "Restart",		"<shift>F5",	NULL, G_CALLBACK(action_call) },
  { "run", 		NULL, "Run/Continue",		"F5",		NULL, G_CALLBACK(action_call) },
  { "si", 		NULL, "Step In",  		"F11",		NULL, G_CALLBACK(action_call) },
			// windows bug here -->		F10 intercepted
  { "sv", 		NULL, "Step Over",  		"<shift>F11",	NULL, G_CALLBACK(action_call) },
  { "so", 		NULL, "Step Out",		"<control>F11",	NULL, G_CALLBACK(action_call) },
  { "bmain", 		NULL, "Breakpoint to main",	"F9",		NULL, G_CALLBACK(action_call) },
  { "bentry",	 	NULL, "Breakpoint to entry",	"<control>F9",	NULL, G_CALLBACK(action_call) },
  { "expa",	 	NULL, "Exp A",			"<control>A",	NULL, G_CALLBACK(action_call) },
  { "expb",	 	NULL, "Exp B",			"<control>B",	NULL, G_CALLBACK(action_call) },
  { "expd",	 	NULL, "Exp D",			"<control>D",	NULL, G_CALLBACK(action_call) },
};

static GtkUIManager *manui;

// la callback principale
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
			case 'm' : queue_cmd( glo, "-break-insert main", BreakSetKill );	break;
			case 'e' : queue_cmd( glo, "-break-insert entry", BreakSetKill );	break;
			} break;
	case 'e' :			// experimental
		switch	( aname[3] )
			{
			case 'a' : expa( glo ); break;
			case 'b' : expb( glo ); break;
			case 'd' : expd( glo ); break;
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

// Create the actions
void mk_actions( glostru * glo )
{
GtkActionGroup *action_group;

// on cree un GtkActionGroup qui va contenir la liste d'actions avec leurs bindkeys et callbacks
// l'argument glo est ici pour etre repasse aux callbacks
action_group = gtk_action_group_new( "NoName" );
gtk_action_group_add_actions( action_group, ui_entries, G_N_ELEMENTS(ui_entries), glo );

// on cree un GtkUIManager dont on pourra extraire les bindkeys et le widget menubar
manui = gtk_ui_manager_new();
gtk_ui_manager_insert_action_group( manui, action_group, 0 );

// on en extrait les bindkeys qu'on attribue a la fenetre principale
gtk_window_add_accel_group( GTK_WINDOW(glo->wmain), gtk_ui_manager_get_accel_group( manui )  );
}

/** ============================ menus de la main window ======================= */

// menu descriptions : une chaine contenant du xml
static const gchar * ui_xml =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='FileMenu'>"
//"      <menuitem action='fopn'/>"
"      <menuitem action='fclr'/>"
"      <menuitem action='quit'/>"
"    </menu>"
"    <menu action='RunMenu'>"
"      <menuitem action='res'/>"
"      <menuitem action='run'/>"
"      <menuitem action='si'/>"
"      <menuitem action='sv'/>"
"      <menuitem action='so'/>"
"    </menu>"
"    <menu action='ExpMenu'>"
"      <menuitem action='bmain'/>"
"      <menuitem action='bentry'/>"
"      <menuitem action='expa'/>"
"      <menuitem action='expb'/>"
"      <menuitem action='expd'/>"
"    </menu>"
"  </menubar>"
"</ui>";

// Create the menubar
GtkWidget * mk_mbar( glostru * glo )
{
GtkWidget * labar;

// on parse le XML pour creer les menus (et toolbars le cas echeant)
gtk_ui_manager_add_ui_from_string( manui, ui_xml, -1, NULL );

labar = gtk_ui_manager_get_widget( manui, "/MenuBar" );
return labar;
}

