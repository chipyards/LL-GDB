using namespace std;
#include <string>
#include <vector>
#include <map>

#include <string.h>
#include "arch_type.h"
#include "target.h"
#include "futf8.h"

// remplir un listing a partir de l'adresse donnee, jusqu'a epuisement du disass
// le contenu anterieur est ecrase
// si le disass n'est pas dispo le listing est laisse vide
// si ilist ne pointe pas sur un listing, retour -1
int target::fill_listing( unsigned int ilist, unsigned long long adr )
{
if	( ilist >= liststock.size() )
	return -1;
// premiere etape : essayer de remonter l'adresse de depart autant que possible
// printf( "avant : " OPT_FMT "\n", (opt_type)adr ); fflush(stdout);
unsigned int ia, delta;
unsigned long long prevadr;
map<unsigned long long, unsigned int>::iterator asmiter = asmmap.find( adr );
do	{
	--asmiter;	// remonter l'iterateur
	prevadr = asmiter->first;
	ia = asmiter->second;
	if	( ( ia == 0 ) || ( ia >= asmstock.size() ) )
		break;
	// verification de continuite de la sequence
	delta = asmstock[ia].qbytes;
	if	( ( prevadr + delta ) != adr )
		break;
	// ici c'est ok
	adr = prevadr;
	} while ( ia );
// printf( "apres : " OPT_FMT "\n", (opt_type)adr ); fflush(stdout);
// deuxieme étape : creer le listing from scratch
int src0;		// first source line number, or -1 if none
int srci;		// current source line number
unsigned int isrcf;	// index of src file in filestock
asmline * daline;
listing * curlist = &(liststock[ilist]);
curlist->lines.clear();
curlist->adr0 = adr;
while	( asmmap.count(adr) )
	{
	// lire la ligne desassemblee
	ia = asmmap[adr];
	if	( ( ia == 0 ) || ( ia >= asmstock.size() ) )
		break;
	daline = &(asmstock[ia]);
	delta = daline->qbytes;
	if	( delta == 0 )
		break;		// ligne invalide, impossible continuer
	// traiter eventuelles lignes de src
	src0 = daline->src0;
	if	( src0 >= 0 )
		{
		isrcf = daline->isrc;
		if	( ( filestock[isrcf].status > 0 ) || ( option_unreach_path ) )
			{
			for	( srci = src0; srci <= daline->src1; ++srci )
				curlist->lines.push_back( listing::encode_ref( isrcf, (unsigned int)srci ) );
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

/*
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
} */

void regbank::reg_all2string( string * s )
{
unsigned int ireg;
char tbuf[256]; const char * fmt = " " OPT_FMT;
for	( ireg = 0; ireg < option_qregs; ++ireg )
	{
	*s += regs[ireg].name;
	snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)regs[ireg].val );
	*s += string(tbuf);
	*s += string("\n");
	}
}

void target::disa_all2string( string * s, unsigned int ilist )
{
unsigned int i, qi, qbmax, ifil, ilin, len;
int ref; char tbuf[256]; const char * fmt;
if	( ilist >= liststock.size() )
	return;
listing * list = &liststock[ilist];
qi = liststock[ilist].lines.size();
qbmax = 1;
if	( option_binvis )	// preliminaire : recherche du max de qbytes pour tabulation
	{
	for	( i = 0; i < qi; ++i )
		{
		ref = list->lines[i];
		if	( ref > 0 )
			{
			if	( asmstock[(unsigned int)ref].qbytes > qbmax )
				qbmax = asmstock[(unsigned int)ref].qbytes;
			}
		}
	}
for	( i = 0; i < qi; ++i )	// boucle principale
	{
	ref = list->lines[i];
	if	( ref < 0 )
		{		// ligne de code source
		ilin = listing::decode_line_number(ref);
		ifil = listing::decode_file_index(ref);
		// numero de ligne (tenant lieu d'adresse)
		#ifdef	PRINT_64
		fmt = "%4d             ";
		#else
		fmt = "%4d     ";
		#endif
		snprintf( tbuf, sizeof(tbuf), fmt, ilin );
		*s += string(tbuf);
		// tabulation supplementaire
		if	( option_binvis )
			*s += string("  ");
		// code source
		*s += string( get_src_line( ifil, ilin ) );
		}
	else	{		// ligne de desassembly
		asmline * daline = &(asmstock[(unsigned int)ref]);
		// adresse
		unsigned long long adr = daline->adr;
		fmt = OPT_FMT " ";
		snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)adr );
		*s += string(tbuf);
		// bin code (optionnel)
		if	( ( option_binvis ) && ( sizeof(tbuf) > (qbmax*3) ) )
			{
			len = daline->bin2txt( tbuf, sizeof(tbuf) );
			if	( len < qbmax * 3 )
				memset( tbuf+len, ' ', (qbmax*3) - len );
			tbuf[qbmax*3] = 0;
			*s += string(tbuf);
			}
		// desassembled code
		*s += daline->asmsrc;
		}
	*s += string("\n");
	}
}

