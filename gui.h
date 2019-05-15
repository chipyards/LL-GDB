#define CHREG_COLOR "#90E0FF"	// fond registre change
#define IP_COLOR  "#FFEE55"	// fond nom rip
#define IP_COLOR2 "#DDBB00"	// triangle
#define SP_COLOR  "#FFB0B0"	// fond nom rsp
#define SP_COLOR2 "#E00000"	// text adr == rsp
#define BP_COLOR  "#B0FFB0"	// fond nom rbp
#define BP_COLOR2 "#00C000"	// text adr == rbp

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
#define MARGIN_IP " <span color=\"" IP_COLOR2 "\">" UTF8_TRIANGLE "</span>"
#define MARGIN_NONE "  "
#define MARGIN_BK "<span color=\"#FF0000\">" UTF8_CIRCLE "</span> "
#define MARGIN_BKIP "<span color=\"#FF0000\">" UTF8_CIRCLE "</span><span foreground=\"" IP_COLOR2 "\">" UTF8_TRIANGLE "</span>"
// pango markup pour le disassembly
#define CSOURCE_COLOR	"#20A020"
#define ADDR_COLOR	"#0050D0"
#define BIN_COLOR	"#D00000"
#define CSOURCE "<span color=\"" CSOURCE_COLOR "\">"
#define ASMADDR "<span color=\"" ADDR_COLOR "\">"
#define BINCODE "<span color=\"" BIN_COLOR "\">"
// pango markup pour la marge de memory
#define MARGIN_SP "<span color=\"" SP_COLOR2 "\">" UTF8_TRIANGLE "</span>"
#define MARGIN_BP "<span color=\"" BP_COLOR2 "\">" UTF8_TRIANGLE "</span>"
#define MARGIN_NOSP " "
// pango markup pour les tooltips
#define HOTKEY_COLOR	"#EE00AA"
#define HOTKEY "<span color=\"" HOTKEY_COLOR "\" font-weight=\"bold\">"

typedef struct
{
GtkWidget * wmain;			// main window
GtkWidget * hpan;			// horizontal paned pair
GtkWidget *   vleft;			// vertical box
GtkWidget *     htool;			// horizontal toolbox
GtkWidget *       mbar;			// window menu (file etc...)
GtkWidget *       bani;			// spinner showing running state
GtkWidget *       enam;			// text entry for target program name
GtkWidget *     vpan;			// vertical paned pair
GtkWidget *       hpan2;		// second horizontal paned pair
GtkWidget *         notrf;		// notebook reg/flags
GtkWidget *           scwr;		// scrollable window for regs
GtkWidget *             tlisr;		// tree view used as list view
GtkListStore *            tmodr;	// list model
GtkWidget *               mreg;		// context menu appele par une call back de la treeview
GtkWidget *                 itrg;	// menu item for register
unsigned int              reg_sel_i;	// index de la ligne courante
GtkWidget *           tlisf;		// tree view used as list view
GtkListStore *          tmodf;		// list model
GtkWidget *         scwl;		// scrollable window for disassembly
GtkWidget *           tlisl;		// tree view used as list view
GtkListStore *          tmodl;		// list model
GtkWidget *             mdisa;		// context menu
GtkWidget *               itbk;		// menu item for breakpoint
int			disa_sel_ref;	// ref de la ligne de disas courante
unsigned long long	disa_sel_adr;	// addr de la ligne de disas courante
GtkTreeViewColumn *     adrcol;		// assembly adr column
GtkTreeViewColumn *     bincol;		// assembly bin column
GtkTreeViewColumn *     asmcol;		// assembly src column
transzcript t;
GtkWidget *       wtran;		// transcript window
GtkWidget *     hbut;			// horizontal box
GtkWidget *       ecmd;			// text entry for command line
GtkWidget *       btog1;		// button
GtkWidget *       btog2;		// button
GtkWidget *       btog3;		// button
GtkWidget *       btog4;		// button

GtkWidget *   vram;			// boite verticale
GtkWidget *     eram;			// text entry pour addr
GtkWidget *     scwm;			// scrollable window for memory
GtkWidget *       tlism;		// tree view used as list view
GtkListStore *      tmodm;		// list model
GtkWidget *         mram;		// context menu
GtkWidget *           itram8;		// menu item for 8-bit format
GtkWidget *           itram16;		// etc...
GtkWidget *           itram32;
GtkWidget *           itram64;
GtkWidget *           itram65;		// 64 bits, display ascii 8 chars
unsigned int        ram_sel_i;		// index de la ligne courante
GtkTreeViewColumn * madrcol;		// ram adr column
GtkTreeViewColumn * mdatcol;		// ram data column

int         idle_id;			// id pour la fonction idle du timeout

// fenetre annexe pour editeur
GtkWidget * wedi;			// main window
GtkWidget * vedi;			// vertical box
GtkWidget *   sedi;			// scrollable window for text
GtkWidget *     tedi;			// text window
GtkTextBuffer *   bedi;			// text buffer



daddy * dad;
mi_parse * mipa;
target * targ;

unsigned int ilist;		// indice du listing courant
unsigned int ip_in_list;	// indice de la ligne de eip dans le listing courant
long long disa_adr;		// adresse pour desassemblage ( 0 <==> ip )
int option_child_console;	// pour debug d'un prog qui utilise stdout et/ou stdin
int option_flavor;		// 0 = AT&T, 1 = Intel
// int option_binvis;		// (cette option est dans la classe target)
unsigned int option_ramblock;	// number of bytes for RAM read
unsigned int option_disablock;	// number of bytes for disassembly
int option_toggles;		// etat initial des check_buttons (1bit per toggle button)
int option_manual_start;	// breakpoints initiaux a mettre a la main

} glostru;

