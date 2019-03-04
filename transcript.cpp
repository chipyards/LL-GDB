/* systeme de log dans une fenetre de transcript GtkTextView
   chaque fois que le nombre de lignes atteint MAXLINES
   le premier quart des lignes est retire du buffer */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdarg.h>

#include "futf8.h"
#include "transcript.h"

/** ========================== metodos ========================= */

void transzcript::printf( const char *fmt, ... )
{
char lbuf[1024];
u8filtre filu;
/////// formatage du message ///////
va_list argptr;
va_start( argptr, fmt );
vsnprintf( lbuf, sizeof(lbuf), fmt, argptr );
va_end( argptr );
// traitement caractere par caractere : filtrage sur place
// enlever UTF8 invalide
char c; unsigned int i = 0, j = 0;
filu.errcnt = 0;
while	( ( c = lbuf[i++] ) != 0 )
	{
	filu.putc( c );
	while	( filu.avail() )
		{ lbuf[j++] = filu.getc(); }
	if	( j >= sizeof(lbuf) )
		{ j = sizeof(lbuf) - 1; break; }
	}
lbuf[j] = 0;
// enlever caracteres de controle
i = 0; j = 0;
while	( ( c = lbuf[i++] ) != 0 )
	{
	if	( ( c < ' ' ) && ( c != '\n' ) )
		c = '?';
	// if	( ( c == '<' ) || ( c == '>' ) )
	//	c = '|';
	lbuf[j++] = c;
	}
lbuf[j] = 0;
/* un luxe superflu *
if ( filu.errcnt )
   {
   char tbuf[64];
   snprintf( tbuf, sizeof(tbuf), "%d utf-8 invalid codes", filu.errcnt );
   modpop( "Zwarning", tbuf, GTK_WINDOW(global_main_window) );
   }
//*/
///////// copie dans le buffer d'affichage //////////
  // Get end iterator
GtkTextIter iter;
gtk_text_buffer_get_end_iter( lebuf, &iter );
  // insert text, the iter will be revalidated after insertion to the end of inserted text
if	( lbuf[0] =='E' )	// ERROR
	gtk_text_buffer_insert_with_tags_by_name( lebuf, &iter, lbuf, -1, "redux", NULL );
else if	( lbuf[0] =='>' )	// commande locale
	gtk_text_buffer_insert_with_tags_by_name( lebuf, &iter, lbuf, -1, "greenux", NULL );
else if	( lbuf[0] =='(' )	// Prompt
	gtk_text_buffer_insert_with_tags_by_name( lebuf, &iter, lbuf, -1, "promptux", NULL );
else if	( lbuf[0] ==':' )	// debug interne
	gtk_text_buffer_insert_with_tags_by_name( lebuf, &iter, lbuf, -1, "blux", NULL );
else	gtk_text_buffer_insert( lebuf, &iter, lbuf, -1 );

///////// limitation de taille /////////
int linum; GtkTextIter iter0;
// compter les lignes
linum = gtk_text_iter_get_line( &iter );
// couper le premier quart si necessaire
if	( linum > MAXLINES )
	{
	gtk_text_buffer_get_start_iter( lebuf, &iter0 );
	gtk_text_buffer_get_iter_at_line( lebuf, &iter, MAXLINES / 4 );
	gtk_text_buffer_delete( lebuf, &iter0 , &iter );
	// remettre iter a la fin pour scroll
	gtk_text_buffer_get_end_iter( lebuf, &iter );
	}

////////// scrolling //////////
  // Move the iterator to the beginning of line, so we don't scroll horizontaly
gtk_text_iter_set_line_offset( &iter, 0 );
// ici 2 solutions
/* la plus simple mais elle marche po bien (cache 1 ligne en bas) *
gtk_text_view_scroll_to_iter( GTK_TEXT_VIEW(ttext), &iter, 0.4, FALSE, 0.0, 0.0 );
//*/
/* et la compliquee */
  // Place the mark at iter.
GtkTextMark *mark;
mark = gtk_text_buffer_get_mark( lebuf, "kirichi" );
gtk_text_buffer_move_mark( lebuf, mark, &iter );
  // Scroll the mark onscreen.
gtk_text_view_scroll_mark_onscreen( GTK_TEXT_VIEW(ttext), mark );
// gtk_text_view_scroll_to_mark( GTK_TEXT_VIEW(ttext), mark, 0.4, FALSE, 0.0, 0.0 );
//*/

}

void transzcript::clear()
{
GtkTextIter iter0, iter1;
gtk_text_buffer_get_start_iter( lebuf, &iter0 );
gtk_text_buffer_get_end_iter( lebuf, &iter1 );
gtk_text_buffer_delete( lebuf, &iter0 , &iter1 );
}

// =================== THE quasi KONSTRUKTOR =======================

// on ne met pas cela dans le constructeur pour ne pas risquer de l'executer
// avant gtk_init()

GtkWidget * transzcript::create()
{
GtkWidget * curwidg;

// scrolled window pour le widget texte
curwidg = gtk_scrolled_window_new( NULL, NULL );
gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(curwidg), GTK_SHADOW_IN );
gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW(curwidg),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS );
wscro = curwidg;

// le widget texte NON EDITABLE
curwidg = gtk_text_view_new();
lebuf = gtk_text_view_get_buffer( GTK_TEXT_VIEW(curwidg) );
gtk_text_view_set_editable( GTK_TEXT_VIEW(curwidg), FALSE );
gtk_container_add( GTK_CONTAINER(wscro), curwidg );

// marqueur pour auto-scroll
GtkTextIter iter;
gtk_text_buffer_get_end_iter( lebuf, &iter );
gtk_text_buffer_create_mark( lebuf, "kirichi", &iter, TRUE );

// Change default font throughout the widget
PangoFontDescription * font_desc;
font_desc = pango_font_description_from_string("Monospace 10");
gtk_widget_modify_font( curwidg, font_desc );
pango_font_description_free( font_desc );

// Change left margin throughout the widget
gtk_text_view_set_left_margin( GTK_TEXT_VIEW( curwidg ), 10 );

ttext = curwidg;

// tags pour la colorisation
// NOTE : la couleur peut etre specifiee par un nom anglais "red", "pink"
// ou par une chaine a la HTML "#FF8000" - ceci n'est pas documente...
gtk_text_buffer_create_tag( lebuf, "redux", "background", "#ffAAAA", NULL);
gtk_text_buffer_create_tag( lebuf, "greenux", "background", "#00EE66", NULL);
gtk_text_buffer_create_tag( lebuf, "blux", "background", "#66BBFF", NULL);	// "scale", PANGO_SCALE_X_LARGE, "weight", PANGO_WEIGHT_BOLD, NULL);
gtk_text_buffer_create_tag( lebuf, "promptux", "background", "#FFFF00", NULL);

return wscro;
}
