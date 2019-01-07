

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
unsigned int pos;	// indice de registre (temporaire, pour parseur)
// constructeur
regbank() : isp(0), ibp(0), iip(0) {};
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
void set_reg_pos( string val ) { pos = strtoul( val.c_str(), NULL, 0 );	};	// std::stoul( val ); };
void set_reg_val( string val ) {
	unsigned long long newval = strtoull( val.c_str(), NULL, 0 );		// std::stoull( val, 0, 0 ); <-- BUG
	if	( pos < regs.size() )
		{
		if	( regs[pos].val != newval )
			regs[pos].changed = 1;
		regs[pos].val = newval;
		}
	};
void reset_reg_changes() {
	for	( unsigned int i = 0; i < regs.size(); ++i )
		regs[i].changed = 0;
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
string relpath;
string abspath;
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
};

class target {
public:
regbank regs;
vector <asmline> asmstock;
map <unsigned long long, unsigned int> asmmap;
vector <srcfile> filestock;
map <string, unsigned int> filemap;
vector <listing> liststock;
// methodes
int add_listing( unsigned long long adr );
void dump_listing( unsigned int i );
};

