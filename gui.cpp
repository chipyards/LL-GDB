// here below is the main(), mother of everything

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdarg.h>
#include <locale.h>


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

/** ============================ list store utilities ======================= */

void list_store_resize( GtkListStore * mod, unsigned int size )
{	// modele minimal, 1 colonne de type int
GtkTreeIter curiter;
gtk_list_store_clear( mod );
for ( unsigned int i = 0; i < size; i++ )
    {
    gtk_list_store_append( mod, &curiter );
    gtk_list_store_set( mod, &curiter, 0, i, -1 );
    }
}

unsigned int list_store_cnt( GtkListStore * mod )
{
GtkTreeIter curiter;
GtkTreePath * curpath;
int * indices;
int depth;
gtk_list_store_append( mod, &curiter );
curpath = gtk_tree_model_get_path( (GtkTreeModel *)mod, &curiter );
gtk_list_store_remove ( mod, &curiter );
indices = gtk_tree_path_get_indices_with_depth( curpath, &depth );
gtk_tree_path_free( curpath );
if	( depth == 1 )	return indices[0];
else			return -depth;
}

// copie le texte src en en le preparant pour le markup i.e. remplacer
//	& par &amp;
//	< par &lt;
int markup_filter( char * tbuf, unsigned int size, const char * src )
{
unsigned int i = 0, j = 0; char c;
while	( j < ( size - 5 ) )
	{
	c = src[i++];
	if	( c == '<' )
		{ tbuf[j++] = '&'; tbuf[j++] = 'l'; tbuf[j++] = 't'; tbuf[j++] = ';'; }
	else	tbuf[j++] = c;
	if	( c == '&' )
		{ tbuf[j++] = 'a'; tbuf[j++] = 'm'; tbuf[j++] = 'p'; tbuf[j++] = ';'; }
	if	( c == 0 )
		break;
	}
tbuf[size-1] = 0;
return (j)?(j-1):0;
}


/** ============================ action functions ======================= */

void init_step( glostru * glo );
void update_disass( glostru * glo );

void some_stats( glostru * glo )
{
glo->t.printf("%d (%d) asm lines\n", glo->targ->asmmap.size(), glo->targ->asmstock.size() );
// for	( unsigned int i = 0; i < glo->targ->asmstock.size(); ++i )
//	glo->targ->asmstock[i].dump();
for	( unsigned int i = 0; i < glo->targ->filestock.size(); ++i )
	glo->t.printf("fichier src : %s %s\n", glo->targ->filestock[i].relpath.c_str(), glo->targ->filestock[i].abspath.c_str() );
unsigned int qlist = glo->targ->liststock[glo->ilist].lines.size();
glo->t.printf("%d listing lines\n", qlist );
glo->t.printf("%d disass model rows\n", list_store_cnt( glo->tmodl ) );
glo->t.printf("%d breakpoints\n", glo->targ->breakpoints.size() );
glo->t.printf("RAM data %u words @ 0x%08X\n", (unsigned int)glo->targ->ramstock[0].w32.size(), (unsigned int)glo->targ->ramstock[0].adr0 );
glo->t.printf("%d RAM model rows\n", list_store_cnt( glo->tmodm ) );
glo->t.printf("job_status=%010llX\n", glo->targ->job_status );
glo->targ->job_dump();
glo->t.printf("iip=%d, isp=%d, ibp=%d\n", glo->targ->regs.iip, glo->targ->regs.isp, glo->targ->regs.ibp );
}

// mettre dans la queue une commande pour GDB, avec echo optionnel dans le transcript (N.B. LF sera ajoute automatiquement)
void queue_cmd( glostru * glo, const char * cmd, job_enum job  )
{
if	( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog1) ) )
	glo->t.printf(">q %s\n", cmd );
glo->targ->job_queue_cmd( cmd, job );
}

// demarrer le prochain job de la queue, s'il existe et si c'est possible
void next_run( glostru * glo )
{
if	( glo->targ->job_isanyrunning() )
	return;
if	( glo->targ->job_isanyerror() )
	{
	if	(
		( glo->targ->reason == string("exited-normally") ) ||
		( glo->targ->reason == string("exited") )
		)
		{
		glo->t.printf(": %s\n", glo->targ->reason.c_str() );
		modpop( "Info", "program exited normally", GTK_WINDOW(glo->wmain) );
		}
	else	{
		glo->t.printf("E %s\n", glo->targ->error_msg.c_str() );
		modpop( "Error", glo->targ->error_msg.c_str(), GTK_WINDOW(glo->wmain) );
		}
	glo->targ->job_status &= (~(ERROR_MASK));
	return;
	}
if	( glo->targ->job_isanyqueued() == 0 )
	return;
int ijob = glo->targ->job_nextqueued();
if	( ijob < 0 )
	return;
const char * cmd = glo->targ->job_cmd[ijob].c_str();
glo->targ->job_set_running( (job_enum)ijob );
if	( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog1) ) )
	glo->t.printf(">r %s\n", cmd );
