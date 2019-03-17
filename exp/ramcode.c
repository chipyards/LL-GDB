/*
Demo injection de code dans la RAM (segment static data)
Windows 32 bit (MinGW32) only !! L'equivalent en 64 bits donne segfault...
ATTENTION
	- notre truc pour trouver l'emplacement de l'adresse de retour est calcule SANS FRAME POINTER,
	  cela concerne -O0 et -Os (bizarrement!)
	- l'inlining casserait notre demo, cela concerne -O2 et -O3
gcc -Wall -g -O0 -fno-inline -fomit-frame-pointer -o ramcode0 exp/ramcode.c
gcc -Wall -g -O1 -fno-inline -fomit-frame-pointer -o ramcode1 exp/ramcode.c
gcc -Wall -g -O2 -fno-inline -fomit-frame-pointer -o ramcode2 exp/ramcode.c
gcc -Wall -g -O3 -fno-inline -fomit-frame-pointer -o ramcode3 exp/ramcode.c
gcc -Wall -g -Os -fno-inline -fomit-frame-pointer -o ramcodes exp/ramcode.c
gcc -Wall -g -Og -fno-inline -fomit-frame-pointer -o ramcodeg exp/ramcode.c

Ce prog :
	- essaie de deviner l'adresse de l'adresse de retour dans la pile
	- detourne l'execution du retour vers un bout de code cree en RAM
	- ce bout de code ajoute 0x55 a une var quelconque
	- preserve la suite du prog
*/ 
#include <stdio.h>


// tableau contenant du code executable !
// exemple pour incrementer une var. a l'adresse 0x406038
/*
	opcodes = "a1 38 60 40 00"
        inst = "mov    0x406038,%eax"
        opcodes = "83 c0 01"
        inst = "add    $0x1,%eax"
        opcodes = "a3 38 60 40 00"
        inst = "mov    %eax,0x406038"
*/
unsigned char zone[] =
	//0     1     2     3		// les 0x90 NOP servent a faciliter l'alignement
/*0x00*/{ 0x90, 0x90, 0x90, 0xa1,	// mov    0x????????,%eax"
/*0x04*/  0xAA, 0xAA, 0xAA, 0xAA, 		// deplacement a completer
/*0x08*/  0x90, 0x83, 0xc0, 0x55,	// add    $0x55,%eax
/*0x0C*/  0x90, 0x90, 0x90, 0xa3,	// mov    %eax,0x????????
/*0x10*/  0xAA, 0xAA, 0xAA, 0xAA,		// deplacement a completer
/*0x14*/  0x90, 0x90, 0x90, 0xE9,	// E9 = jump relatif 32 bits
/*0x18*/  0xAA, 0xAA, 0xAA, 0xAA,		// deplacement a completer 
	};
size_t	adradr;		// pointeur sur la pile qui est elle meme supposee contenir des adresses
size_t	retadr;		// adresse de retour dans main
int a = -2;

void mysub( size_t varadr );

int main()
{
mysub( (size_t)&a );
printf("a=%d\n", a );
fflush(stdout);
return 0;
}

void mysub( size_t varadr ) {
// brancher notre code dans le retour de cette fonction ------------------------------------------
adradr = (size_t)(&varadr);			// recuperer adr de l'arg pour situer la pile
adradr -= sizeof(size_t);			// pointer sur l'adresse de retour de la func
retadr = *((size_t *)adradr);			// recuperer cette adresse de retour
*((size_t *)adradr) = (size_t)zone;		// remplacer l'adresse de retour par celle de zone
// reconstituer le retour à la fin de notre code -------------------------------------------------
retadr -= ( (size_t)zone + sizeof(zone) );	// la coder en relatif a la "fin" de zone
*((unsigned int *)(zone+0x18)) = (unsigned int)retadr;	// completer notre jmp avec le deplacement
// completer les adresses d'accés a la donnee ----------------------------------------------------
*((unsigned int *)(zone+0x04)) = (unsigned int)varadr;
*((unsigned int *)(zone+0x10)) = (unsigned int)varadr;
}
