#include <stdio.h>

#include "futf8.h"

void u8filtre::putc( char c )
{
switch ( c & 0xC0 )
   {
   case 0x00 :
   case 0x40 :	if	( expect )
			{	// invalide, vider le fifo
			wi = 0; ri = 0; expect = 0;
				// mettre le WARNCHAR
			fifo[wi++] = WARNCHAR; errcnt++;
			}
		else	{	// ascii ordinaire
			wi = 0; ri = 0; fifo[wi++] = c;
			}
		break;

   case 0x80 :	if	( expect )
			{	// continuation multibyte
			fifo[wi++] = c; --expect;
			if	( expect == 0 )
				{		// decoder
				int unicode;
				if	( wi == 2 )
					{
					unicode = ( ( fifo[0] & 0x1F ) << 6 ) | ( fifo[1] & 0x3F );
					if	( unicode <= 0x7F )
						{	// trop petit pour 2 bytes
						wi = 0; ri = 0; expect = 0;
						fifo[wi++] = WARNCHAR; errcnt++;
						}
					}
				else if ( wi == 3 )
					{
					unicode = ( ( fifo[0] & 0x0F ) << 12 ) |
						  ( ( fifo[1] & 0x3F ) << 6  ) |
						    ( fifo[2] & 0x3F );
					if	(
						( unicode <= 0x07FF ) || 	// trop petit pour 3 bytes
						( unicode >= 0xD800 )		// surrogate, private, etc...
						)
						{
						wi = 0; ri = 0; expect = 0;
						fifo[wi++] = WARNCHAR; errcnt++;
						}
					}
				else	{	// on se limite a 3 bytes <==> 16 bits
					wi = 0; ri = 0; expect = 0;
					fifo[wi++] = WARNCHAR; errcnt++;
					}
				}
			}
		else	{	// invalide, vider le fifo
			wi = 0; ri = 0; expect = 0;
				// mettre le WARNCHAR
			fifo[wi++] = WARNCHAR; errcnt++;
			}
		 break;
   case 0xC0 :	if	( expect )
			{	// invalide, vider le fifo
			wi = 0; ri = 0; expect = 0;
				// mettre le WARNCHAR
			fifo[wi++] = WARNCHAR; errcnt++;
			}
		else	{	// debut multibyte
			wi = 0; ri = 0;
			fifo[wi++] = c;
			expect = 1;
			if	( c & 0x20 )
				{
				expect = 2;
				if	( c & 0x10 )
					{
					// expect = 3;	// on se limite a 3 bytes <==> 16 bits
					wi = 0; ri = 0; expect = 0;
					fifo[wi++] = WARNCHAR; errcnt++;
					}
				}
			}
		break;
   }
// printf("c=%c wi=%d ri=%d e=%d\n", c, wi, ri, expect );
}

int u8filtre::avail()
{
if	( ( wi > ri ) && ( expect == 0 ) )
	return( wi - ri );
else	return 0;
}

char u8filtre::getc()
{
if	( ( wi > ri ) && ( expect == 0 ) )
	{
	// printf("%c <--\n", fifo[ri] );
	return( fifo[ri++] );
	}
else	{
	return 0;
	}
}