glo->dad->send_cmd( cmd );
glo->dad->send_cmd( "\n" );
}

void expa( glostru * glo )
{
update_disass( glo );
list_store_resize( glo->tmodm, glo->targ->get_ram_qlines(0) );	// hum ce n'est pas l'endroit ou faire ça, trop frequent
gtk_widget_queue_draw( glo->wmain );
glo->targ->job_status &= (~RUNNING_MASK);
}

void expb( glostru * glo )
{
some_stats( glo );
}

void expd( glostru * glo )
{
gasp("GAAAASp");
}

// fonction a appeler chaque fois que ip a change
void update_disass( glostru * glo )
{
// notre ip est-il deja desassemble ? cherchons son index dans asmstock
int ip_asm_line = glo->targ->get_ip_asm_line();
if	( ip_asm_line >= 0 )
	{
	// glo->t.printf("line %d in asmstock\n", ip_asm_line );
	// OUI alors est-il dans le listing courant ?
	int ip_in_list = glo->targ->liststock[glo->ilist].search_line( ip_asm_line, glo->ip_in_list );
	if	( ip_in_list < 0 )
		{ // NON refaisons le listing (pour le moment on ne sait que travailler sur 1 seul)
		// N.B. fill_listing ecrase le contenu anterieur
		// ici ce serait interessant de voir si on ne peut pas plutot prolonger le listing
		glo->targ->fill_listing( glo->ilist, glo->targ->get_ip() );
		// on a besoin du compte des lignes pour mettre a jour le tree model
		unsigned int qlist = glo->targ->liststock[glo->ilist].lines.size();
		list_store_resize( glo->tmodl, qlist );
		}
	ip_in_list = glo->targ->liststock[glo->ilist].search_line( ip_asm_line, 0 );
	if	( ip_in_list >= 0 )
		{ // OUI alors le scroll doit marcher
		// glo->t.printf("line %d in list\n", ip_in_list );
		glo->ip_in_list = ip_in_list;
		// scroll sur ip
		GtkTreePath * lepath;
		lepath = gtk_tree_path_new_from_indices( glo->ip_in_list, -1 );
		// char *pipo = gtk_tree_path_to_string( lepath );
		// glo->t.printf( "scroll to %d (%s)\n", glo->ip_in_list, pipo );
		gtk_tree_view_scroll_to_cell( (GtkTreeView *)glo->tlisl,
			lepath,	// GtkTreePath *path,
			NULL, 	// *column,
			TRUE,	// use_align,
			0.5,	// row_align,
			0.0 );	// col_align
		gtk_tree_path_free( lepath );
		}
	}
else	{ // NON allons desassembler si possible
	unsigned long long adr;
	adr = glo->targ->get_ip();
	if	( ( adr ) && ( !glo->targ->job_is_queued(Disass) ) )
		{
		char tbuf[128];	// this fmt var below is to avoid bogus MinGW warnings about %llX
		const char * fmt = "-data-disassemble -s 0x" OPT_FMT " -e 0x" OPT_FMT " -- 5";
		snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)adr, glo->option_disablock + (opt_type)adr );
		queue_cmd( glo, tbuf, Disass );
		}
	}
}

// cette fonction pretend assurer les etapes de demarrage
void init_step( glostru *glo )
{
queue_cmd( glo, "-data-list-register-names", RegNames );
if	( glo->option_child_console )
	queue_cmd( glo, "-gdb-set new-console on", GDBSet );
// send_cmd( glo, "-gdb-set mi-async on");	// marche PO sous windows
queue_cmd( glo, "-exec-run --start", Run );	// ici GDB met un bk temporaire sur main
queue_cmd( glo, "-data-list-register-values x", RegVal );
// glo->targ->job_dump();
}

/** ============================ widget call backs ======================= */

void cmd_call( GtkWidget *widget, glostru * glo )
{
const char * cmd;
cmd = gtk_entry_get_text( GTK_ENTRY(widget) );
if	( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog1) ) )
	glo->t.printf("> %s\n", cmd );
glo->dad->send_cmd( cmd );
glo->dad->send_cmd( "\n" );
gtk_entry_set_text( GTK_ENTRY(widget), "" );
}