/** fonctions d'action ---------------------------*/

void queue_cmd( glostru * glo, const char * cmd, job_enum job );
void init_step( glostru *glo, int autostart );
void disassembly_at( glostru * glo );
void breakpoint_at( glostru * glo );
void expe( glostru * glo );

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

/** widget callbacks ------------------------------*/
// tool buttons
void restart_call( GtkWidget *widget, glostru * glo );
void run_call( GtkWidget *widget, glostru * glo );
void step_into_call( GtkWidget *widget, glostru * glo );
void step_over_call( GtkWidget *widget, glostru * glo );
void step_out_call( GtkWidget *widget, glostru * glo );
// context menus
void reg_call_copy( GtkWidget *widget, glostru * glo );
void reg_call_copy_all( GtkWidget *widget, glostru * glo );
gboolean reg_right_call( GtkWidget *curwidg, GdkEventButton *event, glostru * glo );
void disa_call_bk( GtkWidget *widget, glostru * glo );
void disa_call_bk_run( GtkWidget *widget, glostru * glo );
void disa_call_bk_all( GtkWidget *widget, glostru * glo );
void disa_call_flavor( GtkWidget *widget, glostru * glo );
void disa_call_binvis( GtkWidget *widget, glostru * glo );
void disa_call_copy_adr( GtkWidget *widget, glostru * glo );
void disa_call_copy_code( GtkWidget *widget, glostru * glo );
void disa_call_copy_all( GtkWidget *widget, glostru * glo );
gboolean disa_right_call( GtkWidget *curwidg, GdkEventButton *event, glostru * glo );
void ram_call_fmt( GtkWidget *widget, glostru * glo );
void ram_call_copy( GtkWidget *widget, glostru * glo );
void ram_call_copy_all( GtkWidget *widget, glostru * glo );
gboolean ram_right_call( GtkWidget *curwidg, GdkEventButton *event, glostru * glo );
// entries
void ram_adr_call( GtkWidget *widget, glostru * glo );
void cmd_call( GtkWidget *widget, glostru * glo );
// treeview data callbacks
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
void disa_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo );
void flag_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo );
void ram_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo );
// misc
void update_RAM( glostru * glo );

/** ============================ list store utilities ======================= */

void list_store_resize( GtkListStore * mod, unsigned int size );
unsigned int list_store_cnt( GtkListStore * mod );
