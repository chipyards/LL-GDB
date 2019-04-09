// demo pour la fenetre de transcript + menu bar
// il y a une entree de "ligne de commande" en bas

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdarg.h>


using namespace std;
#include <string>
#include <vector>
#include <map>

#include <windows.h>

#include "transcript.h"
#include "spawn_w.h"
#include "target.h"
#include "mi_parse.h"

#include "gui.h"
#include "actions.h"

/** ============================ make context menus ======================= */

// create the register view context menu
GtkWidget * mk_reg_menu( glostru * glo )
{
GtkWidget * curmenu;
GtkWidget * curitem;

curmenu = gtk_menu_new ();    // Don't need to show menus, use gtk_menu_popup
// gtk_menu_popup( (GtkMenu *)menu1_x, NULL, NULL, NULL, NULL, event->button, event->time );

curitem = gtk_menu_item_new_with_label("Copy");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( reg_call_copy ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
glo->itrg = curitem;
gtk_widget_show ( curitem );

return curmenu;
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

curitem = gtk_menu_item_new_with_label("Kill All Breakpoints");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( disa_call_bk_all ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_separator_menu_item_new();
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("Change disassembly flavor");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( disa_call_flavor ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("Show/Hide Executable Code");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( disa_call_binvis ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("Open in text editor");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( disa_call_editor ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("Copy Addr");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( disa_call_copy_adr ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("Copy Code");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( disa_call_copy_code ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

return curmenu;
}

// create the memory view context menu
GtkWidget * mk_ram_menu( glostru * glo )
{
GtkWidget * curmenu;
GtkWidget * curitem;
GSList * group = NULL;

curmenu = gtk_menu_new ();    // Don't need to show menus, use gtk_menu_popup
// gtk_menu_popup( (GtkMenu *)menu1_x, NULL, NULL, NULL, NULL, event->button, event->time );

curitem = gtk_radio_menu_item_new_with_label( group, "Bytes (8-bit words)");
group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(curitem) );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( ram_call_fmt ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
glo->itram8 = curitem;
gtk_widget_show ( curitem );

curitem = gtk_radio_menu_item_new_with_label( group, "Words (16-bit words)");
group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(curitem) );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( ram_call_fmt ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
glo->itram16 = curitem;
gtk_widget_show ( curitem );

curitem = gtk_radio_menu_item_new_with_label( group, "DWords (32-bit words)");
group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(curitem) );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( ram_call_fmt ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
glo->itram32 = curitem;
gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(curitem), TRUE );	// defaut
glo->ram_format = 32;							// defaut
gtk_widget_show ( curitem );

curitem = gtk_radio_menu_item_new_with_label( group, "QWords (64-bit words)");
group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(curitem) );
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( ram_call_fmt ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
glo->itram64 = curitem;
gtk_widget_show ( curitem );

curitem = gtk_radio_menu_item_new_with_label( group, "ASCII");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( ram_call_fmt ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
glo->itram7 = curitem;
gtk_widget_show ( curitem );

curitem = gtk_menu_item_new_with_label("Copy Line");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( ram_call_copy ), (gpointer)glo );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

return curmenu;
}
/** ============================ make the subwindows ======================= */

// Create the register view
GtkWidget * mk_reg_view( glostru * glo )
{
GtkWidget *curwidg;
GtkCellRenderer *renderer;
GtkTreeViewColumn *curcol;
GtkTreeSelection* cursel;

// le modele : minimal, 1 colonne de type int
glo->tmodr = gtk_list_store_new( 1, G_TYPE_INT );
list_store_resize( glo->tmodr, glo->targ->regs.option_qregs );

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

gtk_tree_view_column_set_title( curcol, " Contents " );
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

// connecter callback pour right-clic (c'est la callback qui va identifier le right)
g_signal_connect( curwidg, "button-press-event", (GCallback)reg_right_call, (gpointer)glo );

return(curwidg);
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

// la colonne binaire, avec data_func
renderer = gtk_cell_renderer_text_new();
curcol = gtk_tree_view_column_new();

gtk_tree_view_column_set_title( curcol, "Executable Code" );
gtk_tree_view_column_pack_start( curcol, renderer, TRUE );
gtk_tree_view_column_set_cell_data_func( curcol, renderer,
                                         (GtkTreeCellDataFunc)disa_data_call,
                                         (gpointer)glo, NULL );
gtk_tree_view_column_set_resizable( curcol, TRUE );
gtk_tree_view_column_set_visible( curcol, FALSE );
gtk_tree_view_append_column( (GtkTreeView*)curwidg, curcol );
glo->bincol = curcol;

// la colonne source text, avec data_func
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
g_signal_connect( curwidg, "button-press-event", (GCallback)ram_right_call, (gpointer)glo );

return(curwidg);
}

/** ============================ make the editor window ======================= */

static gint editor_delete_call( GtkWidget *widget, GdkEvent *event, gpointer data )
{
gtk_widget_hide( widget );
return (TRUE);
}

void mk_editor( glostru * glo )
{
GtkWidget *curwidg;

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );/* DIALOG est deprecated, POPUP est autre chose */
/* ATTENTION c'est serieux : modal veut dire que la fenetre devient la
   seule a capturer les evenements ( i.e. les autres sont bloquees ) */
gtk_window_set_modal( GTK_WINDOW(curwidg), FALSE );
// gtk_window_set_type_hint( GTK_WINDOW(curwidg), GDK_WINDOW_TYPE_HINT_DIALOG );

g_signal_connect( curwidg, "delete_event",
                  G_CALLBACK( editor_delete_call ), NULL );

gtk_window_set_title( GTK_WINDOW(curwidg), "Editor" );
glo->wedi = curwidg;

/* creer boite verticale */
curwidg = gtk_vbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( glo->wedi ), curwidg );
glo->vedi = curwidg;

// widget texte
curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(curwidg), GTK_SHADOW_IN );
gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW(curwidg),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS );
gtk_box_pack_start( GTK_BOX( glo->vedi ), curwidg, TRUE, TRUE, 0);
gtk_widget_set_usize( curwidg, 700, 100 );
glo->sedi = curwidg;

curwidg = gtk_text_view_new();
glo->bedi = gtk_text_view_get_buffer( GTK_TEXT_VIEW(curwidg) );
gtk_container_add( GTK_CONTAINER(glo->sedi), curwidg );

// Change default font throughout the widget
PangoFontDescription * font_desc;
font_desc = pango_font_description_from_string("Monospace 10");
gtk_widget_modify_font( curwidg, font_desc );
pango_font_description_free( font_desc );

// Change left margin throughout the widget
gtk_text_view_set_left_margin( GTK_TEXT_VIEW( curwidg ), 10 );

glo->tedi = curwidg;

gtk_widget_show_all( glo->wedi );
}


/** ============================ make the GUI ======================= */

gint close_event_call( GtkWidget *widget,
                        GdkEvent  *event,
                        gpointer   data )
{ gtk_main_quit(); return TRUE; }

void quit_call( GtkWidget *widget, glostru * glo )
{ gtk_main_quit(); }

void mk_the_gui( glostru * glo )
{
GtkWidget *curwidg;

// principale fenetre
curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );

// pour garder le controle de la situation (bouton X du bandeau de la fenetre)
g_signal_connect( curwidg, "delete_event",
		  G_CALLBACK( close_event_call ), glo );
// juste pour le cas ou on aurait un gasp() qui destroy abruptment la fenetre principale
g_signal_connect( curwidg, "destroy",
		  G_CALLBACK( gtk_main_quit ), NULL );

gtk_window_set_title( GTK_WINDOW(curwidg), "LL-GDB" );

gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 2 );
glo->wmain = curwidg;

// boite verticale
curwidg = gtk_vbox_new( FALSE, 2 );
gtk_container_add( GTK_CONTAINER( glo->wmain ), curwidg );
glo->vmain = curwidg;

// les actions (bindkeys)
mk_actions( glo );
// la barre de menu
curwidg = mk_mbar( glo );
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->mbar = curwidg;

// paire verticale "paned"
curwidg = gtk_vpaned_new ();
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );
// gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 5 );	// le tour exterieur
glo->vpan = curwidg;