int idle_call( glostru * glo )
{
int retval;
char tbuf[1024];
int d;
int tog1 = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog1) );	// cmd dump
int tog2 = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog2) );	// MI dump
int tog3 = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog3) );	// streams dump
int tog4 = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glo->btog4) );	// raw dump

// la boucle de lecture de la pipe de sortie de GDB
while	( ( d = glo->dad->child_getc() ) >= 0 )
	{
	if	( tog4 )
		glo->t.printf("%c", d );
	retval = glo->mipa->proc1char( d );
	if	( retval == 0 )				// 0 = un char banal, on continue
		continue;
	else if	( retval < 0 )				// <0 = erreur detectee par parseur de MI
		{
		modpop( "erreur", "erreur MI parser", GTK_WINDOW(glo->wmain) );
		glo->t.printf("Err %d\n", -retval ); break;
		}
	else	{					// >0 = fin de quelque chose
		// d'abord extraire pour mettre a jour la target
		glo->mipa->extract( retval, glo->targ );
		// ensuite les dumps optionnels
		if	(					// dump du MI indente
			( ( tog2 ) && ( !tog4 ) ) &&
			( ( retval >= 1 ) && ( retval <= 8 ) )
			)
			{
			if	( glo->mipa->dump( retval, tbuf, sizeof(tbuf) ) )
				glo->t.printf("%s\n", tbuf );
			}
		else if	(					// dump du prompt et des streams
			( ( tog3 ) && ( !tog4 ) ) &&
			( retval == 9 )
			)
			{
			if	( glo->mipa->dump( retval, tbuf, sizeof(tbuf) ) )
				{
				tbuf[sizeof(tbuf)-1] = 0;	// si jamais il manque le terminateur
				int pos = (int)strlen(tbuf) - 1;
				while	( ( pos >= 0 ) && ( tbuf[pos] <= ' ' ) )
					tbuf[pos--] = 0;	// on enleve line end et trailing blank
				glo->t.printf("%s\n", tbuf );
				}
			}
		// ensuite regarder l'evolution des jobs (N.B. job_reset_running a seulement pu etre appele par extract() )
		job_enum fjob = glo->targ->job_finished();
		if	( fjob != NOJOB )
			{
			switch	( fjob )
				{
				case GDBSet:
				     break;
				case File:
				     break;
				case FileInfo:
				     break;
				case BreakSetKill: gtk_widget_queue_draw( glo->tlisl );
				     break;
				case BreakList: gtk_widget_queue_draw( glo->tlisl );
				     break;
				case Run:	if	( ( !tog2 ) && ( !tog3 ) && ( !tog4 ) )
						glo->t.printf("stopped : %s\n", glo->targ->reason.c_str() );
				     break;
				case Continue:	if	( ( !tog2 ) && ( !tog3 ) && ( !tog4 ) )
						glo->t.printf("stopped : %s\n", glo->targ->reason.c_str() );
				     break;
				case RegNames:	gtk_widget_queue_draw( glo->tlisr );
				     break;
				case RegVal:	gtk_widget_queue_draw( glo->tlisr );
						update_disass( glo ); gtk_widget_queue_draw( glo->tlisl );
								      gtk_widget_queue_draw( glo->tlisf );
				     break;
				case Disass:	update_disass( glo ); gtk_widget_queue_draw( glo->tlisl );
				     break;
				case RAMRead:	gtk_widget_queue_draw( glo->tlism );
						list_store_resize( glo->tmodm, glo->targ->get_ram_qlines(0) );
				     break;
				case NOJOB: break;
				}
			if	( tog1 )
				switch	( fjob )
					{
					case GDBSet:		glo->t.printf("end GDBSet\n");		break;
					case File:		glo->t.printf("end File\n");		break;
					case FileInfo:		glo->t.printf("end FileInfo\n");	break;
					case BreakSetKill:	glo->t.printf("end BreakSetKill\n");	break;
					case BreakList:		glo->t.printf("end BreakList\n");	break;
					case Run:		glo->t.printf("end Run\n");		break;
					case Continue:		glo->t.printf("end Continue\n");	break;
					case RegNames:		glo->t.printf("end RegNames\n");	break;
					case RegVal:		glo->t.printf("end RegVal\n");		break;
					case Disass:		glo->t.printf("end Disass\n");		break;
					case RAMRead:		glo->t.printf("end RAMRead\n");		break;
					case NOJOB: break;
					}
			// printf("job %d fini\n", (int)fjob ); fflush(stdout);
			}	// fin de "fin de job"
		}	// fin de "fin de quelque chose"
	}	// while child_getc()
