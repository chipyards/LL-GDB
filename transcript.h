/* systeme de log dans une fenetre de transcript GtkTextView
   chaque fois que le nombre de lignes atteint MAXLINES
   le premier quart des lignes est retire du buffer */

#define MAXLINES 8000

class transzcript {
public :
GtkWidget * wscro;	// scrolled window
GtkWidget * ttext;	// text view
GtkTextBuffer * lebuf;	// text buffer
// constructor
transzcript() : wscro(NULL) {};
// methodes principales
void printf( const char *fmt, ... );
void clear();
// accesseurs
GtkWidget * create();	// rend la fenetre principale pour empaquetage dans le GUI
};

