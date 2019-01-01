

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
unsigned int pos;
// methodes
void start_reg_names() { regs.clear(); };
void add_reg_name( string val ) { regs.push_back( registro( val ) ); };
void set_reg_pos( string val ) { pos = strtoul( val.c_str(), NULL, 0 );	};	// std::stoul( val ); };
void set_reg_val( string val ) {
	unsigned long long newval = strtoull( val.c_str(), NULL, 0 );		// std::stoull( val, 0, 0 ); <-- BUG
	if	( pos < regs.size() )
		{
		regs[pos].changed = ( regs[pos].val != newval );
		regs[pos].val = newval;
		}
	};

};

class target {
public:
regbank regs;

};

