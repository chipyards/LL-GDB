
// un result est un couple clef-valeur
// la valeur peut etre 1 string ou un conteneur t.q. [] ou {}

// un conteneur (tuple ou array)
class conteneur {
public:
string nam;
char type;	// parmi ',','[','{'
// constructor
conteneur( const string & thenam, char c ) { nam = thenam; type = c; };
};

// le parseur
class mi_parse {
public:
string nam;     // nom de result courant
string val;     // valeur de result courant

int e;			// etat du parseur

vector < conteneur > stac;	// stack of parent containers

// constructor
mi_parse() : e(1) {};

// methodes
// cette fonction parse un caractere et rend zero sauf si on a une fin de valeur
int proc1char( int c );

// formatter un dump texte indente, en fonction de la valeur retournee par mi_parse::proc1char()
int dump( int retval, char * buf, int size );

};