if	( glo->targ->job_isanyrunning()  )
	{
	static GdkColor laranja = { 0, 0xFF00, 0xA000, 0x4000 };
	gtk_widget_modify_base( glo->ecmd, GTK_STATE_NORMAL, &laranja );
	}
else	{
	gtk_widget_modify_base( glo->ecmd, GTK_STATE_NORMAL, NULL );
	}
next_run( glo );
return( -1 );
}

void ram_adr_call( GtkWidget *widget, glostru * glo )
{
char tbuf[128]; const char * fmt;
unsigned long long adr;
adr = strtoull( gtk_entry_get_text( GTK_ENTRY(widget) ), NULL, 16 );	// accepte 0x ou hex brut
adr &= (~(unsigned long long)7);	// alignement autoritaire sur 64 bits
fmt = "0x" OPT_FMT;
snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)adr );
gtk_entry_set_text( GTK_ENTRY(widget), tbuf );
if	( adr > 0 )			// un peu foolproof mais pas trop
	{
	fmt = "-data-read-memory-bytes 0x" OPT_FMT " %u";
	snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)adr, glo->option_ramblock );
	queue_cmd( glo, tbuf, RAMRead );
	}
}

/** ================= context menus call backs ======================= */

void reg_call_copy( GtkWidget *widget, glostru * glo )
{
unsigned int i = glo->reg_sel_i; // glo->reg_sel_i a ete deja mis a jour par reg_right_call()
char tbuf[64]; const char * fmt = OPT_FMT;
if	(  i < glo->targ->regs.regs.size()  )
	snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)glo->targ->regs.regs[i].val );
else	tbuf[0] =0;
GtkClipboard * myclip;
myclip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
gtk_clipboard_set_text( myclip, tbuf, -1);
}

void reg_call_copy_all( GtkWidget *widget, glostru * glo )
{
string s;
glo->targ->regs.reg_all2string( &s );
GtkClipboard * myclip;
myclip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
gtk_clipboard_set_text( myclip, s.c_str(), -1);
}

// une call back pour le right-clic --> context menu
// le menu est deja cree (par mk_reg_menu), cette fonction le poppe apres avoir customise certains labels
gboolean reg_right_call( GtkWidget *curwidg, GdkEventButton *event, glostru * glo )
{
/* single click with the right mouse button? */
if	( ( event->type == GDK_BUTTON_PRESS ) && ( event->button == 3 ) )
	{	// tentative d'identifier la row :
        GtkTreePath *path;
        if	( gtk_tree_view_get_path_at_pos( GTK_TREE_VIEW(curwidg), event->x, event->y, &path, NULL, NULL, NULL) )
		{	// on a un path, on en extrait un indice
		unsigned int * indices; char text[32];
		int depth;
		indices = (unsigned int *)gtk_tree_path_get_indices_with_depth( path, &depth );
		gtk_tree_path_free(path);
		glo->reg_sel_i = *indices;
		if	( *indices < glo->targ->regs.regs.size() )
			snprintf( text, sizeof(text), "Copy %s", glo->targ->regs.regs[*indices].name.c_str() );
		else	text[0] = 0;
		gtk_menu_item_set_label( (GtkMenuItem *)glo->itrg, text );
		}
	gtk_menu_popup( (GtkMenu *)glo->mreg, NULL, NULL, NULL, NULL, event->button, event->time );
	return TRUE;			/* we handled this */
	}
return FALSE;				/* we did not handle this */
}

void disa_call_bk( GtkWidget *widget, glostru * glo )
{
char tbuf[128];
unsigned long long adr = glo->disa_sel_adr; // glo->disa_sel_adr a ete deja mis a jour par disa_right_call()
if	( adr )
	{
	if	( glo->targ->is_break( adr ) )
		{
		unsigned int bknum = glo->targ->breakpoints[adr];
		snprintf( tbuf, sizeof(tbuf), "-break-delete %u", bknum );
		}
	else	{
		const char * fmt = "-break-insert *0x" OPT_FMT;
		snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)adr );
		}
	queue_cmd( glo, tbuf, BreakSetKill );
	}
queue_cmd( glo, "-break-list", BreakList );
}

void disa_call_bk_all( GtkWidget *widget, glostru * glo )
{
queue_cmd( glo, "-break-delete", BreakSetKill );
queue_cmd( glo, "-break-list", BreakList );
}

