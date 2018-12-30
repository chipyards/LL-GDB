/** ========================== filtres UTF8 ========================= */

// ce filtre ne laisse passer que l'ascii et le valide UTF8 (3 bytes max)
// il remplace les bytes suspects par un et un seul WARNCHAR
// il faut lire le filtre a chaque byte sinon il peut deborder !

#define WARNCHAR '@'

class u8filtre {
public :
char fifo[4];	// de 1 a 4 ne contient que les multibytes
int wi, ri, expect, errcnt;
// constructeur
u8filtre() : wi(0), ri(0), expect(0) {};
// accesseurs
void putc( char c );
int avail();
char getc();
};
