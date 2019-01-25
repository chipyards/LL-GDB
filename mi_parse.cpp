/* parseur de donnees MI produites par GDB
   - maintient une pile de l'ascendance de l'element courant
 */
#include <stdio.h>
using namespace std;
#include <string>
#include <vector>
#include <map>

#include "target.h"
#include "mi_parse.h"

/*
etats :

<0 : vu fin de conteneur, depilage immediat requis puis aller en (-e)
 1 : debut de ligne, attendre ^, etc...
 2 : vu ^, attendre texte ou ',' ou fin de ligne
 10 : a l'interieur d'un conteneur, attendre texte, quoted-string ou autre conteneur ou fin de conteneur
 11 : vu 1er char d'un nom, attendre la suite ou '='
 12 : vu '=', attendre valeur i.e. " ou conteneur i.e. '[' ou '{'
 13 : vu '"', attendre char ou '"' ou '\'
 14 : vu '\', attendre char
 15 : vu '"' (fin de valeur) ou fin de conteneur, attendre ',' ou fin de conteneur
 70 : vu '~', stream-output, attendre texte ou fin de ligne
 74 : vu '\', attendre char ou fin de ligne

N.B. 15 precede 10 sauf au debut du conteneur

valeurs de retour :

	0 : R.A.S.
	1 : fin de valeur

	2 : debut de conteneur nomme
	3 : debut de conteneur anonyme

	4 : fin de conteneur nomme
	5 : fin de conteneur anonyme

	6 : debut de conteneur report
	7 : debut et fin de conteneur report vide
	8 : fin de conteneur report

	9 : fin de stream-output
*/

