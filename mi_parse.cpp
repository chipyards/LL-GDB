/* parseur de donnees MI produites par GDB
   - maintient une pile de l'ascendance de l'element courant
 */

using namespace std;

#include <string>

#include <vector>
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

	6 : fin de conteneur racine vide
	7 : fin de conteneur racine

	8 : fin de stream-output
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
	{		// les conteneurs hierarchises (kinda serialized objects)
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
			stac.push_back( conteneur( nam, c ) );	// conteneur racine
			e = 10;
			}
		else if	( c < ' ' )
			{
			e = 1;
			return( 6 );		// fin de report court, le nom est dans nam
			}
		else	{
			nam += char(c);
			}			break;
	case 10: if	( ( c == '{' ) || ( c == '[' ) )
			{
			stac.push_back( conteneur( "", c ) );	// debut conteneur anonyme
			e = 10;
			return( 3 );
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
				return( 7 );	// fin de conteneur racine
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
			return( 2 );
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
				return( 7 );	// fin de conteneur racine
				}
			else	return(-66633);	// fin de ligne inattendue
			}
		else	return(-6665);		// garbage apres conteneur
						break;
	case 70 : if	( c < ' ' )
			{
			e = 1;		// rien a depiler
			return( 8 );	// fin de stream, le contenu est dans val
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
