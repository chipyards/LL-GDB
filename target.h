

class registro {	// a core register
public:
unsigned long long val;
int changed;
string name;
// constructeur
explicit registro( string nam ) : val( 0 ), changed( 0 ) { name = nam; };
};

class regbank {		// the register bank
public:
vector <registro> regs;
unsigned int isp;
unsigned int ibp;
unsigned int iip;
// constructeur
regbank() : isp(0), ibp(0), iip(0) { regs.push_back( registro( string( "invalid") ) ); };
// methodes
void start_reg_names() { regs.clear(); };
void add_reg_name( string val ) {
	if	( ( val == string("esp") ) || val == string("rsp") )
		isp = regs.size();
	else if	( ( val == string("ebp") ) || val == string("rbp") )
		ibp = regs.size();
	else if	( ( val == string("eip") ) || val == string("rip") )
		iip = regs.size();
	regs.push_back( registro( val ) );
	};
void set_reg( unsigned int pos, unsigned long long newval ) {
	if	( pos < regs.size() )
		{
		regs[pos].changed = ( regs[pos].val != newval );
		regs[pos].val = newval;
		}
	};
registro * get_rip() { return &(regs[iip]); };
registro * get_rsp() { return &(regs[isp]); };
registro * get_rbp() { return &(regs[ibp]); };
};

#define MAXOPBYTES 8

class asmline {		// one line of disassembled code (may come with some source lines)
public:
unsigned long long adr;
unsigned char bin[MAXOPBYTES];	// variable length executable code
unsigned int qbytes;	// number of bytes making the executable code
int src0;		// first source line, or -1 if none
int src1;		// last source line (inclusive)
unsigned int isrc;	// index of file in filestock
string asmsrc;		// disassembed intruction
// methodes
void init() {
	adr = 0; qbytes = 0; src0 = -1;
	};
unsigned long long nextadr() {			// adr instr. suivante
	return( adr + qbytes );
	};
void count_the_bytes( string rawbytes ) {	// taille du code executable
	qbytes = ( rawbytes.size() + 1 )/ 3;
	};
void dump() {					// just for debug
	if	( src0 >=0 )
		printf("         src(%4d,%4d) file %d\n", src0, src1, isrc );
	printf("%08X %u %s\n", (unsigned int)adr, qbytes, asmsrc.c_str() );
	};
void set_adr( string val ) {
	adr = strtoull( val.c_str(), NULL, 0 );
	};
};

class srcfile {		// one source file
public:
int status;		// 0 = init, -1 = echec, 1 = lecture ok
string relpath;
string abspath;
vector <string> lines;
// constructeur
srcfile() : status(0) {};
// methodes
void readfile();	// remplir lines[]
};

class listing {			// a disassembly listing ready for display (mixed src-asm)
public:
unsigned long long adr0;	// begin address
unsigned long long adr1;	// end address (excluded)
vector <int> lines;		// encoded line references
				//	>= 0 : index in asmstock
				//	<  0 : combined index-in-filestock and line-number-in-file
// methods
static unsigned int decode_file_index( unsigned int ref ) {
	return ( ref >> 16 ) & 0x7FFF;
	};
static unsigned int decode_line_number( unsigned int ref ) {
	return ref & 0xFFFF;
	};
static int encode_ref( unsigned int file_index, unsigned int line_number ) {
	return ( line_number & 0xFFFF ) | ( file_index << 16 ) | 0x80000000;
	};
int search_line( int index, unsigned int hint );	// chercher un index dans le listing (-1 si echec)
};

typedef enum { Ready=0, Running=1, Disas=2, Registers=4, RAM=8, Breaks=16, Init=0x1000 } status_enum;

class target {
public:
int status;
regbank regs;
vector <asmline> asmstock;
map <unsigned long long, unsigned int> asmmap;
vector <srcfile> filestock;
map <string, unsigned int> filemap;
vector <listing> liststock;
map <unsigned long long, unsigned int> breakpoints;
// constructeur
target() : status(Init) {
	asmline badline;		// preparer une ligne pour affichage provisoire
	asmstock.push_back( badline );
	asmstock.back().init();		// adr = 0 !
	asmstock.back().asmsrc = string("waiting for disassembly");
	asmmap[0] = 0;
	listing badlist;		// avoir toujours au moins un listing, meme vide
	liststock.push_back( badlist );
	}
// methodes
int fill_listing( unsigned int ilist, unsigned long long adr );
int add_listing( unsigned long long adr );
void dump_listing( unsigned int i );
unsigned long long get_ip() {
	return regs.get_rip()->val;
	}
int is_break( unsigned long long adr ) {
	return breakpoints.count( adr );
	}
int get_ip_asm_line() {	// rend -1 si adr non desassemblee
	if	( asmmap.count(get_ip()) )
		return (int)asmmap[get_ip()];
	else	return -1;
	};
const char * get_src_line( unsigned int ifil, unsigned int ilin ) {
	if	( ifil < filestock.size() )
		{
		if	( --ilin < filestock[ifil].lines.size() )
			return( filestock[ifil].lines[ilin].c_str() );
		else	{
			// static char tbuf[64];
			// snprintf( tbuf, sizeof(tbuf), "invalid src line, status %d", filestock[ifil].status );
			// return tbuf;
			return filestock[ifil].relpath.c_str();
			}
		}
	else	return "(invalid src file)";
	}
void status_set( status_enum bit ) {
	status |= (int)bit;
	}
void status_reset( status_enum bit ) {
	status &= ~((int)bit);
	}
void add_break( unsigned int num, unsigned long long adr ) {
	breakpoints[adr] = num;		// s'il existe deja on l'ecrase et l'ancien num est perdu...
	}
};