// paire horizontale "paned" dans la moitie superieure de la paned verticale
// avec 1 notebook et une seconde paire
curwidg = gtk_hpaned_new ();
gtk_paned_pack1( GTK_PANED(glo->vpan), curwidg, TRUE, FALSE );
glo->hpan = curwidg;

curwidg = gtk_notebook_new();
gtk_paned_pack1 (GTK_PANED (glo->hpan), curwidg, FALSE, FALSE );
glo->notl = curwidg;

curwidg = gtk_hpaned_new ();
gtk_paned_pack2 (GTK_PANED (glo->hpan), curwidg, TRUE, FALSE );
gtk_widget_set_size_request (curwidg, 740, 100);
glo->hpan2 = curwidg;

// notebook de gauche : registres
curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( curwidg), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
gtk_notebook_append_page( GTK_NOTEBOOK( glo->notl ), curwidg, gtk_label_new("Registers") );
#ifdef PRINT_64
gtk_widget_set_size_request (curwidg, 150, 450);
#else
gtk_widget_set_size_request (curwidg, 110, 260);
#endif

glo->scwr = curwidg;

glo->mreg = mk_reg_menu( glo );
curwidg = mk_reg_view( glo );
gtk_container_add( GTK_CONTAINER( glo->scwr ), curwidg );
glo->tlisr = curwidg;

curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( curwidg), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
gtk_notebook_append_page( GTK_NOTEBOOK( glo->notl ), curwidg, gtk_label_new("Flags") );
gtk_widget_set_size_request (curwidg, 60, 100);
glo->scw2 = curwidg;

// seconde paire : disassembly + RAM
// disassembly
curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( curwidg), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS );
gtk_paned_pack1 (GTK_PANED (glo->hpan2), curwidg, TRUE, FALSE );
glo->scwl = curwidg;

glo->mdisa = mk_disa_menu( glo );
curwidg = mk_disa_view( glo );
gtk_container_add( GTK_CONTAINER( glo->scwl ), curwidg );
glo->tlisl = curwidg;

// RAM
curwidg = gtk_vbox_new( FALSE, 2 );
gtk_paned_pack2 (GTK_PANED (glo->hpan2), curwidg, FALSE, FALSE );
#ifdef PRINT_64
gtk_widget_set_size_request (curwidg, 238, 100);
#else
gtk_widget_set_size_request (curwidg, 168, 100);
#endif
glo->vram = curwidg;

curwidg = gtk_entry_new();
g_signal_connect( curwidg, "activate",
                  G_CALLBACK( ram_adr_call ), (gpointer)glo );
gtk_entry_set_editable( GTK_ENTRY(curwidg), TRUE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "" );
gtk_box_pack_start( GTK_BOX( glo->vram ), curwidg, FALSE, FALSE, 0 );
glo->eram = curwidg;

curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( curwidg), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS );
gtk_box_pack_start( GTK_BOX( glo->vram ), curwidg, TRUE, TRUE, 0 );
glo->scwm = curwidg;

glo->mram = mk_ram_menu( glo );
curwidg = mk_ram_view( glo );
gtk_container_add( GTK_CONTAINER( glo->scwm ), curwidg );
glo->tlism = curwidg;

// Le transcript dans la moitie inferieure de la paned verticale
curwidg = glo->t.create();
gtk_paned_pack2( GTK_PANED(glo->vpan), curwidg, FALSE, TRUE );
gtk_widget_set_size_request( curwidg, 700, 200 );
glo->wtran = curwidg;

// boite horizontale
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 2);
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->hbut = curwidg;

/* entree editable */
curwidg = gtk_entry_new();
g_signal_connect( curwidg, "activate",
                  G_CALLBACK( cmd_call ), (gpointer)glo );
gtk_entry_set_editable( GTK_ENTRY(curwidg), TRUE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "" );
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
curwidg = gtk_check_button_new_with_label ("stream dump");
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( curwidg ), FALSE );
glo->btog3 = curwidg;

// check bouton
curwidg = gtk_check_button_new_with_label ("raw dump");
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( curwidg ), FALSE );
glo->btog4 = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Quit ");
g_signal_connect( curwidg, "clicked",
                  G_CALLBACK( quit_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->bqui = curwidg;

gtk_widget_show_all( glo->wmain );
}
