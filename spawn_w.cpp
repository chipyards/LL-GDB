#include <windows.h> 
#include <stdio.h> 
#include <string.h>

#include "spawn_w.h"

extern "C" {
static DWORD WINAPI thread_body( LPVOID lpvParam )
{
daddy * dad = (daddy*)lpvParam;
int c; unsigned int bSuccess; long unsigned cnt;
 
printf("child stdout thread started, cmd = %s\n", dad->cmd_line ); fflush(stdout);

while	(1)
	{	// lire un seul char a la fois...
	bSuccess = ReadFile( dad->g_hChildStd_OUT_Rd, &c, 1, &cnt, NULL );
	if	( ! bSuccess )
		{ c = -2; cnt = 1; }
	if	( cnt )
		{
		dad->ring_data[dad->ring_wri & (QRING-1)] = c;
		++dad->ring_wri;
		}
	} 
return 1;
}
}

// lire un char dans le fifo de son stdout (-1: fifo vide, -2: erreur )
int daddy::child_getc()
{					// pas encore fait la detection debordement
int c;
if	( ring_wri != ring_rdi )
	{
	c = ring_data[ring_rdi & (QRING-1)];
	++ring_rdi;
	return c;
	}
else	return -1;
}

int daddy::start_child( char * cmdline )
{
SECURITY_ATTRIBUTES saAttr; 
cmd_line = cmdline;	// copie pour reference

/*** PREPARE THE PIPES ***/

// Set the bInheritHandle flag so pipe handles are inherited. 
saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
saAttr.bInheritHandle = TRUE; 
saAttr.lpSecurityDescriptor = NULL; 

// Create a pipe for the child process's STDOUT. 
if	( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) ) 
	return 771; 
// Ensure the read handle to the pipe for STDOUT is not inherited.
if	( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
	return 772;
// Create a pipe for the child process's STDIN. 
if	( ! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0) )
	return 773;
// Ensure the write handle to the pipe for STDIN is not inherited. 
if	( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
	return 774;
 
/*** CREATE THE PROCESS ***/ 

STARTUPINFO siStartInfo;
BOOL bSuccess; 
 
// Set up members of the PROCESS_INFORMATION structure. 
ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
// Set up members of the STARTUPINFO structure. 
// This structure specifies the STDIN and STDOUT handles for redirection.
 
ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
siStartInfo.cb = sizeof(STARTUPINFO); 
siStartInfo.hStdError = g_hChildStd_OUT_Wr;
siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
siStartInfo.hStdInput = g_hChildStd_IN_Rd;
siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
// Create the child process. 
    
bSuccess = CreateProcess(NULL, 
	cmdline,	// command line 
	NULL,		// process security attributes 
	NULL,		// primary thread security attributes 
	TRUE,		// handles are inherited 
	0,		// creation flags 
	NULL,		// NULL ==> use parent's environment 
	NULL,		// NULL ==> use parent's current directory 
	&siStartInfo,	// STARTUPINFO pointer 
	&piProcInfo	// struct to receive PROCESS_INFORMATION 
	);
 
if	( ! bSuccess ) 
	return 780;	// on a cette erreur si cmdline invoque un prog qui n'existe pas

/*** CREATE THE STDOUT THREAD ***/ 
ring_wri = 0;
ring_rdi = 0;
hThread = CreateThread( 
		NULL,		// permission par defaut  
		0,              // default stack size 
		thread_body,    // thread proc
		this,    	// thread parameter 
		0,              // not suspended 
		NULL      	// adresse pour recuperer thread ID 
		);

if	( hThread == NULL ) 
	return 790; 

return 0;
}

// envoyer une commande dans son stdin
int daddy::send_cmd( const char * cmd )
{
DWORD dwWritten; 
 
if	( ! WriteFile( g_hChildStd_IN_Wr, cmd, strlen(cmd), &dwWritten, NULL ) )
	return -1; 

return 0;
}

