using namespace std;
#include <string>
#include <vector>
#include <map>

#include "target.h"

// retourne index du listing ou -1 si echec (asm non dispo)
int target::add_listing( unsigned long long adr )
{
if	( asmmap.count(adr) == 0 )
	return -1;
unsigned int ia, delta;
int src0, srci;
asmline * daline;
// creer listing vierge
listing newlist;
listing * curlist;
liststock.push_back( newlist );
curlist = &(liststock.back());
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
return liststock.size() - 1;
}

void target::dump_listing( unsigned int ilist )
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