void disa_call_flavor( GtkWidget *widget, glostru * glo )
{
glo->option_flavor ^= 1;
if	( glo->option_flavor )
	{
	gtk_tree_view_column_set_title( glo->asmcol, "Source Code (Intel flavor)" );
	queue_cmd( glo, "-gdb-set disassembly-flavor intel", GDBSet );
	}
else	{
	gtk_tree_view_column_set_title( glo->asmcol, "Source Code (AT&T flavor)" );
	queue_cmd( glo, "-gdb-set disassembly-flavor att", GDBSet );
	}
glo->targ->asm_init();	// effacer tout le disassembly
update_disass( glo );
}

void disa_call_binvis( GtkWidget *widget, glostru * glo )
{
glo->targ->option_binvis ^= 1;
if	( glo->targ->option_binvis )
	{
	gtk_tree_view_column_set_visible( glo->bincol, TRUE );
	}
else	{
	gtk_tree_view_column_set_visible( glo->bincol, FALSE );
	}
glo->targ->asm_init();	// effacer tout le disassembly car mi_parse tient compte de option_binvis
update_disass( glo );	// ce n'est pas seulement la visibilite des colonnes qui change
}

void disa_call_copy_adr( GtkWidget *widget, glostru * glo )
{
char tbuf[64]; const char * fmt = OPT_FMT;
if	( glo->disa_sel_ref >= 0 )
	snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)glo->disa_sel_adr );
else	tbuf[0] =0;
GtkClipboard * myclip;
myclip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
gtk_clipboard_set_text( myclip, tbuf, -1);
}

void disa_call_copy_code( GtkWidget *widget, glostru * glo )
{
int ref = glo->disa_sel_ref;
GtkClipboard * myclip;
myclip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
if	( ref < 0 )
	{		// ligne de code source
	unsigned int ilin = listing::decode_line_number(ref);
	unsigned int ifil = listing::decode_file_index(ref);
	gtk_clipboard_set_text( myclip, glo->targ->get_src_line( ifil, ilin ), -1);
	}
else	{		// ligne asm
	asmline * daline = &(glo->targ->asmstock[(unsigned int)ref]);
	if	( glo->targ->option_binvis )
		{
		char text[256];
		int pos = daline->bin2txt( text, sizeof(text) );
		snprintf( text+pos, sizeof(text)-pos, daline->asmsrc.c_str() );
		if	( sizeof(text) > ( pos + daline->asmsrc.size() + 1 ) )
			memcpy ( text+pos, daline->asmsrc.c_str(), daline->asmsrc.size()+1 );
		gtk_clipboard_set_text( myclip, text, -1);
		}
	else	gtk_clipboard_set_text( myclip, daline->asmsrc.c_str(), -1);
	}
}

void disa_call_copy_all( GtkWidget *widget, glostru * glo )
{
string s;
glo->targ->disa_all2string( &s, glo->ilist );
GtkClipboard * myclip;
myclip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
gtk_clipboard_set_text( myclip, s.c_str(), -1);
}

// une call back pour le right-clic --> context menu
// le menu est deja cree (par mk_disa_menu), cette fonction le poppe apres avoir customise certains labels
gboolean disa_right_call( GtkWidget *curwidg, GdkEventButton *event, glostru * glo )
{
/* single click with the right mouse button? */
if	( ( event->type == GDK_BUTTON_PRESS ) && ( event->button == 3 ) )
	{ // tentative d'identifier la row :
        GtkTreePath *path;
        if	( gtk_tree_view_get_path_at_pos( GTK_TREE_VIEW(curwidg), event->x, event->y, &path, NULL, NULL, NULL) )
		{	// on a un path, on en extrait un indice
		unsigned int * indices; int ref;
		int depth;
		indices = (unsigned int *)gtk_tree_path_get_indices_with_depth( path, &depth );
		// ici on pourrait forcer la selection mais bof
		gtk_tree_path_free(path);
		// recuperer la reference codee de ligne dans l'objet target
		ref = glo->targ->get_disa_ref( glo->ilist, *indices );
		glo->disa_sel_ref = ref;
		// decoder
		if	( ref < 0 )
			{		// ligne de code source
			glo->disa_sel_adr = 0;
			// glo->t.printf( "src line %u\n", listing::decode_line_number(ref) );
			gtk_menu_item_set_label( (GtkMenuItem *)glo->itbk, "-");
			}
		else	{		// ligne asm
			unsigned long long adr = glo->targ->asmstock[(unsigned int)ref].adr;
			glo->disa_sel_adr = adr;
			// on copie l'adresse en ascii dans le label de l'item !
			char tbuf[128]; const char * fmt;
			if	( glo->targ->is_break( adr ) )
				fmt = "Kill breakpoint at 0x" OPT_FMT;
			else	fmt = "Set breakpoint at 0x" OPT_FMT;
			snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)adr );
			gtk_menu_item_set_label( (GtkMenuItem *)glo->itbk, tbuf);
			}
		gtk_menu_popup( (GtkMenu *)glo->mdisa, NULL, NULL, NULL, NULL, event->button, event->time );
		}
	return TRUE;			/* we handled this */
	}
