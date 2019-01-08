#define IP_COLOR "#FFEE55"

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
GtkWidget *         scw4;	// scrollable window for memory
GtkWidget *     wtran;		// transcript window
GtkWidget *   mbar;		// horizontal menu
GtkWidget *   hbut;		// horizontal box
GtkWidget *     ecmd;		// text entry
GtkWidget *     btog;		// button
GtkWidget *     bqui;		// button

transzcript t;

daddy * dad;
mi_parse * mipa;
target * targ;
unsigned int ilist;		// indice du listing courant
unsigned int ip_in_list;	// indice de la ligne de eip dans le listing courant

int option_child_console;	// pour debug d'un prog qui utilise stdout et/ou stdin
unsigned int exp_N;		// entier generique pour experiences

} glostru;
