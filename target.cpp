using namespace std;
#include <string>
#include <vector>
#include <map>

#include <string.h>
#include "target.h"

// remplir un listing a partir de l'adresse donnee, jusqu'a epuisement du disass
// le contenu anterieur est ecrase
// si le disass n'est pas dispo le listing est laisse vide
// si ilist ne pointe pas sur un listing, retour -1
int target::fill_listing( unsigned int ilist, unsigned long long adr )
{
if	( ilist >= liststock.size() )
	return -1;
unsigned int ia, delta;
int src0, srci;
asmline * daline;
listing * curlist = &(liststock[ilist]);
curlist->lines.clear();
curlist->adr0 = adr;
while	( asmmap.count(adr) )
	{
	// lire la ligne desassemblee
	ia = asmmap[adr];
	daline = &(asmstock[ia]);
	delta = daline->qbytes;
	if	( delta == 0 )
		break;		// ligne invalide, impossible continuer
	// traiter eventuelles lignes de src
	src0 = daline->src0;
	if	( src0 >= 0 )
		{
		for	( srci = src0; srci <= daline->src1; ++srci )
			{
			curlist->lines.push_back( listing::encode_ref( daline->isrc, (unsigned int)srci ) );
			}
		}
	// completer listing avec la ligne asm
	curlist->lines.push_back((int)ia);
	// passer a l'instruction suivante
	adr += delta;
	}
// le listing est fini
curlist->adr1 = adr;
return 0;
}

// retourne index du listing ou -1 si echec
int target::add_listing( unsigned long long adr )		// not tested
{
if	( asmmap.count(adr) == 0 )
	return -1;
// creer listing vierge
listing newlist;
liststock.push_back( newlist );
unsigned int ilist = liststock.size() - 1;
fill_listing( ilist, adr );
return (int)ilist;
}

void target::dump_listing( unsigned int ilist )	// unsecure !!!
{
unsigned int i, ifil, ilin;
int ref;
asmline * daline;
printf("listing %u\n", ilist );
for	( i = 0; i < liststock[ilist].lines.size(); ++i )
	{
	ref = liststock[ilist].lines[i];
	if	( ref < 0 )
		{		// ligne de code source
		ifil = listing::decode_file_index(ref);
		ilin = listing::decode_line_number(ref);
		printf("%-8d %s\n", ilin, filestock[ifil].relpath.c_str() );
		}
	else	{		// ligne asm
		daline = &(asmstock[(unsigned int)ref]);
		// daline->dump();	// ce dump envoie aussi un resume des lignes de src
		printf("%08X %u %s\n", (unsigned int)daline->adr, daline->qbytes, daline->asmsrc.c_str() );
		}
	}
}

// chercher un index dans le listing (-1 si echec)
// on cherche soit :
//	une ligne asm de asmstock dont on a eventuellement trouve l'index dans asmmap
//	une ligne de code source qu'on a trouve dans un fichier source
// recherche par brute-force car on ne peut pas emettre d'hypothese sur l'ordre des index
int listing::search_line( int index, unsigned int hint )
{
unsigned int i;
if	( hint >= lines.size() )
	hint = 0;
for	( i = hint; i < lines.size(); ++i )
	{
	if	( lines[i] == index )
		return (int)i;
	}
for	( i = 0; i < hint; ++i )
	{
	if	( lines[i] == index )
		return (int)i;
	}
return -1;
}

// n'appeler cette methode que si status == 0
void srcfile::readfile()
{
FILE * fil;
fil = fopen( relpath.c_str(), "r" );
if	( fil == NULL )
	{
	fil = fopen( abspath.c_str(), "r" );
	if	( fil == NULL )
		status = -1;
	}
if	( fil )
	{
	char lbuf[256]; unsigned int pos;
	while	( fgets( lbuf, sizeof( lbuf ), fil ) )
		{
		pos = strlen( lbuf ) - 1;	// enlever line end et trailing blanc
		while	( ( pos >= 0 ) && ( lbuf[pos] <= ' ' ) )
			lbuf[pos--] = 0;
		lines.push_back( string( lbuf ) );
		}
	fclose( fil );
	status = 1;
	}
}