return FALSE;				/* we did not handle this */
}

void ram_call_fmt( GtkWidget *widget, glostru * glo )
{
if	( widget == glo->itram8 )
	glo->targ->option_ram_format = 8;
else if	( widget == glo->itram16 )
	glo->targ->option_ram_format = 16;
else if	( widget == glo->itram32 )
	glo->targ->option_ram_format = 32;
else if	( widget == glo->itram64 )
	glo->targ->option_ram_format = 64;
else if	( widget == glo->itram65 )
	glo->targ->option_ram_format = 65;
list_store_resize( glo->tmodm, glo->targ->get_ram_qlines(0) );
gtk_widget_queue_draw( glo->tlism );
}

void ram_call_copy( GtkWidget *widget, glostru * glo )
{
char text[128];
// glo->ram_sel_i a ete deja mis a jour par ram_right_call()
glo->targ->ram_val2txt( text, sizeof(text), 0, glo->ram_sel_i );
GtkClipboard * myclip;
myclip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
gtk_clipboard_set_text( myclip, text, -1);
}

void ram_call_copy_all( GtkWidget *widget, glostru * glo )
{
string s;
glo->targ->ram_all2string( &s, 0 );
GtkClipboard * myclip;
myclip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
gtk_clipboard_set_text( myclip, s.c_str(), -1);
}

// une call back pour le right-clic --> context menu
// le menu est deja cree (par mk_ram_menu), cette fonction le poppe apres avoir customise certains labels
gboolean ram_right_call( GtkWidget *curwidg, GdkEventButton *event, glostru * glo )
{
/* single click with the right mouse button? */
if	( ( event->type == GDK_BUTTON_PRESS ) && ( event->button == 3 ) )
	{	// tentative d'identifier la row :
        GtkTreePath *path;
        if	( gtk_tree_view_get_path_at_pos( GTK_TREE_VIEW(curwidg), event->x, event->y, &path, NULL, NULL, NULL) )
		{	// on a un path, on en extrait un indice
		unsigned int * indices;
		int depth;
		indices = (unsigned int *)gtk_tree_path_get_indices_with_depth( path, &depth );
		gtk_tree_path_free(path);
		glo->ram_sel_i = *indices;
		}
	gtk_menu_popup( (GtkMenu *)glo->mram, NULL, NULL, NULL, NULL, event->button, event->time );
	return TRUE;			/* we handled this */
	}
return FALSE;				/* we did not handle this */
}

/** ================= Gtk Tree View call backs ======================= */

void regname_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
unsigned int i;
const char * text;
const char * bgcolor;
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
// recuperer la donnee dans l'objet target
if	( i < glo->targ->regs.regs.size() )
	text = glo->targ->regs.regs[i].name.c_str();
else	text = "?";
g_object_set( rendy, "text", text, NULL );
if	( i == glo->targ->regs.iip ) bgcolor = IP_COLOR;
else if	( i == glo->targ->regs.isp ) bgcolor = SP_COLOR;
else if	( i == glo->targ->regs.ibp ) bgcolor = BP_COLOR;
else	bgcolor = "#E4E4E4";
g_object_set( rendy, "cell-background", bgcolor, "cell-background-set", TRUE, NULL );
}

void regval_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
unsigned int i;
int chng = 0;
char text[64]; const char * fmt;
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
// elaborer la donnee
#define MARKUP_REG	// 2 styles disponibles pour highlight du reg qui a change
if	( i < glo->targ->regs.regs.size() )
	{
	chng = glo->targ->regs.regs[i].changed;
	#ifdef MARKUP_REG
	fmt = "<span background=\"%s\">" OPT_FMT "</span>";
	snprintf( text, sizeof(text), fmt, chng?CHREG_COLOR:"#FFFFFF", (opt_type)glo->targ->regs.regs[i].val );
	#else
	fmt = OPT_FMT;
	snprintf( text, sizeof(text), fmt, (unsigned int)glo->targ->regs.regs[i].val );
	#endif
	}
