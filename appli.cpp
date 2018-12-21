#include <stdio.h>

using namespace std;
#include <string>
#include <vector>

#include "mi_parse.h"

#include <windows.h>
#include "spawn_w.h"

void usage()
{
printf("usage : appli d <cmd> : test spawn, process generique avec 2 pipes\n"
       "        appli m <fil> : test parseur MI avec un fichier texte\n"
       "        appli g <target> : test spawn gdb MI avec parseur\n" );
}

// test 2 pipes avec gdb en mode mi
int test_mi( const char * cmd )
{
daddy ledad;
#define dad (&ledad)
mi_parse lemipa;
#define mipa (&lemipa)
int retval;
char tbuf[1024];
int d;

snprintf( tbuf, sizeof(tbuf), "gdb --interpreter=mi %s", cmd );

retval = dad->start_child( tbuf );
if	( retval )
	printf("error daddy %d\n", retval );

while	( fgets( tbuf, sizeof(tbuf), stdin ) )
	{
	if	( tbuf[0] == 'Q' )
		break;
	if	( tbuf[0] > ' ' )
		dad->send_cmd(tbuf);
	while	( ( d = dad->child_getc() ) >= 0 )
		{
		retval = mipa->proc1char( d );
		if	( retval == 0 )
			continue;
		else if	( retval < 0 )
			{
			printf("err %d\n", -retval );
			return 1;
			}
		else	{
			if	( mipa->dump( retval, tbuf, sizeof(tbuf) ) )
				printf("%s\n", tbuf );
			}
		}
	fflush(stdout);
	}
return 0;
}



// test 2 pipes avec un child process quelconque
int test_daddy( const char * cmd )
{
daddy ledad;
#define dad (&ledad)
int retval;
char tbuf[1024];
int d;

retval = dad->start_child( (char*)cmd );
if	( retval )
	printf("error daddy %d\n", retval );

while	( fgets( tbuf, sizeof(tbuf), stdin ) )
	{
	if	( tbuf[0] == 'Q' )
		break;
	if	( tbuf[0] )
		dad->send_cmd(tbuf);
	while	( ( d = dad->child_getc() ) >= 0 )
		{
		putc( d, stdout );
		}
	fflush(stdout);
	}
return 0;
}

// test du parseur de MI, sans child process, utilise un fichier texte ou stdin
int test_parseur( const char * fnam )
{
FILE * fil;
mi_parse lemipa;
#define mipa (&lemipa)

char tbuf[1<<18];
char dbuf[256];
int c, retval;
unsigned int i, curlin = 1;

if	( fnam )
	{
	fil = fopen( fnam, "r" );
	if	( fil == NULL )
		return 1;
	}
else	fil = stdin;

while	( fgets( tbuf, sizeof(tbuf), fil ) )
	{
	mipa->e = 1; i = 0;
	do	{
		c = tbuf[i++];
		if	( i >= sizeof(tbuf) )
			{
			printf("!!! buffer plein !!!\n");
			c = 0;
			}
		retval = mipa->proc1char( c );
		if	( retval == 0 )
			continue;
		else if	( retval < 0 )
			{
			printf("err %d, char %d, line %d\n",
				-retval, i, curlin );
			return 1;
			}
		else	{
			if	( mipa->dump( retval, dbuf, sizeof(dbuf) ) )
				printf("%s\n", dbuf );
			}
		} while ( c );
	++curlin;
	if	( fil != stdin )
		getchar();
	}

return 0;
}

int main( int argc, char ** argv )
{
if	( argc < 2 )
	{ usage(); return 1; }

switch	( argv[1][0] )
	{
	case 'd' : test_daddy( argv[2] ); break;
	case 'm' : test_parseur( argv[2] ); break;
	case 'g' : test_mi( argv[2] ); break;
	default: usage();
	}
return 0;
}

