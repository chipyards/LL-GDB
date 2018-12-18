#include <stdio.h>

using namespace std;
#include <string>
#include <vector>

#include "mi_parse.h"
//#define PIPO
int main( int argc, char ** argv )
{
mi_parse lemipa;
#define mipa (&lemipa)
char tbuf[1<<18];
int c, retval;
unsigned int i, curlin = 1;

while	( fgets( tbuf, sizeof(tbuf), stdin ) )
	{
	#ifdef PIPO
	printf("!%s!\n", tbuf );
	#else
	mipa->e = 1; i = 0; 
	do	{
		c = tbuf[i++];
		if	( i >= sizeof(tbuf) )
			{
			printf("fin de buffer !!!\n");
			c = 0;
			}
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
			case 6 : printf("fin report court %s\n", mipa->nam.c_str() ); break;
			case 7 : printf("fin report %s\n",
				 mipa->stac.back().nam.c_str() ); break;
			case 8 : printf("stream %c %s\n", 
				 mipa->nam.c_str()[0], mipa->val.substr(0,72).c_str() ); break;
			default : if	( retval < 0 )
					{
					printf("err %d, char %d, line %d\n",
						-retval, i, curlin );
					return 1;
					}
			}
		} while ( c );
	#endif
	++curlin;
	}

return 0;
}