else	snprintf( text, sizeof(text), "?" );
#ifdef MARKUP_REG
g_object_set( rendy, "markup", text, NULL );
#else
g_object_set( rendy, "text", text, NULL );
// la couleur ne revient pas toute seule au defaut (blanc)
g_object_set( rendy, "cell-background", chng?CHREG_COLOR:"#FFFFFF", "cell-background-set", TRUE, NULL );
#endif
}

void flag_data_call( GtkTreeViewColumn * tree_column,	// page 3-16 Vol. 1
                     GtkCellRenderer   * rendy,		// CF (bit 0) Carry flag
                     GtkTreeModel      * tree_model,	// PF (bit 2) Parity flag
                     GtkTreeIter       * iter,		// AF (bit 4) Auxiliary Carry flag
                     glostru *         glo )		// ZF (bit 6) Zero flag
{							// SF (bit 7) Sign flag
unsigned int i, reg, f=0;					// OF (bit 11) Overflow flag
char text[16]; const char * fmt;			// DF (bit 10) Direction lag
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
// elaborer la donnee
reg = (unsigned int)glo->targ->regs.get_eflags()->val;
switch	( i )
	{
	case 0 : fmt = "CF %c"; f = (reg & (1<<0))?'1':'0'; break;
	case 1 : fmt = "ZF %c"; f = (reg & (1<<6))?'1':'0'; break;
	case 2 : fmt = "SF %c"; f = (reg & (1<<7))?'1':'0'; break;
	case 3 : fmt = "OF %c"; f = (reg & (1<<11))?'1':'0'; break;
	case 4 : fmt = "PF %c"; f = (reg & (1<<2))?'1':'0'; break;
	case 5 : fmt = "AF %c"; f = (reg & (1<<4))?'1':'0'; break;
	case 6 : fmt = "DF %c"; f = (reg & (1<<10))?'1':'0'; break;
	default : fmt = "?";
	}
snprintf( text, sizeof(text), fmt, f );
g_object_set( rendy, "text", text, NULL );
}

// Autre style : 1 seule callback pour toutes les colonnes
// on peut identifier la treeview avec gtk_tree_view_column_get_tree_view()
// on peut identifier la colonne avec gtk_tree_view_column_get_title() si son nom est unique (FBI)
// ou juste avec tree_column si on l'a stocke, c'est plus correct
void disa_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
unsigned int i, pos;
int ref;
char text[128];
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
// recuperer la reference codee de ligne dans l'objet target
ref = glo->targ->get_disa_ref( glo->ilist, i );
// decoder
if	( ref < 0 )
	{		// ligne de code source
	unsigned int ilin = listing::decode_line_number(ref);
	unsigned int ifil = listing::decode_file_index(ref);
	if	( tree_column == glo->adrcol )
		{
		snprintf( text, sizeof(text), CSOURCE "  %4d</span>", ilin );
		g_object_set( rendy, "markup", text, NULL );
		}
	else if	( tree_column == glo->asmcol )
		{
		pos = snprintf( text, sizeof(text), CSOURCE );
		pos += markup_filter( text+pos, sizeof(text)-pos, glo->targ->get_src_line( ifil, ilin ) );
		if	( sizeof(text) > pos )
			snprintf( text+pos, sizeof(text)-pos, "</span>" );
		g_object_set( rendy, "markup", text, NULL );
		}
	else if	( tree_column == glo->bincol )
		{
		g_object_set( rendy, "text", "", NULL );
		}
	}
else	{		// ligne asm
	asmline * daline = &(glo->targ->asmstock[(unsigned int)ref]);
	if	( tree_column == glo->adrcol )
		{
		unsigned long long adr = daline->adr;
		const char * fmt;
		if	( adr == glo->targ->get_ip() )
			{
			if	( glo->targ->is_break( adr ) )
				fmt = MARGIN_BKIP ASMADDR OPT_FMT "</span>";
			else	fmt = MARGIN_IP ASMADDR OPT_FMT "</span>";
			}
		else	{
			if	( glo->targ->is_break( adr ) )
				fmt = MARGIN_BK ASMADDR OPT_FMT "</span>";
			else	fmt = MARGIN_NONE ASMADDR OPT_FMT "</span>";
			}
		snprintf( text, sizeof(text), fmt, (opt_type)adr );
		g_object_set( rendy, "markup", text, NULL );
		}
	else if	( tree_column == glo->asmcol )
		{
		g_object_set( rendy, "text", daline->asmsrc.c_str(), NULL );
		}
	else if	( ( glo->targ->option_binvis ) && ( tree_column == glo->bincol ) )
		{
		pos = snprintf( text, sizeof(text), BINCODE );
		pos += daline->bin2txt( text+pos, sizeof(text)-pos );
		if	( sizeof(text) > pos )
			snprintf( text+pos, sizeof(text)-pos, "</span>" );
		g_object_set( rendy, "markup", text, NULL );
		}
	}
}

