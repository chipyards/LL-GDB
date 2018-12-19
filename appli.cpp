#include <stdio.h>

using namespace std;
#include <string>
#include <vector>

#include "mi_parse.h"
//#define PIPO
int main( int argc, char ** argv )
{
FILE * fil;
mi_parse lemipa;
#define mipa (&lemipa)
char tbuf[1<<18];
int c, retval, level = 0;
unsigned int i, curlin = 1;

if	( argc > 1 )
	{
	fil = fopen( argv[1], "r" );
	if	( fil == NULL )
		return 1;
	}
else	fil = stdin;

while	( fgets( tbuf, sizeof(tbuf), fil ) )
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
			6 : debut de conteneur report
			7 : fin de conteneur report vide
			8 : fin de conteneur report
			9 : fin de stream-output
		*/
		switch	( retval )
			{
			case 1: if	( level > 0 )
					printf( "%*s", level*3, " " );	// indent!
				if	( mipa->nam.size() )	
					printf("%s = \"%s\"\n", mipa->nam.c_str(), mipa->val.c_str() );
				else	printf("\"%s\"\n", mipa->val.c_str() );
				break;
			case 2: if	( level > 0 )
					printf( "%*s", level*3, " " );	// indent! 
				printf("debut conteneur %s %c\n",
				mipa->stac.back().nam.c_str(), mipa->stac.back().type );
				++level;
				break;
			case 3: ++level;
				break;
			case 4: --level;
				if	( level > 0)
					printf( "%*s", level*3, " " );	// indent!
				printf("fin conteneur %s !%c\n",
					mipa->stac.back().nam.c_str(), mipa->stac.back().type );
				break;
			case 5: --level;
				break;
			case 6: printf("debut report %s\n", mipa->nam.c_str() );
				++level;
				break;
			case 7: printf("report court %s\n", mipa->nam.c_str() );
				break;
			case 8: --level;
				printf("fin report %s\n", mipa->stac.back().nam.c_str() );
				break;
			case 9: if	( mipa->nam.c_str()[0] == '(' )
					printf("(%s\n",  mipa->val.c_str() );
				// else	printf("stream %c%s\n", mipa->nam.c_str()[0], mipa->val.substr(0,72).c_str() );
				break;
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
	if	( fil != stdin )
		getchar();
	}

return 0;
}
