

class registro {
public:
unsigned long long val;
int changed;
string name;
// constructeur
explicit registro( string nam ) : val( 0 ), changed( 0 ) { name = nam; };
};

class regbank {
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
};

class target {
public:
regbank regs;

};