// formatte une valeur lue dans une RAM
int target::ram_val2txt( char * text, unsigned int size, unsigned int iram, unsigned int iline ) {
const char * fmt;
if	( iram >= ramstock.size() )
	return snprintf( text, size, "no data" );
switch	( option_ram_format )
	{
	case 64 :
		if	( (1+(2*iline)) < ramstock[iram].w32.size() )
			{
			fmt = "%08X%08X";
			return snprintf( text, size, fmt, ramstock[iram].w32[1+(2*iline)], ramstock[iram].w32[2*iline] );
			}
		else	return snprintf( text, size, "no data" );
		break;
	case 65 :	// 64 bits, display ascii 8 chars
		if	( (1+(2*iline)) < ramstock[iram].w32.size() )
			{
			unsigned char * b; unsigned int i;
			b = (unsigned char *)&(ramstock[iram].w32[2*iline]);
			for	( i = 0; (i < 8) && (i < size); ++i )
				{
				text[i] = b[i];
				if	( ( text[i] < ' ' ) || ( text[i] > '~' ) )
					text[i] = ' ';
				}
			if	( i >= size )
				i = size-1;
			text[i] = 0;
			return i+1;
			}
		else	return snprintf( text, size, "no data" );
		break;
	case 8 :
		if	( iline < ramstock[iram].w32.size() )
			{
			unsigned char * bytes = (unsigned char *)&(ramstock[iram].w32[iline]);
			fmt = "%02X %02X %02X %02X";
			return snprintf( text, size, fmt, bytes[0], bytes[1], bytes[2], bytes[3] );
			}
		else	return snprintf( text, size, "no data" );
		break;
	case 16 :
		if	( iline < ramstock[iram].w32.size() )
			{
			unsigned short * shorts = (unsigned short *)&(ramstock[iram].w32[iline]);
			fmt = "%04X %04X";
			return snprintf( text, size, fmt, shorts[0], shorts[1] );
			}
		else	return snprintf( text, size, "no data" );
		break;
	case 32 :
	default:
		if	( iline < ramstock[iram].w32.size() )
			{
			fmt = "%08X";
			return snprintf( text, size, fmt, ramstock[iram].w32[iline] );
			}
		else	return snprintf( text, size, "no data" );
		break;
	}
}

void target::ram_all2string( string * s, unsigned int iram )
{
unsigned int iline, qlines;
char tbuf[256]; const char * fmt = OPT_FMT " ";
qlines = get_ram_qlines(iram);
if	( qlines == 0 )
	return;
for	( iline = 0; iline < qlines; ++iline )
	{
	snprintf( tbuf, sizeof(tbuf), fmt, (opt_type)get_ram_adr( iram, iline ) );
	*s += string( tbuf );
	ram_val2txt( tbuf, sizeof(tbuf), iram, iline );
	*s += string( tbuf );
	*s += string("\n");
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
	char lbuf[256]; int pos, len;
	while	( fgets( lbuf, sizeof( lbuf ), fil ) )	// aucune confiance dans ce fgets
		{
		lbuf[sizeof(lbuf)-1] = 0;		// il peut omettre le terminateur
		len = (int)strlen(lbuf);
		// on attaque par la fin pour enlever line end et trailing blank
		pos = len - 1;
		while	( ( pos >= 0 ) && ( lbuf[pos] <= ' ' ) ) // ou rendre une chaine vide
			lbuf[pos--] = 0;			 // on enleve line end et trailing blank
		// on attaque par le debut pour enlever UTF8 invalide qui plante GTK
		// traitement caractere par caractere : filtrage sur place
		char c; unsigned int i = 0, j = 0;
		u8filtre filu;
		while	( ( c = lbuf[i++] ) != 0 )
			{
			filu.putc( c );
			while	( filu.avail() )
				{ lbuf[j++] = filu.getc(); }
			if	( j >= sizeof(lbuf) )
				{ j = sizeof(lbuf) - 1; break; }
			}
		lines.push_back( string( lbuf ) );
		}
	fclose( fil );
	status = 1;
	}
}

static const signed char hex_digit[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

// translate raw hex bytes into bin 32-bit words
void memory::txt2w32( const char * txt )
{
unsigned int v; int d;
do	{
	v = 0;
	d = hex_digit[(unsigned int)(*(txt++))]; if ( d < 0 ) break; v |= ( d << 4 );
	d = hex_digit[(unsigned int)(*(txt++))]; if ( d < 0 ) break; v |= ( d );
	d = hex_digit[(unsigned int)(*(txt++))]; if ( d < 0 ) break; v |= ( d << 12 );
	d = hex_digit[(unsigned int)(*(txt++))]; if ( d < 0 ) break; v |= ( d << 8 );
	d = hex_digit[(unsigned int)(*(txt++))]; if ( d < 0 ) break; v |= ( d << 20 );
	d = hex_digit[(unsigned int)(*(txt++))]; if ( d < 0 ) break; v |= ( d << 16 );
	d = hex_digit[(unsigned int)(*(txt++))]; if ( d < 0 ) break; v |= ( d << 28 );
	d = hex_digit[(unsigned int)(*(txt++))]; if ( d < 0 ) break; v |= ( d << 24 );
	w32.push_back( v );
	} while ( d >= 0 );
}

// parsing du code executable
void asmline::parse_the_bytes( const char * txt )
{
unsigned int ib = 0, c; int d;
while	( ib < MAXOPBYTES )
	{
	c = *(txt++);
	if	( c == ' ' )	// ignorer le blanc devant un byte, il ne sert a rien
		c = *(txt++);
	if	( c == 0 )
		break;
	d = hex_digit[c] << 4;
	c = *(txt++);
	if	( c == 0 )
		break;
	d |= hex_digit[c];
	if	( d >= 0 )
                bytes[ib++] = d;
	}
qbytes = ib;
}

/* verifier que les adresses sont triees dans la map
void target::asmmap_dump() {
const char * fmt = OPT_FMT "\n";
//map<unsigned long long, unsigned int>::iterator itou = asmmap.begin();
//while	( itou != asmmap.end() )
//	printf( fmt, (opt_type)(itou++)->first );
map<unsigned long long, unsigned int>::iterator itou = asmmap.end();
while	( itou != asmmap.begin() )
	printf( fmt, (opt_type)(--itou)->first );
fflush(stdout);
}*/
