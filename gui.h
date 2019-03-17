#define CHREG_COLOR "#90E0FF"	// fond registre change
#define IP_COLOR  "#FFEE55"	// fond nom rip
#define IP_COLOR2 "#DDBB00"	// triangle
#define SP_COLOR  "#FFB0B0"	// fond nom rsp
#define SP_COLOR2 "#E00000"	// text adr == rsp
#define BP_COLOR  "#B0FFB0"	// fond nom rbp
#define BP_COLOR2 "#00C000"	// text adr == rbp

#define UTF8_TRIANGLE	"\xe2\x96\xba"
#define UTF8_CIRCLE	"\xe2\x97\x8f"

#ifdef __amd64
#define PRINT_64
#endif

#ifdef	PRINT_64
#define OPT_FMT "%016llX"
typedef unsigned long long opt_type;
#else
#define OPT_FMT "%08X"
typedef unsigned int opt_type;
#endif

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
// pango markup pour la marge de memory
#define MARGIN_SP "<span foreground=\"" SP_COLOR2 "\">" UTF8_TRIANGLE "</span>"
#define MARGIN_BP "<span foreground=\"" BP_COLOR2 "\">" UTF8_TRIANGLE "</span>"
#define MARGIN_NOSP " "

typedef struct
{
GtkWidget * wmain;			// main window
GtkWidget * vmain;			// vertical box
GtkWidget *   vpan;			// vertical paned pair
GtkWidget *     hpan;			// horizontal paned pair
GtkWidget *       notl;			// left notebook
GtkWidget *         scwr;		// scrollable window for regs
GtkListStore *        tmodr;		// list model
GtkWidget *           tlisr;		// tree view used as list view
GtkWidget *         scw2;		// scrollable window for flags
GtkWidget *       hpan2;		// second horizontal paned pair

GtkWidget *         scwl;		// scrollable window for disassembly
GtkListStore *        tmodl;		// list model
GtkWidget *           tlisl;		// tree view used as list view
GtkWidget *             mdisa;		// context menu
GtkWidget *               itbk;		// menu item for breakpoint
unsigned long long	bkadr;		// addr for breakpoint
GtkTreeViewColumn *     adrcol;		// assembly adr column
GtkTreeViewColumn *     bincol;		// assembly bin column
GtkTreeViewColumn *     asmcol;		// assembly src column

GtkWidget *         vram;		// boite verticale
GtkWidget *           eram;		// text entry pour addr
GtkWidget *           scwm;		// scrollable window for memory
GtkListStore *          tmodm;		// list model
GtkWidget *             tlism;		// tree view used as list view
GtkWidget *               mram;		// context menu
GtkWidget *                 itram8;	// menu item for format
GtkWidget *                 itram16;
GtkWidget *                 itram32;
GtkWidget *                 itram64;
GtkTreeViewColumn *       madrcol;	// ram adr column
GtkTreeViewColumn *       mdatcol;	// ram data column

transzcript t;
GtkWidget *     wtran;			// transcript window
GtkWidget *   mbar;			// horizontal menu
GtkWidget *   hbut;			// horizontal box
GtkWidget *     ecmd;			// text entry
GtkWidget *     btog1;			// button
GtkWidget *     btog2;			// button
GtkWidget *     btog3;			// button
GtkWidget *     btog4;			// button
GtkWidget *     bqui;			// button

// fenetre annexe pour editeur
GtkWidget * wedi;			// main window
GtkWidget * vedi;			// vertical box
GtkWidget *   sedi;			// scrollable window for text
GtkWidget *     tedi;			// text window
GtkTextBuffer *   bedi;			// text buffer



daddy * dad;
mi_parse * mipa;
target * targ;
int ram_format;

unsigned int ilist;		// indice du listing courant
unsigned int ip_in_list;	// indice de la ligne de eip dans le listing courant

int option_child_console;	// pour debug d'un prog qui utilise stdout et/ou stdin
int option_flavor;		// 0 = AT&T, 1 = Intel
// int option_binvis;		// (cette option est dans la classe target)
unsigned int option_ramblock;	// number of bytes for RAM read
unsigned int option_disablock;	// number of bytes for disassembly
int option_toggles;		// etat initial des check_buttons (1bit per toggle button)

} glostru;

/** fonctions de layout ---------------------------*/

// create the disassembly view context menu
GtkWidget * mk_disa_menu( glostru * glo );
// Create the disassembly view
GtkWidget * mk_disa_view( glostru * glo );
// Create the register view
GtkWidget * mk_reg_view( glostru * glo );
// Create the memory view
GtkWidget * mk_ram_view( glostru * glo );
// Create the menubar
GtkWidget * mk_bars( glostru * glo );
// make the GUI
void mk_the_gui( glostru * glo );
// make the external editor window
void mk_editor( glostru * glo );

/** widget callbacks ------------------------------*/

void disa_call_bk( GtkWidget *widget, glostru * glo );
void disa_call_bk_all( GtkWidget *widget, glostru * glo );
void disa_call_flavor( GtkWidget *widget, glostru * glo );
void disa_call_binvis( GtkWidget *widget, glostru * glo );
void disa_call_editor( GtkWidget *widget, glostru * glo );
gboolean disa_right_call( GtkWidget *curwidg, GdkEventButton *event, glostru * glo );
void ram_call_fmt( GtkWidget *widget, glostru * glo );
gboolean ram_right_call( GtkWidget *curwidg, GdkEventButton *event, glostru * glo );
void ram_adr_call( GtkWidget *widget, glostru * glo );
void cmd_call( GtkWidget *widget, glostru * glo );

void disa_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo );
void regname_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo );
void regval_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo );
void ram_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo );

/** ============================ list store utilities ======================= */

void list_store_resize( GtkListStore * mod, unsigned int size );
unsigned int list_store_cnt( GtkListStore * mod );