// cette fonction parse un caractere et rend zero sauf si on a une fin de valeur
// si la valeur de retour est >= 0, le processus peut continuer (e est a jour)
int mi_parse::proc1char( int c )
{
if	( e < 0 )	// depilage eventuel avant lecture char
	{
	if	( stac.size() )
		stac.pop_back();
	else	return(-6661);		// depilage impossible
	e = -e;
	}
if	( c < ' ' )
	{	// ci-dessous: liste des etats gerant eux-memes la fin de ligne
	if	( ( e != 1 ) && ( e != 2 ) && ( e != 10 ) && ( e != 15 ) && ( e != 70 ) )
		return(-66631);		// fin de ligne inattendue
	}
switch	( e )
	{		// les reports hierarchises (kinda serialized objects)
			//     reply           status      exec status      notif
	case 1:	if	( ( c == '^' ) || ( c == '+' ) || ( c == '*' ) || ( c == '=' ) )
			{
			e = 2;
			nam = char(c);	// le '^' est inclus dans le nom de conteneur
			}
			// les streams 'human readable'
			//     console		log	   target out	    (gdb) prompt    (echo cmd)
		else if	( ( c == '~' ) || ( c == '&' ) || ( c == '@' ) || ( c == '(' ) || ( c == '-' ) )
			{
			e = 70;		// raw stream
			nam = char(c); val = ""; // le nom du stream est simplement ~, & ou @
			}
		else if ( c > ' ' )	// accepter les residus de newline
			{
			return(-6662);	// debut de report illegal
			}			break;
	case 2:	if	( c == ',' )
			{
			stac.push_back( conteneur( nam, c ) );
			e = 10;
			return( 6 );	// debut conteneur report, le nom est dans nam
			}
		else if	( c < ' ' )
			{
			e = 1;
			return( 7 );	// debut et fin de report court, le nom est dans nam
			}
		else	{
			nam += char(c);
			}			break;
	case 10: if	( ( c == '{' ) || ( c == '[' ) )
			{
			stac.push_back( conteneur( "", c ) );
			e = 10;
			return( 3 );	// debut conteneur anonyme
			}
		else if ( ( c == '}' ) || ( c == ']' ) )
			{		// cet etat -15 sert a differer le depilage du conteneur avant etat 15
			e = -15; 	// afin que celui-ci puisse etre lu juste apres ce return
			if	( stac.back().nam.size() )
				return( 4 );	// fin de conteneur nomme
			else	return( 5 );	// fin de conteneur anonyme
			}
		else if	( c == '"' )
			{
			val = ""; nam = "";	// debut de valeur anonyme
			e = 13;
			}
		else if	( c < ' ' )
			{
			c = stac.back().nam[0];
			if	( ( c == '^' ) || ( c == '+' ) || ( c == '*' ) || ( c == '=' ) )
				{
				e = -1;
				return( 8 );	// fin de conteneur report
				}
			else	return(-66632);	// fin de ligne inattendue
			}
		else	{
			nam = char(c);
			e = 11;
			}			break;
	case 11: if	( c == '=' )
			{
			val = "";
			e = 12;
			}
		else	{
			nam += char(c);
			}			break;
	case 12: if	( ( c == '{' ) || ( c == '[' ) )
			{
			stac.push_back( conteneur( nam, c ) );	// debut conteneur
			e = 10;
			return( 2 );	// debut conteneur
			}
		else if	( c == '"' )
			{
			val = "";
			e = 13;		// debut de valeur simple
			}
		else	return(-6664);		// signe egal mal suivi
						break;
	case 13: if	( c == '\\' )
			e = 14;
		else if	( c == '"' )
			{
			e = 15;
			return( 1 );		// fin de valeur simple
			}
		else	{
			val += char(c);
			}			break;
	case 14:	{
			e = 13;
			val += char(c);
			}			break;
	case 15: if	( c == ',' )
			{
			e = 10;
			}
		else if ( ( c == '}' ) || ( c == ']' ) )
			{		// cet etat -15 sert a differer le depilage du conteneur avant etat 15
			e = -15; 	// afin que celui-ci puisse etre lu juste apres ce return
			if	( stac.back().nam.size() )
				return( 4 );	// fin de conteneur nomme
			else	return( 5 );	// fin de conteneur anonyme
			}
		else if	( c < ' ' )
			{
			c = stac.back().nam[0];
			if	( ( c == '^' ) || ( c == '+' ) || ( c == '*' ) || ( c == '=' ) )
				{
				e = -1;
				return( 8 );	// fin de conteneur report
				}
			else	return(-66633);	// fin de ligne inattendue
			}
		else	return(-6665);		// garbage apres conteneur
						break;
	case 70 : if	( c < ' ' )
			{
			e = 1;		// rien a depiler
			return( 9 );	// fin de stream, le contenu est dans val
			}
		else if ( c == '\\' )
			{
			e = 74;
			}
		else if	( c != '"' )
			{
			val += char(c);
			}			break;
	case 74 : if	( c == 'n' )
			val += char(10);
		else if ( c == 't' )
			val += char(8);
		else	val += char(c);
		e = 70;				break;
	}
return 0;
}

// formatter un dump texte indente, en fonction de la valeur retournee par mi_parse::proc1char()
// seulement si celle-ci est positive - retourne le nombre de chars mis dans buf
int mi_parse::dump( int retval, char * buf, int size )
{
int pos = 0;
static int level = 0;
switch	( retval )
	{
	case 1: if	( level > 0 )
			pos += snprintf( buf, size,  "%*s", level*3, " " );	// indent!
		if	( nam.size() )
			pos += snprintf( buf+pos, size, "%s = \"%s\"", nam.c_str(), val.c_str() );
		else	pos += snprintf( buf+pos, size, "\"%s\"", val.c_str() );
		break;
	case 2: if	( level > 0 )
			pos += snprintf( buf, size,  "%*s", level*3, " " );	// indent!
		// pos += snprintf( buf+pos, size, "debut conteneur %s %c",
		pos += snprintf( buf+pos, size, "conteneur %s %c",
				 stac.back().nam.c_str(), stac.back().type );
		++level;
		break;
	case 3: ++level;
		break;
	case 4: --level;
		if	( level > 0)
			pos += snprintf( buf, size,  "%*s", level*3, " " );	// indent!
		// pos += snprintf( buf+pos, size, "fin conteneur %s !%c",
		//	stac.back().nam.c_str(), stac.back().type );
		// un hack bien sale : '{' + 2 donne '}', '[' + 2 donne ']'
		pos += snprintf( buf+pos, size, "%c", stac.back().type + 2 );
		break;
	case 5: --level;
		break;
	case 6: pos += snprintf( buf, size, "debut report %s", nam.c_str() );
		level = 1;
		break;
	case 7: pos += snprintf( buf, size, "report court %s", nam.c_str() );
		level = 0;
		break;
	case 8: pos += snprintf( buf, size, "fin report %s", stac.back().nam.c_str() );
		level = 0;
		break;
	case 9: if	( nam.c_str()[0] == '(' )
			pos += snprintf( buf, size, "(%s",  val.c_str() );
		else	pos += snprintf( buf, size, "stream %c%s", nam.c_str()[0], val.substr(0,72).c_str() );
		break;
	}
return pos;
}