void ram_data_call( GtkTreeViewColumn * tree_column,
                     GtkCellRenderer   * rendy,
                     GtkTreeModel      * tree_model,
                     GtkTreeIter       * iter,
                     glostru *         glo )
{
unsigned int i;
char text[128]; const char * fmt;
// recuperer notre index dans la colonne 0 du model
gtk_tree_model_get( tree_model, iter, 0, &i, -1 );
if	( tree_column == glo->madrcol )
	{
	unsigned long long adr = glo->targ->get_ram_adr( 0, i );
	if	( adr == glo->targ->get_sp() )
		fmt = MARGIN_SP OPT_FMT;
	else if	( adr == glo->targ->get_bp() )
		fmt = MARGIN_BP OPT_FMT;
	else	fmt = MARGIN_NOSP OPT_FMT;
	snprintf( text, sizeof(text), fmt, (opt_type)adr );
	g_object_set( rendy, "markup", text, NULL );
	}
else if	( tree_column == glo->mdatcol )
	{
	glo->targ->ram_val2txt( text, sizeof(text), 0, i );
	g_object_set( rendy, "text", text, NULL );
	}
}

/** =================== THE MAIN ======================= */

// the big storage of everything
static glostru theglo;
static daddy ledad;
static mi_parse lemipa;
static target latarget;
// pour exception gasp() de modpop2.c
// ceci cree bien le storage de la variable et l'exporte
// pour un programme C (a savoir modpop2.c) !!
extern "C" { GtkWindow * global_main_window; };

int main( int argc, char *argv[] )
{
#define glo (&theglo)

glo->dad = &ledad;
glo->mipa = &lemipa;
glo->targ = &latarget;
glo->ilist = 0;

setlocale( LC_ALL, "C" );	// question de survie

// option de CLI
glo->option_child_console = 1;
glo->option_flavor = 0;
glo->targ->option_binvis = 0;

#ifdef PRINT_64
glo->targ->regs.option_qregs = 18;
#else
glo->targ->regs.option_qregs = 10;
#endif

#ifdef PRINT_64
glo->option_ramblock = 256;
#else
glo->option_ramblock = 128;
#endif
glo->option_disablock = 256;
glo->option_toggles = 0;

for	( int iopt = 1; iopt < argc; ++iopt )
	{
	if	( argv[iopt][0] == '-' )
		{
		switch	( argv[iopt][1] )
			{
			case 't' : glo->option_toggles = atoi( argv[iopt]+2 );
				break;
			case 'c' : glo->option_child_console = atoi( argv[iopt]+2 );
				break;
			}
		}
	else	glo->targ->main_file_name = string( argv[iopt] );
	}

gtk_init(&argc,&argv);

// main window layout
mk_the_gui( glo );
global_main_window = GTK_WINDOW(glo->wmain);

// etat initial des toggles et radio-buttons
gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(glo->itram32), TRUE );	// defaut
glo->targ->option_ram_format = 32;							// defaut

if	( glo->option_toggles & 1 )
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( glo->btog1 ), TRUE );
if	( glo->option_toggles & 2 )
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( glo->btog2 ), TRUE );
if	( glo->option_toggles & 4 )
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( glo->btog3 ), TRUE );
if	( glo->option_toggles & 8 )
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( glo->btog4 ), TRUE );

// gtk_timeout_add( 31, (GtkFunction)(idle_call), (gpointer)glo );
glo->idle_id = g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );
// cet id servira pour deconnecter l'idle_call : g_source_remove( glo->idle_id );

char tbuf[128]; int retval;
snprintf( tbuf, sizeof(tbuf), "gdb --interpreter=mi %s", glo->targ->main_file_name.c_str() );
retval = glo->dad->start_child( tbuf );
if	( retval )
	glo->t.printf("Error daddy %d\n", retval );
else	init_step( glo );

// modpop( "test", "before gtk_main", GTK_WINDOW(glo->wmain) );

gtk_main(); // on va rester dans cette fonction jusqu'a ce qu'une callback appelle gtk_main_quit();
g_source_remove( glo->idle_id );
// gtk_widget_destroy( glo->wmain );
printf("clean exit :-)\n");
return 0;
}
