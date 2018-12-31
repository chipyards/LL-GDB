
typedef struct
{
GtkWidget * wmain;		// main window
GtkWidget * vmain;		// vertical box
GtkWidget *   vpan;		// vertical paned pair
GtkWidget *     hpan;		// horizontal paned pair
GtkWidget *       notl;		// left notebook
GtkWidget *         scw1;	// scrollable window for stack
GtkWidget *         scw2;	// scrollable window for stack
GtkWidget *       notr;		// right notebook
GtkWidget *         scw3;	// scrollable window for disassembly
GtkWidget *         scw4;	// scrollable window for disassembly
GtkWidget *     wtran;		// transcript window
GtkWidget *   mbar;		// horizontal menu
GtkWidget *   hbut;		// horizontal box
GtkWidget *     ecmd;		// text entry
GtkWidget *     btog;		// button
GtkWidget *     bqui;		// button

transzcript t;

daddy * dad;
mi_parse * mipa;
regbanko * regbank;

} glostru;
