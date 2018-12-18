/*
 gcc -Wall -o jserv.exe jserv.c -lws2_32

 * jserv version b derive de simples.c ameliore - Simple TCP/UDP server
 * clot la connexion a reception de "QUIT"
 * les messages sont des textes sans null mais avec \r\n
 * protocole style telnet ou http : les messages peuvent arriver coupes en
 * morceaux meme byte par byte mais c'est CR/LF qui declenche le parsing
 * (ici CR est ignore...)
 * code pour Winsock2
 */

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_PORT 5001

void Usage(char *progname) {
	fprintf(stderr,"Usage\n%s -e [endpoint] -i [interface]\n",
		progname);
	fprintf(stderr,"Where:\n");
	fprintf(stderr,"\tendpoint is the port to listen on\n");
	fprintf(stderr,"\tinterface is the ipaddr (in dotted decimal notation)");
	fprintf(stderr," to bind to\n");
	fprintf(stderr,"Defaults are 5001 and INADDR_ANY\n");
	WSACleanup();
	exit(1);
}

int main( int argc, char **argv ) {
        #define MSG_MAX 1024
	char net_buffer[12];      // buffer pour recv
	char msg_buffer[MSG_MAX];  // buffer pour message 
	int msg_size;              // son contenu
	int echoflag = 0;
	char *interface= NULL;
	unsigned short port=DEFAULT_PORT;
	int retval, connected=0;
	int fromlen;
	int i;
	struct sockaddr_in local, from;
	WSADATA wsaData;
	SOCKET listen_socket, msg_socket;

	/* Parse arguments */
	if ( argc > 1 )
	   {
	   for ( i=1; i < argc ;i++ )
	       {
	       if   ( (argv[i][0] == '-') || (argv[i][0] == '/') )
	            {
		    switch ( tolower(argv[i][1]) )
		       {
		       case 'i': interface = argv[++i];  break;
		       case 'e': port = atoi(argv[++i]); break;
		       default : Usage(argv[0]);         break;
		       }
		    }
	       else Usage(argv[0]);
	       }
	   }
	if ( port == 0 )
	   Usage(argv[0]);

// demarrer Winsock
	if ( WSAStartup( 0x202, &wsaData ) == SOCKET_ERROR )
	   {
	   fprintf(stderr,"WSAStartup failed with error %d\n",WSAGetLastError());
	   WSACleanup();
	   exit(1);
	   }
	
// preparer la structure local ( type sockaddr_in )
	local.sin_family = AF_INET;
	// interface est 1 string (dotted decimal) qu'on convertit en long
	// avec inet_addr()
	// mais dans le cas simple INADDR_ANY pointe sur notre unique adapter
	local.sin_addr.s_addr = (!interface)?(INADDR_ANY):(inet_addr(interface)); 
        // htons swappe si necessaire les bytes du numero de port
	local.sin_port = htons(port);

// ouverture socket
	listen_socket = socket( AF_INET, SOCK_STREAM, 0 ); // TCP socket
	
	if ( listen_socket == INVALID_SOCKET )
	   {
           fprintf(stderr,"socket() failed with error %d\n",WSAGetLastError());
	   WSACleanup();
	   exit(1);
	   }

// lier (bind) ce socket avec notre struct local ==> choix interface et port	   

	if ( bind( listen_socket, (struct sockaddr*) &local, sizeof(local) ) 
	      == SOCKET_ERROR
	   )
	   {
	   fprintf(stderr,"bind() failed with error %d\n",WSAGetLastError());
	   WSACleanup();
	   exit(1);
	   }

// ecoutons donc (cette fonction n'est pas bloquante )
// le second argument est la taille maxi de la queue des connexions en attente
	if ( listen( listen_socket, 5 ) == SOCKET_ERROR )
	   {
	   fprintf(stderr,"listen() failed with error %d\n",WSAGetLastError());
	   WSACleanup();
	   exit(1);
	   }

	printf("%s: 'Listening' on port %d, \n", argv[0], port );
	
// boucle des connexions
while  ( 1 ) 
        {
	fromlen =sizeof(from);
	// la fonction accept ( bloquante sauf si socket non bloquant )
	// attend une connexion
	// elle rend un nouveau socket, et se charge de remplir la struc from
	msg_socket = accept( listen_socket, (struct sockaddr*)&from, &fromlen );
	
	if ( msg_socket == INVALID_SOCKET )
	   {
	   fprintf(stderr,"accept() error %d\n",WSAGetLastError());
	   WSACleanup();
	   exit(1);
           }
	// numero IP et port sont convertis pour affichage   
	printf("accepted connection from %s, port %d\n", 
		inet_ntoa(from.sin_addr), ntohs(from.sin_port)) ;
	connected = 1; msg_size = 0;
	// boucle des messages	
	while ( connected )
	   {	
	   // lecture message (bloquante, sauf si socket non bloquant)	
	   retval = recv( msg_socket, net_buffer, sizeof(net_buffer), 0 );
			
	   if ( retval == SOCKET_ERROR )
	      {
	      int errcode;
	      errcode = WSAGetLastError();
	      if   ( errcode == WSAECONNRESET ) // may be WSAECONNABORTED
	           {
		   printf("Client closed connection\n");
	           closesocket(msg_socket);
	           connected = 0; break;
	           }
	      else { 
	           fprintf(stderr,"recv() failed: error %d\n", errcode );
	           closesocket(msg_socket);
	           exit(1);
		   }
	      }
	   if ( retval == 0 )
	      {
	      printf("Client closed connection\n");
	      closesocket(msg_socket);
	      connected = 0; break;
	      }
	   // normalement retval est la longueur du message reçu
	   // ajoutons cela a notre buffer de message
	   if   (  ( msg_size + retval + 1 )  <  MSG_MAX  )
	        { 
	        memcpy( msg_buffer + msg_size, net_buffer, retval );
		msg_size += retval;
		}
	   else {
	        memcpy( msg_buffer + msg_size, net_buffer, MSG_MAX - msg_size - 1 );
		msg_size = MSG_MAX - 1;
                }
	   // echo eventuel
	   if ( echoflag )
	      {
	      retval = send( msg_socket, net_buffer, retval, 0 );
              if ( retval == SOCKET_ERROR )
	         {
	         fprintf(stderr,"send() failed: error %d\n",WSAGetLastError());
	         }
	      }	
	   // message complet ?		   
	   if (  ( msg_buffer[msg_size-1] == '\n' ) || ( msg_size == MSG_MAX - 1 )  )
	      {
	      msg_buffer[msg_size] = 0;  // terminateur pour printf
	      printf("Client > %s", msg_buffer );
              if ( strncmp( msg_buffer, "QUIT", 4 ) == 0 )
	         {
	         printf("Client said QUIT, let's close connection\n");
	         closesocket(msg_socket);
	         connected = 0; break;
	         }
              if ( strncmp( msg_buffer, "echo", 4 ) == 0 )
	         {
	         echoflag ^= 1;
	         printf("Client said echo, let's turn echo %s\n",
		         (echoflag)?("on"):("off") );
	         }
	      // une reponse au client
	      sprintf( net_buffer, "Ok %d\r\n", connected++ );
	      printf("Server > %s", net_buffer );
	      retval = send( msg_socket, net_buffer, strlen(net_buffer), 0 );
	      if ( retval == SOCKET_ERROR )
	         {
	         fprintf(stderr,"send() failed: error %d\n",WSAGetLastError());
	         }
	      // preparer pour prochain message	 
	      msg_size = 0;
	      } // message complet

	   } // boucle messages   
	}
}