// extraire et ranger les donnees dans la target, en fonction de la valeur retournee par mi_parse::proc1char()
	/*	0 : R.A.S.
		1 : fin de valeur
		2 : debut de conteneur nomme
		3 : debut de conteneur anonyme
		4 : fin de conteneur nomme
		5 : fin de conteneur anonyme
		6 : debut de conteneur report
		7 : fin de conteneur report vide
		8 : fin de conteneur report
		9 : fin de stream-output
	*/
int mi_parse::extract( int retval, target * targ )
{
static asmline curasm;
static string relfile;
static string absfile;
static int curline0 = -1;
static int curline1;

int ss = stac.size();
switch	( retval )
	{
	case 1: if	( ( ss ) && ( stac.back().nam == string("register-names") ) )	// short circuit eval ;-)
			targ->regs.add_reg_name( val );
		else if	( ( ss ) && ( stac.back().nam == string("src_and_asm_line") ) )
			{
			if	( nam == string("line") )
				{
				if	( curline0 < 0 )
					curline0 = curline1 = strtoul( val.c_str(), NULL, 10 );
				else	curline1 = strtoul( val.c_str(), NULL, 10 );
				}
			else if	( nam == string("file") )
				relfile = val;
			else if	( nam == string("fullname") )
				absfile = val;
			}
		else if	( ( ss >= 2 ) && ( stac[stac.size()-2].nam == string("register-values") ) )
			{
			if	( nam == string("number") ) targ->regs.set_reg_pos( val );
			else if	( nam == string("value") )  targ->regs.set_reg_val( val );
			}
		else if	( ( ss >= 2 ) && (
				( stac[stac.size()-2].nam == string("line_asm_insn") ) ||
				( stac[stac.size()-2].nam == string("asm_insns") )
				)
			)
			{
			if	( nam == string("address") ) curasm.set_adr( val );
			else if	( nam == string("opcodes") ) curasm.count_the_bytes( val );
			else if	( nam == string("inst") ) curasm.asmsrc = val;
			}
		break;
	case 2: if	( ( ss ) && ( stac.back().nam == string("register-names") ) )
			targ->regs.start_reg_names();
		break;
	case 3: if	( ( ss >= 2 ) && (
				( stac[stac.size()-2].nam == string("line_asm_insn") ) ||
				( stac[stac.size()-2].nam == string("asm_insns") )
				)
			)
			curasm.init();
		break;
	case 4: break;
	case 5: if	( ( ss >= 2 ) && (
				( stac[stac.size()-2].nam == string("line_asm_insn") ) ||
				( stac[stac.size()-2].nam == string("asm_insns") )
				)
			)
			{
			if	( targ->asmmap.count(curasm.adr) )
				{
				}
			else	{
				if	( stac[stac.size()-2].nam == string("line_asm_insn") )
					{
					curasm.src0 = curline0;
					curasm.src1 = curline1;
					curline0 = -1;
					if	( targ->filemap.count(relfile) == 0 )
						{
						srcfile curfile;
						targ->filemap[relfile] = targ->filestock.size();
						targ->filestock.push_back( curfile );
						targ->filestock.back().relpath = relfile;
						targ->filestock.back().abspath = absfile;
						if	( targ->filestock.back().status == 0 )
							targ->filestock.back().readfile();
						}
					curasm.isrc = targ->filemap[relfile];
					}
				targ->asmmap[curasm.adr] = targ->asmstock.size();
				targ->asmstock.push_back( curasm );
				}
			}
		break;
	case 6: break;
	case 7: break;
	case 8: break;
	case 9: break;
	}
return 0;
}
