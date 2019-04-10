

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
unsigned int option_qregs;
unsigned int isp;
unsigned int ibp;
unsigned int iip;
// constructeur
regbank() : isp(0), ibp(0), iip(0) { regs.push_back( registro( string( "invalid") ) ); };
// methodes
void start_reg_names() { regs.clear(); };
void add_reg_name( string val ) {
	if	( regs.size() < option_qregs )
		{
		if	( ( val == string("esp") ) || ( val == string("rsp") ) )
			isp = regs.size();
		else if	( ( val == string("ebp") ) || ( val == string("rbp") ) )
			ibp = regs.size();
		else if	( ( val == string("eip") ) || ( val == string("rip") ) )
			iip = regs.size();
		regs.push_back( registro( val ) );
		}
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

#define MAXOPBYTES 16

class asmline {		// one line of disassembled code (may come with some source lines)
public:
unsigned long long adr;
unsigned char bytes[MAXOPBYTES];	// variable length executable code
unsigned int qbytes;	// number of bytes making the executable code
int src0;		// first source line, or -1 if none
int src1;		// last source line (inclusive)
unsigned int isrc;	// index of file in filestock
string asmsrc;		// disassembed intruction
// methodes
void init() {
	adr = 0; qbytes = 0; src0 = -1;
	};
unsigned long long nextadr() {				// adr instr. suivante
	return( adr + qbytes );
	};
void set_adr( string val ) {
	adr = strtoull( val.c_str(), NULL, 0 );
	};
void count_the_bytes( string rawbytes ) {		// taille du code executable
	qbytes = ( rawbytes.size() + 1 )/ 3;
	};
void parse_the_bytes( const char * txt );		// parsing du code executable
int bin2txt( char * text, unsigned int size ) {	// dump executable code
	text[0] = 0;	// au cas ou on aurait 0 bytes...
	if	( size < (qbytes*3+1) )
		return 0;
	for	( unsigned int ib = 0; ib < qbytes; ++ib )
		snprintf( text + (ib*3), size - (ib*3), "%02X ", bytes[ib] );
	return qbytes*3;
	}
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

class memory {		// a memory listing ready for display
public:
unsigned long long adr0;	// begin address;
vector <unsigned int> w32;	// 32-bit words
// methodes
void txt2w32( const char * txt );	// parser les bytes en hex --> w32
};

// liste des jobs, par ordre de priorite decroissante
typedef enum { GDBSet=0, File, FileInfo, BreakSetKill, BreakList, Run, Continue, RegNames, RegVal, Disass, RAMRead } job_enum;
#define QJOB 11	// nombre de jobs prevus dans cette enum A MAINTENIR A LA MAIN!!!
#define QUEUED_BIT	1	// bit pour un job dans job_status
#define RUNNING_BIT	2
#define ERROR_BIT	4
#define DONE_BIT	8
#define CLEAR_BITS	(QUEUED_BIT|RUNNING_BIT|ERROR_BIT)	// all but DONE
#define QUEUED_MASK	0x1111111111111111LL
#define RUNNING_MASK	0x2222222222222222LL
#define ERROR_MASK	0x4444444444444444LL

class target {
public:
unsigned long long job_status;	// 4 bits/job
string job_cmd[QJOB];		// commande courante pour chaque job
string main_file_name;
string reason;		// reason of *stopped
string error_msg;
regbank regs;
vector <asmline> asmstock;
map <unsigned long long, unsigned int> asmmap;
vector <srcfile> filestock;
map <string, unsigned int> filemap;
vector <listing> liststock;
map <unsigned long long, unsigned int> breakpoints;
vector <memory> ramstock;
int option_binvis;
// constructeur
target() : job_status(0LL), option_binvis(0) {
	asm_init();
	memory badram;		// avoir toujours au moins une RAM, meme vide
	badram.adr0 = 0;
	ramstock.push_back( badram );
	}
// methodes
void asm_init() {
	asmstock.clear();
	asmmap.clear();
	asmline badline;		// preparer une ligne pour affichage provisoire
	asmstock.push_back( badline );
	asmstock.back().init();		// adr = 0 !
	asmstock.back().asmsrc = string("disassembly not available");
	asmmap[0] = 0;
	liststock.clear();
	listing badlist;		// avoir toujours au moins un listing, meme vide
	liststock.push_back( badlist );
	}
int fill_listing( unsigned int ilist, unsigned long long adr );
int add_listing( unsigned long long adr );
void dump_listing( unsigned int i );
int get_disa_ref( unsigned int ilist, unsigned int i ) { // retourne la ref cherchee ou 0 si echec
	listing * list;					 // (0 pointe sur ligne asm "disassembly not available")
	// recuperer le listing selon ilist
	if	( ilist < liststock.size() )
		list = &(liststock[ilist]);
	else	return 0;
	// recuperer la ref selon i
	if	( i < list->lines.size() )
		return list->lines[i];
	else	return 0;
	}
int ram_val2txt( char * text, unsigned int size, unsigned int iram, unsigned int iline, int ram_format );
unsigned long long get_ip() {
	return regs.get_rip()->val;
	}
unsigned long long get_sp() {
	return regs.get_rsp()->val;
	}
unsigned long long get_bp() {
	return regs.get_rbp()->val;
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
void add_break( unsigned int num, unsigned long long adr ) {
	breakpoints[adr] = num;		// s'il existe deja on l'ecrase et l'ancien num est perdu...
	}
void job_set_cmd( const char * cmd, job_enum job ) {
	job_cmd[job] = string( cmd );
	}
void job_set_queued( job_enum job ) {
	job_status |= ( (long long)QUEUED_BIT << (((unsigned int)job)<<2) );
	}
bool job_is_queued( job_enum job ) {
	return ( ( job_status & ( (long long)QUEUED_BIT << (((unsigned int)job)<<2) ) ) != 0 );
	}
void job_set_running( job_enum job ) {
	job_status &= (~( (long long)CLEAR_BITS << (((unsigned int)job)<<2) ));
	job_status |= ( (long long)RUNNING_BIT << (((unsigned int)job)<<2) );
	}
void job_reset_running( job_enum job ) {
	job_status &= (~( (long long)RUNNING_BIT << (((unsigned int)job)<<2) ));
	}
void job_set_error( job_enum job ) {
	job_status &= (~( (long long)CLEAR_BITS << (((unsigned int)job)<<2) ));
	job_status |= ( (long long)ERROR_BIT << (((unsigned int)job)<<2) );
	}
void job_queue_cmd( const char * cmd, job_enum job ) {
	job_set_cmd( cmd, job );
	job_set_queued( job );
	}
bool job_isanyqueued() {
	return ( ( job_status & QUEUED_MASK ) != 0 );
	}
bool job_isanyrunning() {
	return ( ( job_status & RUNNING_MASK ) != 0 );
	}
bool job_isanyerror() {
	return ( ( job_status & ERROR_MASK ) != 0 );
	}
int job_nextqueued() {
	unsigned long long q = job_status;
	int ijob = 0;
	while	( ijob < QJOB )
		{
		if	( (int)q & QUEUED_BIT )
			return ijob;
		q >>= 4; ++ijob;
		}
	return -1;
	}
int job_running() {
	unsigned long long q = job_status;
	int ijob = 0;
	while	( ijob < QJOB )
		{
		if	( (int)q & RUNNING_BIT )
			return ijob;
		q >>= 4; ++ijob;
		}
	return -1;
	}
void job_dump() {
	unsigned long long q = job_status;
	for	( int ijob = 0; ijob < QJOB; ++ijob )
		{
		printf("%c%c%c%c %s\n",
			((int)q&QUEUED_BIT)?('Q'):('-'),((int)q&RUNNING_BIT)?('R'):('-'),
			((int)q&ERROR_BIT)?('E'):('-'),((int)q&DONE_BIT)?('D'):('-'), job_cmd[ijob].c_str() );
		q >>= 4;
		}
	printf("\n"); fflush(stdout);
	}
};

