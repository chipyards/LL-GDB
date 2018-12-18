#include <windows.h> 
#include <stdio.h> 
#include <string.h>

#define BUFSIZE 4096 
 

#include <stdarg.h>
/* gasp classique --> stderr par defaut */
void gasp( const char *fmt, ... )  /* fatal error handling */
{
  va_list  argptr;
  fprintf( stderr, "ERROR : " );
  va_start( argptr, fmt );
  vfprintf( stderr, fmt, argptr );
  va_end( argptr );
  fprintf( stderr, "\n" );
  exit(1);
}


HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

void CreateChildProcess( char * ); 
void WriteToPipe( const char * ); 
void ReadFromPipeLoop(void); 
 

// le thread de reception
DWORD WINAPI thread_body( LPVOID lpvParam )
{
ReadFromPipeLoop();
return 1;
}

int main( int argc, char **argv ) 
{ 
char tbuf[1024]; 

   SECURITY_ATTRIBUTES saAttr; 
 
   printf("\n->Start of parent execution.\n");fflush(stdout);

// Set the bInheritHandle flag so pipe handles are inherited. 
 
   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
   saAttr.bInheritHandle = TRUE; 
   saAttr.lpSecurityDescriptor = NULL; 

// Create a pipe for the child process's STDOUT. 
 
   if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) ) 
      gasp("StdoutRd CreatePipe"); 

// Ensure the read handle to the pipe for STDOUT is not inherited.

   if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
      gasp("Stdout SetHandleInformation"); 

// Create a pipe for the child process's STDIN. 
 
   if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) 
      gasp("Stdin CreatePipe"); 

// Ensure the write handle to the pipe for STDIN is not inherited. 
 
   if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
      gasp("Stdin SetHandleInformation"); 
 
// Create the child process. 
   
if	( argc > 1 )
	snprintf( tbuf, sizeof(tbuf), argv[1] );
else	snprintf( tbuf, sizeof(tbuf), "gdb td_m2.exe" );

   CreateChildProcess( tbuf );

// create the receiver thread
HANDLE hThread;
hThread = CreateThread( 
		NULL,				// permission par defaut  
		0,                 		// default stack size 
		thread_body,    		// thread proc
		0,    				// thread parameter 
		0,                 		// not suspended 
		NULL );      			// adresse pour recuperer thread ID 

if	( hThread == NULL ) 
	gasp("CreateThread failed"); 

int c;

do	{
	c = getchar();
	tbuf[0] = 0;
	switch	( c )
		{
		case 'b' : snprintf( tbuf, sizeof(tbuf), "-break-insert main\n" );
			break;
		case 'r' : snprintf( tbuf, sizeof(tbuf), "-exec-run\n" );
			break;
		case 'c' : snprintf( tbuf, sizeof(tbuf), "-exec-continue\n" );
			break;
		case 'q' : snprintf( tbuf, sizeof(tbuf), "-gdb-exit\n\n" );
		}
	if	( tbuf[0] )
		WriteToPipe(tbuf); 
	} while ( c != 'Q' );

   return 0; 
} 
 
void CreateChildProcess( char * cmdline )
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{ 
   PROCESS_INFORMATION piProcInfo; 
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE; 
 
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
      cmdline,		// command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      0,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &piProcInfo);  // receives PROCESS_INFORMATION 
   
   // If an error occurs, exit the application. 
   if ( ! bSuccess ) 
      gasp("CreateProcess");
   else 
   {
      // Close handles to the child process and its primary thread.
      // Some applications might keep these handles to monitor the status
      // of the child process, for example. 

      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
   }
}
 
void WriteToPipe(const char * tbuf) 

// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data. 
{ 
   DWORD dwWritten; 
   BOOL bSuccess = FALSE;
 
      bSuccess = WriteFile(g_hChildStd_IN_Wr, tbuf, strlen(tbuf), &dwWritten, NULL);
      if ( ! bSuccess )
	gasp("WriteFile"); 
 
} 
 
/*
// Close the pipe handle so the child process stops reading. 
 
   if ( ! CloseHandle(g_hChildStd_IN_Wr) ) 
      ErrorExit(TEXT("StdInWr CloseHandle")); 
*/

void ReadFromPipeLoop(void) 
// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
{ 
   DWORD dwRead; 
   CHAR chBuf[BUFSIZE]; 
   BOOL bSuccess = FALSE;

   for (;;) 
   { 
      bSuccess = ReadFile( g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
      if	( ! bSuccess )
		 { printf("echec ReadFile\n"); break; }
	else if	( dwRead == 0 )
		 { printf("read 0 bytes\n"); break; }
	 else 	{ chBuf[dwRead] = 0; printf("-->%s<--\n", chBuf ); }
 fflush(stdout);
   } 
} 
 
