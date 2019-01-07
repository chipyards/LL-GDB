
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
unsigned int ilist;

unsigned int exp_N;		// entier pour experiences

} glostru;
