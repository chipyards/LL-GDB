#include <stdio.h>

using namespace std;
#include <string>
#include <vector>

#include "mi_parse.h"

int main( int argc, char ** argv )
{
mi_parse lemipa;
#define mipa (&lemipa)
char tbuf[4096];
int i, c, retval;

while	( fgets( tbuf, sizeof(tbuf), stdin ) )
	{
	mipa->e = 1; i = 0;
	do	{
		c = tbuf[i];
		retval = mipa->proc1char( c );
		if	( retval == 0 )
			continue;
		/*	0 : R.A.S.
			1 : fin de valeur
			2 : debut de conteneur nomme
			3 : debut de conteneur anonyme
			4 : fin de conteneur nomme
			5 : fin de conteneur anonyme
			6 : fin de conteneur racine vide
			7 : fin de conteneur racine
		*/
		switch	( retval )
			{
			case 1 : printf("%s = \"%s\"\n", mipa->nam.c_str(), mipa->val.c_str() ); break;
			case 2 : printf("debut conteneur %s %c\n",
				 mipa->stac.back().nam.c_str(), mipa->stac.back().type ); break;
			case 4 : printf("fin conteneur %s !%c\n",
				 mipa->stac.back().nam.c_str(), mipa->stac.back().type ); break;
			case 6 :
			case 7 : printf("fin report %s\n",
				 mipa->stac.back().nam.c_str() ); break;
			default : if	( retval < 0 )
					printf("err %d, char %d, line %d\n",
						-retval, mipa->curchar - 1, mipa->curlin );
			}
		} while ( c );
	}





return 0;
}
