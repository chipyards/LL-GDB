#define IP_COLOR  "#FFEE55"
#define IP_COLOR2 "#DDBB00"

#define UTF8_TRIANGLE	"\xe2\x96\xba"
#define UTF8_CIRCLE	"\xe2\x97\x8f"

/*glo->t.printf(
"stop     238a |\xe2\x8e\x8a|\n"
"continue 23EF |\xe2\x8f\xaf|\n"
"record   23FA |\xe2\x8f\xba|\n"
"fish eye 25C9 |\xe2\x97\x89|\n"
"triangle 25B6 |\xe2\x96\xb6|\n"
"triangle 25BA |" UTF8_TRIANGLE "|\n"
"circle   25CF |" UTF8_CIRCLE "|\n"
"circle   26AB |\xe2\x9a\xab|\n"
"              |_|\n" );
*/
// pango markup pour la marge du disassembly
#define MARGIN_IP " <span foreground=\"" IP_COLOR2 "\">" UTF8_TRIANGLE "</span>"
#define MARGIN_NONE "  "
#define MARGIN_BK "<span foreground=\"#FF0000\">" UTF8_CIRCLE "</span> "
#define MARGIN_BKIP "<span foreground=\"#FF0000\">" UTF8_CIRCLE "</span><span foreground=\"" IP_COLOR2 "\">" UTF8_TRIANGLE "</span>"

typedef struct
{
GtkWidget * wmain;		// main window
GtkWidget * vmain;		// vertical box
GtkWidget *   vpan;		// vertical paned pair
GtkWidget *     hpan;		// horizontal paned pair
GtkWidget *       notl;		// left notebook
GtkWidget *         scwr;	// scrollable window for regs
GtkListStore *        tmodr;	// list model
GtkWidget *           tlisr;	// tree view used as list view
GtkWidget *         scw2;	// scrollable window for stack
GtkWidget *       notr;		// right notebook
GtkWidget *         scwl;	// scrollable window for disassembly
GtkListStore *        tmodl;	// list model
GtkWidget *           tlisl;	// tree view used as list view
GtkWidget *             mdisa;	// context menu
GtkWidget *               itbk;	// menu item for breakpoint
unsigned long long	  bkadr;// addr for breakpoint
GtkWidget *         scw4;	// scrollable window for memory
GtkWidget *     wtran;		// transcript window
GtkWidget *   mbar;		// horizontal menu
GtkWidget *   hbut;		// horizontal box
GtkWidget *     ecmd;		// text entry
GtkWidget *     btog1;		// button
GtkWidget *     btog2;		// button
GtkWidget *     btog3;		// button
GtkWidget *     btog4;		// button
GtkWidget *     bqui;		// button


transzcript t;

daddy * dad;
mi_parse * mipa;
target * targ;
int timor;			// timer, unite = idle loop = 31ms env. (30 fps)

unsigned int ilist;		// indice du listing courant
unsigned int ip_in_list;	// indice de la ligne de eip dans le listing courant

int option_child_console;	// pour debug d'un prog qui utilise stdout et/ou stdin
unsigned int exp_N;		// entier generique pour experiences

} glostru;
