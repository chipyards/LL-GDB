
#define QRING	(1<<18)		// doit etre 1 puissance de 2

// le spawneur pour Windows
class daddy {
public:
// handles pour les pipes
HANDLE g_hChildStd_IN_Rd;
HANDLE g_hChildStd_IN_Wr;
HANDLE g_hChildStd_OUT_Rd;
HANDLE g_hChildStd_OUT_Wr;
// thread local pour lire pipe stdout du child
HANDLE hThread;
// struct contenant les handles du child process piProcInfo.hProcess et piProcInfo.hThread
PROCESS_INFORMATION piProcInfo;
// fifo pour stdout du child
char ring_data[QRING];
unsigned int ring_wri;	// write index
unsigned int ring_rdi;	// read index

char * cmd_line;	// pour memoire


// methodes
public:
// demarrer un child process selon ligne de commande (tout dans 1 chaine)
int start_child( char * cmdline );

// envoyer une commande dans son stdin
int send_cmd( const char * cmd );

// lire un char dans le fifo de son stdout (-1: fifo vide, -2: erreur )
int child_getc();

};
