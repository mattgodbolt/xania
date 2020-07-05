/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  doorman.c                                                            */
/*                                                                       */
/*************************************************************************/


/*
 * Doorman.c
 * Stand-alone 'doorman' for the MUD
 * Sits on port 9000, and relays connections through a named pipe
 * to the MUD itself, thus acting as a link between the MUD
 * and the player independant of the MUD state.
 * If the MUD crashes, or is rebooted, it should be able to maintain
 * connections, and reconnect automatically
 *
 * Terminal issues are resolved here now, which means ANSI-compliant terminals
 * should automatically enable colour, and word-wrapping is done by doorman
 * instead of the MUD, so if you have a wide display you can take advantage
 * of it.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <signal.h>
#include <getopt.h>
#include "merc.h"
#include "doorman.h"

// Data is bloody well unsigned - I still maintain that char should be! ;)
typedef unsigned char byte;

void IncomingData (int fd, byte  *buffer, int nBytes);
void SendConnectPacket (int i);
void SendInfoPacket (int i);
void ProcessIdent (struct sockaddr_in address, int resultFd);

typedef struct tagChannel
{
	int       	fd;           /* File descriptor of the channel */
	int       	xaniaID;      /* Xania's ID to this channel */
	char      	*hostname;    /* Ptr to hostname */
	char      	*username;    /* Ptr to username, if any */
	byte	*buffer;      /* Buffer of incoming data */
	int       	nBytes;
	short     	port;
	long      	netaddr;
	int       	width, height;/* Width and height of terminal */
	bool      	ansi, gotTerm;
	bool      	echoing;
	bool      	firstReconnect;
	bool       connected;    /* Socket is ready to connect to Xania */
	pid_t      identPid;     /* PID of look-up process */
	int        identFd[2];   /* FD of look-up processes pipe (read on 0) */
	char		authCharName[64]; /* name of authorized character */
} Channel;

/* Global variables */
Channel channel[CHANNEL_MAX];
int mudFd;
pid_t child;
int port = 9000;
bool connected = false, waitingForInit = false;
int listenSock;
time_t lastConnected;
fd_set ifds;
int debug = 0;

/* printf() to an fd */
void fdprintf (int fd, char *format, ...)
{
	char buffer[4096];
	int len;
	
	va_list args;
	va_start (args, format);
	len = vsprintf (buffer, format, args);
	va_end (args);
	write (fd, buffer, len);
}

/* printf() to a channel */
void cprintf (int chan, char *format, ...)
{
	char buffer[4096];
	int len;
	
	va_list args;
	va_start (args, format);
	len = vsprintf (buffer, format, args);
	va_end (args);
	write (channel[chan].fd, buffer, len);
}

/* printf() to everyone */
void wall (char *format, ...)
{
	char buffer[4096];
	int len, chan;
	
	va_list args;
	va_start (args, format);
	len = vsprintf (buffer, format, args);
	va_end (args);
	
	for (chan = 0; chan < CHANNEL_MAX; ++chan)
		if (channel[chan].fd)
			write (channel[chan].fd, buffer, len);
}

/* log() replaces fprintf (stder,...) */
void log (char *format, ...)
{
	char buffer[4096], *Time;
	time_t now;
	
	va_list args;
	va_start (args, format);
	vsprintf (buffer, format, args);
	va_end (args);
	
	time (&now);
	Time = ctime (&now);
	Time[strlen(Time)-1] = '\0'; // Kill the newline
	fprintf (stderr, "%d::%s %s\n", getpid(), Time, buffer);
}

/* Create a new connection */
int NewConnection (int fd, struct sockaddr_in address)
{
	int i;
	pid_t forkRet;
	
	for (i = 0 ; i < CHANNEL_MAX; ++i)
		if (channel[i].fd == 0)
			break;
		if (i == CHANNEL_MAX) {
			fdprintf (fd, "Xania is out of channels!\n\rTry again soon\n\r"); 
			close (fd);
			log ("Rejected connection from %s - out of channels", inet_ntoa (address.sin_addr));
			return -1;
		}
		
		channel[i].fd = fd;
		channel[i].hostname = strdup (inet_ntoa (address.sin_addr));
		channel[i].username = strdup ("(unknown)");
		channel[i].port = ntohs (address.sin_port);
		channel[i].netaddr = ntohl (address.sin_addr.s_addr);
		channel[i].nBytes = 0;
		channel[i].buffer = malloc (INCOMING_BUFFER_SIZE);
		channel[i].width = 80; channel[i].height = 24;
		channel[i].ansi = channel[i].gotTerm = false;
		channel[i].echoing = true;
		channel[i].firstReconnect = false;
		channel[i].connected = false;
		channel[i].authCharName[0] = '\0'; // Initially unauthenticated
		
		log ("[%d] Incoming connection from %s on fd %d", i, channel[i].hostname, channel[i].fd);
		
		/* Start the state machine */
		IncomingData (fd, NULL, 0);
		
		/* Fire off the query to the ident server */
		if (pipe(channel[i].identFd) == -1) {
			log ("Pipe failed - next message (unable to fork) is due to this");
			forkRet = -1;
			channel[i].identFd[0] = channel[i].identFd[1] = 0;
		} else {
			forkRet = fork();
		}
		switch (forkRet) {
		case 0:
			// Child process - go and do the lookup
			// First close the child's copy of the parent's fd
			close (channel[i].identFd[0]);
			log ("ID: on %d", channel[i].identFd[1]);
			channel[i].identFd[0] = 0;
			ProcessIdent(address, channel[i].identFd[1]);
			close (channel[i].identFd[1]);
			log ("ID: exitting");
			exit(0);
			break;
		case -1:
			// Error - unable to fork()
			log ("Unable to fork() an IdentD lookup process");
			channel[i].username = strdup("(unknown)");
			// Close up the pipe
			if (channel[i].identFd[0])
				close (channel[i].identFd[0]);
			if (channel[i].identFd[1])
				close (channel[i].identFd[1]);
			channel[i].identFd[0] = channel[i].identFd[1] = 0;
			break;
		default:
			// First close the parent's copy of the child's fd
			close (channel[i].identFd[1]);
			channel[i].identFd[1] = 0;
			channel[i].identPid = forkRet;
			FD_SET (channel[i].identFd[0], &ifds);
			log ("Forked ident process has PID %d on %d", forkRet, channel[i].identFd[0]);
			break;
			
		}
		
		
		FD_SET (fd, &ifds);
		
		// Tell them if the mud is down
		if (!connected)
			fdprintf (fd, "Xania is down at the moment - you will be connected as soon as it is up again.\n\r");
		
		SendConnectPacket (i);
		return i;
}

int HighestFd (void)
{
	int hFd = 0;
	int i;
	for (i = 0; i < CHANNEL_MAX; ++i) {
		if (channel[i].fd > hFd)
			hFd = channel[i].fd;
		if (channel[i].fd && 
			channel[i].identPid && 
			channel[i].identFd[0] > hFd)
			hFd = channel[i].identFd[0];
	}
	return hFd;
}

int FindChannel (int fd)
{
	int i;
	if (fd == 0) {
		log ("Panic!  fd==0 in FindChannel!  Bailing out with -1");
		return -1;
	}
	for (i = 0; i < CHANNEL_MAX; ++i)
		if (fd == channel[i].fd)
			return i;
		return -1;
}

void CloseConnection (int fd)
{
	int chan = FindChannel (fd);
	if (chan == -1) {
		log ("Erk - unable to find channel for fd %d", fd);
		return;
	}
	log ("[%d] Closing connection to %s@%s", chan, channel[chan].username, channel[chan].hostname);
	if (channel[chan].hostname)
		free (channel[chan].hostname);
	if (channel[chan].username)
		free (channel[chan].username);
	channel[chan].fd = 0;
	close (fd);
	FD_CLR (fd, &ifds);
	if (channel[chan].identPid) {
		if (kill (channel[chan].identPid, 9) < 0) {
			log ("Couldn't kill child process (ident process has already exitted)");
		}
	}
	if (channel[chan].identFd[0]) {
		if (channel[chan].identFd[0]) {
			FD_CLR (channel[chan].identFd[0], &ifds);
			close (channel[chan].identFd[0]);
		}
		if (channel[chan].identFd[1]) {
			close (channel[chan].identFd[1]);
		}
	}
}

/*********************************/

bool SupportsAnsi (const char *src)
{
	static const char *terms[] = {
		"xterm", "xterm-colour", "xterm-color",
			"ansi", "xterm-ansi", "vt100", "vt102",
			"vt220"
	};
	int i;
	for (i = 0; i < (sizeof (terms) / sizeof (terms[0])); ++i)
		if (!strcasecmp (src, terms[i]))
			return true;
		return false;
}

void SendCom (int fd, char a, char b)
{
	byte buf[4];
	buf[0] = IAC; buf[1] = a; buf[2] = b;
	write (fd, buf, 3);
}
void SendOpt (int fd, char a)
{
	byte buf[6];
	buf[0] = IAC; buf[1] = SB; buf[2] = a;
	buf[3] = TELQUAL_SEND;
	buf[4] = IAC;
	buf[5] = SE;
	write (fd, buf, 6);
}

void IncomingIdentInfo (int chan, int fd)
{
	// A response from the ident server
	int numBytes, ret;
	char buffer[1024], *spc;
    ret = read (fd, &numBytes, sizeof (numBytes));
	if (ret <= 0) {
		log ("[%d] IdentD pipe died on read (read returned %d)", chan, ret);
		FD_CLR (fd, &ifds);
		close (fd);
		channel[chan].identFd[0] = channel[chan].identFd[1] = 0;
		channel[chan].identPid = 0;
		FD_CLR (fd, &ifds);
		return;
	}
	if (numBytes >= 1024) {
		log ("Ident responded with >1k - possible spam attack");
		numBytes = 1023;
	}
	read (fd, buffer, numBytes);
	buffer[numBytes] = '\0'; // ensure zero-termedness
	FD_CLR (fd, &ifds);
	close (fd);
	channel[chan].identFd[0] = channel[chan].identFd[1] = 0;
	channel[chan].identPid = 0;
	
	spc = strchr (buffer, ' ');
	if (spc == NULL) {
		log ("[%d] Garbled response from ident server (%s)", chan, buffer);	  
	} else {
		// Free any previous data
		if (channel[chan].username)
			free (channel[chan].username);
		if (channel[chan].hostname)
			free (channel[chan].hostname);
		*spc = '\0';
		channel[chan].username = strdup (buffer);
		channel[chan].hostname = strdup (spc+1);
	}
	log ("[%d] User is %s@%s", chan, channel[chan].username, channel[chan].hostname);
	SendInfoPacket (chan);
}

void IncomingData (int fd, byte *buffer, int nBytes)
{
	int chan = FindChannel (fd);
	
	if (chan == -1) {
		log ("Oh dear - I got data on fd %d, but no channel!", fd);
		return;
	}
	
	if (buffer) {
		byte  *ptr;
		int nLeft, nRealBytes;
		
		/* Check for buffer overflow */
		if ((nBytes + channel[chan].nBytes) > INCOMING_BUFFER_SIZE) {
			char *ovf = ">>> Too much incoming data at once - PUT A LID ON IT!!\n\r";
			write (fd, ovf, strlen (ovf));
			channel[chan].nBytes = 0;
			return;
		}
		/* Add the data into the buffer */
		memcpy (&channel[chan].buffer[channel[chan].nBytes], buffer, nBytes);
		channel[chan].nBytes += nBytes;
		
		/* Scan through and remove telnet commands */
		nRealBytes = 0;
		for (ptr = channel[chan].buffer, nLeft = channel[chan].nBytes; 
	       nLeft; ) 
		   {
			   /* telnet command? */
			   if (*ptr == IAC) {
				   if (nLeft < 3) /* Is it too small to fit a whole command in? */
					   break;
				   switch (*(ptr+1)) {
				   case WILL:
					   switch (*(ptr+2)) {
					   case TELOPT_TTYPE:
						   // Check to see if we've already got the info
						   // Whoiseye's bloody DEC-TERM spams us multiply
						   // otherwise...
						   if (!channel[chan].gotTerm) {
							   channel[chan].gotTerm = TRUE;
							   SendOpt (fd, *(ptr+2));
						   }
						   break;
					   case TELOPT_NAWS:
						   // Do nothing for NAWS
						   break;
					   default:
						   SendCom (fd, DONT, *(ptr+2));
						   break;
					   }
					   memmove (ptr, ptr+3, nLeft - 3);
					   nLeft -= 3;
					   break;
					   case WONT:
						   SendCom (fd, DONT, *(ptr+2));
						   memmove (ptr, ptr+3, nLeft - 3);
						   nLeft -= 3;
						   break;
					   case DO:
						   if (ptr[2] == TELOPT_ECHO && channel[chan].echoing
							   == false) {
							   SendCom (fd, WILL, TELOPT_ECHO);
							   memmove (ptr, ptr+3, nLeft - 3);
							   nLeft -= 3;
						   } else {
							   SendCom (fd, WONT, *(ptr+2));
							   memmove (ptr, ptr+3, nLeft - 3);
							   nLeft -= 3;
						   }
						   break;
					   case DONT:
						   SendCom (fd, WONT, *(ptr+2));
						   memmove (ptr, ptr+3, nLeft - 3);
						   nLeft -= 3;
						   break;
					   case SB:
						   {
							   char sbType, sbWhat;
							   byte *eob, *p;
							   /* Enough room for an SB command? */
							   if (nLeft < 6) {
								   nLeft = 0;
								   break;
							   }
							   sbType = *(ptr + 2);
							   sbWhat = *(ptr + 3);
							   /* Now read up to the IAC SE */
							   eob = ptr + nLeft - 1;
							   for (p = ptr+4; p < eob; ++p)
								   if (*p == IAC && *(p+1) == SE)
									   break;
								   /* Found IAC SE? */
								   if (p == eob) {
									   /* No: skip it all */
									   nLeft = 0;
									   break;
								   }
								   *p = '\0';
								   /* Now to decide what to do with this new data */
								   switch (sbType) {
								   case TELOPT_TTYPE:
									   log ("[%d] TTYPE = %s", chan, ptr + 4);
									   if (SupportsAnsi (ptr+4))
										   channel[chan].ansi = true;
									   break;
								   case TELOPT_NAWS:
									   log ("[%d] NAWS = %dx%d", chan, ptr[4], ptr[6]);
									   channel[chan].width = ptr[4];
									   channel[chan].height = ptr[6];
									   break;
								   }
								   
								   /* Remember eob is 1 byte behind the end of buffer */
								   memmove (ptr, p + 2, nLeft - ((p+2) - ptr));
								   nLeft -= ((p+2) - ptr);
								   
								   break;
						   }
						   
					   default:
						   memmove (ptr, ptr + 2, nLeft - 2);
						   nLeft -=2;
						   break;
				   }
				   
			   } else {
				   ptr++;
				   nRealBytes++;
				   nLeft--;
			   }
	  }
	  /* Now to update the buffer stats */
	  channel[chan].nBytes = nRealBytes;
	  
	  /* Second pass - look for whole lines of text */
	  for (ptr = channel[chan].buffer, nLeft = channel[chan].nBytes;
	  nLeft;
	  ++ptr, --nLeft) {
		  char c;
		  switch (*ptr) {
		  case '\n':
		  case '\r':
			  c = *ptr;
			  *ptr = '\0';
			  /* Send it to the MUD */
			  if (connected && channel[chan].connected) {
				  Packet p;
				  p.type = PACKET_MESSAGE;
				  p.channel = chan;
				  p.nExtra = strlen (channel[chan].buffer) + 2;
				  write (mudFd, &p, sizeof (p));
				  write (mudFd, channel[chan].buffer, p.nExtra - 2);
				  write (mudFd, "\n\r", 2);
			  }  else {
				  // XXX NEED TO STORE DATA HERE
			  }
			  /* Check for \n\r or \r\n */		    
			  if (nLeft > 1 && (*(ptr+1) == '\r' || *(ptr+1) == '\n') &&
				  *(ptr+1) != c) {
				  memmove (channel[chan].buffer, ptr+2, nLeft-2);
				  nLeft--;
			  } else if (nLeft) {
				  memmove (channel[chan].buffer, ptr+1, nLeft-1);
			  } // else nLeft is zero, so we might as well avoid calling memmove(x,y, 0)
			  
			  /* The +1 at the top of the loop resets ptr to buffer */
			  ptr = channel[chan].buffer - 1;
			  break;
		  }
	  }
	  channel[chan].nBytes = ptr - channel[chan].buffer;
     } else {     /* Special case for initialisation */
		 /* Send all options out */
		 SendCom (fd, DO, TELOPT_TTYPE);
		 SendCom (fd, DO, TELOPT_NAWS);
		 SendCom (fd, WONT, TELOPT_ECHO);
     }
}

/*********************************/

/* The Ident Server */
void IdentPipeHandler (int ignored)
{
	(void)ignored;
	log ("Ident server dying on a PIPE");
	exit(1);
}

void ProcessIdent (struct sockaddr_in address, int outFd)
{
	char buf[1024];
	struct sockaddr_in authServer;
	struct hostent *ent;
	int sock;
	int nBytes;
	int i;
	const char *hostName;
	char *userName;
	unsigned short theirPort;
	
	/*
	* Firstly, and quite importantly, close all the descriptors we
	* don't need, so we don't keep open sockets past their useful
	* life.  This was a bug which meant ppl with firewalled auth
	* ports could keep other ppls connections open (nonresponding)
	*/
	for (i = 0; i < CHANNEL_MAX; ++i) {
		if (channel[i].fd)
			close (channel[i].fd);
		if (channel[i].identFd[0])
			close (channel[i].identFd[0]);
	}
	
	/*
	* Bail out on a PIPE - this means our parent has crashed
	*/
	signal (SIGPIPE, IdentPipeHandler);
	
	log ("ID: About to call gethostbyaddr...");
	ent = gethostbyaddr ((char *)&address.sin_addr, sizeof (address.sin_addr), AF_INET);
	log ("ID: returned from gethostbyaddr...");
	
	if (ent) {
		hostName = ent->h_name;
	} else {
		hostName = inet_ntoa (address.sin_addr);
	}
	
	/*
	* Now to deal with the pesky ident name
	*/
	theirPort = ntohs (address.sin_port);
	log ("ID: Attempting to connect to %s:113 : %d , %d", hostName, theirPort, port);
	
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		log ("ID: Unable to get socket");
		exit(1);
	}
	
	memset (&authServer, 0, sizeof (authServer));
	authServer.sin_addr.s_addr = address.sin_addr.s_addr;
	authServer.sin_port = htons (113);
	authServer.sin_family = PF_INET;
	
	userName = "(unknown)";
	if (connect (sock, (struct sockaddr *)&authServer, sizeof (authServer)) < 0) {
		log ("ID: Unable to get remote username");
	} else {
		char buf[1024];
		sprintf (buf, "%u , %u\n\r", theirPort, port);
		write (sock, buf, strlen(buf));
		nBytes = read (sock, buf, sizeof (buf));
		if (nBytes > 0) {
			char *foo = buf;
			do {
				while (isspace(*foo))
					foo++;
				userName = foo;
				foo = strchr (foo, ':');
				if (foo)
					foo++;
			} while (foo);
			foo = userName;
			while (!isspace(*foo))
				foo++;
			*foo = '\0';
		} else {
			log ("ID: Unable to read from socket");
		}
	}
	
	close (sock);
	
	nBytes = sprintf (buf, "%s %s", userName, hostName) + 1;
	write (outFd, &nBytes, sizeof (nBytes));
	if (write (outFd, buf, nBytes) < 0) {
		log ("ID: Unable to write to doorman - perhaps it crashed (%d, %s)", errno, sys_errlist[errno]);
	} else {
		log ("ID: Looked up %s@%s", userName, hostName);
	}
}

/*********************************/

void SendToMUD (Packet *p, void *payload)
{
	if (connected && mudFd != -1) {
		if (write (mudFd, p, sizeof (Packet)) < 0) {
			perror ("SendToMUD");
			exit(1);
		}
		if (p->nExtra)
			if (write (mudFd, payload, p->nExtra) < 0) {
				perror ("SendToMUD");
				exit(1);
			}
	}
}

/*
* Aborts a connection - closes the socket
* and tells the MUD that the channel has closed
*/
void AbortConnection(int fd)
{
	Packet p;
	p.type = PACKET_DISCONNECT;
	p.nExtra = 0;
	p.channel = FindChannel(fd);
	SendToMUD (&p, NULL);
	CloseConnection (fd);
}

void SendInfoPacket (int i)
{
	Packet p;
	char buffer[4096];
	InfoData *data = (InfoData *)buffer;
	char *hostname = channel[i].hostname,
		*username = channel[i].username ? channel[i].username : "(unknown)";
	p.type = PACKET_INFO;
	p.channel = i;
	p.nExtra = sizeof (InfoData) + strlen (hostname) + strlen (username) + 2;
	
	data->port 	= channel[i].port;
	data->netaddr     	= channel[i].netaddr;
	data->ansi		= channel[i].ansi;
	strcpy (data->data, username);
	strcpy (&data->data[strlen(username)+1], hostname);
	SendToMUD (&p, buffer);
}

void SendConnectPacket (int i)
{
	Packet p;
	if (channel[i].connected) {
		log ("[%d] Attempt to send connect packet for already-connected channel", i);
	} else if (connected && mudFd != -1) {
		// Have we already been authenticated?
		if (channel[i].authCharName[0] != '\0') {
			p.type = PACKET_RECONNECT;
			p.nExtra = strlen (channel[i].authCharName) + 1;
			p.channel = i;
			channel[i].connected = true;
			SendToMUD (&p, channel[i].authCharName);
			SendInfoPacket (i);
			log ("[%d] Sent reconnect packet to MUD for %s", i, channel[i].authCharName);
		} else {
			p.type = PACKET_CONNECT;
			p.nExtra = 0;
			p.channel = i;
			channel[i].connected = true;
			SendToMUD (&p, NULL);
			SendInfoPacket (i);
			log ("[%d] Sent connect packet to MUD", i);
		}
	}
}

void MudHasCrashed (int sig)
{
	int i;
	(void)sig;
	close (mudFd);
	FD_CLR (mudFd, &ifds);
	mudFd = -1;
	connected = false;
	waitingForInit = false;
	
	wall ("\n\rDoorman has lost connection to Xania.\n\r");
	
	for (i = 0; i < CHANNEL_MAX; ++i)
		if (channel[i].fd)
			channel[i].connected = false;
}

/*
* Deal with new connections from outside
*/
void ProcessNewConnection (void)
{
	struct sockaddr_in incoming;
	int len = sizeof (incoming);
	int newFD;
	int newChannel;
	
	newFD = accept (listenSock, (struct sockaddr *)&incoming, &len);
	if (newFD == -1) {
		log("Unable to accept new connection!");
		perror ("accept");
		return;
	}
	newChannel = NewConnection (newFD, incoming);
}

/*
* Attempt connection to the MUD
*/
void TryToConnectToXania (void)
{
	struct sockaddr_un xaniaAddr;
	
	lastConnected = time(NULL);
	
	memset (&xaniaAddr, 0, sizeof (xaniaAddr));
	xaniaAddr.sun_family = PF_UNIX;
	sprintf (xaniaAddr.sun_path, XANIA_FILE, port, getenv("USER") ? getenv("USER") : "unknown");
	
	/*
	* Create a socket for connection to Xania itself
	*/
	mudFd = socket (PF_UNIX, SOCK_STREAM, 0);
	if (mudFd < 0) {
		log ("Unable to create a socket to connect to Xania with!");
		exit(1);
	}
	log ("Descriptor %d is Xania", mudFd);
	
	if (connect (mudFd, (struct sockaddr *)&xaniaAddr, sizeof (xaniaAddr)) >= 0) {
		log ("Connected to Xania");
		waitingForInit = true;
		wall ("\n\rReconnected to Xania - Xania is still booting...\n\r");
		FD_SET (mudFd, &ifds);
	} else {
		int chanFirst;
		static const char *recon = "Attempting to reconnect to Xania";
		log ("Connection attempt to MUD failed.");
		for (chanFirst = 0; chanFirst < CHANNEL_MAX;
	       ++chanFirst) {
			   if (channel[chanFirst].fd &&
				   channel[chanFirst].firstReconnect == false) {
				   write (channel[chanFirst].fd, recon,
					   strlen(recon));
				   channel[chanFirst].firstReconnect = true;
			   }
		   }
		   wall (".");
		   log ("Closing descriptor %d (mud)", mudFd);
		   close (mudFd);
		   mudFd = -1;
	}
}

/*
* Deal with a request from Xania
*/
void ProcessMUDMessage (int fd)
{
	Packet p;
	int nBytes;
	int i;
	
	nBytes = read (fd, (char *)&p, sizeof (Packet));
	
	if (nBytes == 0) {
		log ("doorman::Connection to MUD lost");
		MudHasCrashed(0);
	} else {
		char payload[16384];
		if (p.nExtra)
			read (fd, payload, p.nExtra);
		switch (p.type) {
		case PACKET_MESSAGE:
			/* A message from the MUD */
			if (p.channel >= 0) {
				int nBytes;
				nBytes = write (channel[p.channel].fd, payload, p.nExtra);
				if (nBytes <= 0) {
					if (nBytes < 0) {
						log ("[%d] Received error %d (%s) on write - closing connection", 
							p.channel, errno, sys_errlist[errno]);
						
					}
					AbortConnection (channel[p.channel].fd);
				}
			}
			break;
		case PACKET_AUTHORIZED:
			/* A character has successfully logged in */
			if (p.nExtra < sizeof (channel[p.channel].authCharName)) {
				memcpy (channel[p.channel].authCharName, payload, p.nExtra);
				log ("[%d] Successfully authorized %s", p.channel, channel[p.channel].authCharName);
			} else {
				log ("[%d] Auth name too long!", 
					p.channel);
			}
			break;
		case PACKET_DISCONNECT:
			/* Kill off a channel */
			CloseConnection (channel[p.channel].fd);
			break;
		case PACKET_INIT:
			/* Xania has initialised */
			waitingForInit = false;
			connected = true;
			/* Now try to connect everyone who has
			been waiting */
			for (i = 0; i < CHANNEL_MAX; ++i) {
				if (channel[i].fd) {
					SendConnectPacket (i);
					channel[i].firstReconnect = false;
				}
			}
			break;
		case PACKET_ECHO_ON:
	       /* Xania wants text to be echoed by the
			client */
			SendCom(channel[p.channel].fd, WONT,
				TELOPT_ECHO);
			channel[p.channel].echoing = true;
			break; 
		case PACKET_ECHO_OFF:
	       /* Xania wants text to be echoed by the
			client */
			SendCom(channel[p.channel].fd, WILL,
				TELOPT_ECHO);
			channel[p.channel].echoing = false;
			break; 
		default:
			break;
		}
	}
}

/*
* The main loop
*/
void ExecuteServerLoop (void)
{
	struct timeval timeOut = { CONNECTION_WAIT_TIME, 0 };
	int i;
	int nFDs, highestClientFd;
	fd_set tempIfds, exIfds;
	int maxFd;
	
	/* 
	* Firstly, if not already connected to Xania, attempt connection
	*/
	if (!connected && !waitingForInit) {
		if ((time(NULL) - lastConnected) >= CONNECTION_WAIT_TIME) {
			TryToConnectToXania();
		}
	}
	
	/*
	* Select on a copy of the input fds
	* Also find the highest set FD
	*/
	maxFd = MAX (listenSock, mudFd);
	highestClientFd = HighestFd();
	maxFd = MAX (maxFd, highestClientFd);
	
	tempIfds = ifds;
	// Set up exIfds only for normal sockets and xania
	FD_ZERO (&exIfds);
	if (mudFd > 0)
		FD_SET (mudFd, &exIfds);
	for (i = 0; i < CHANNEL_MAX; ++i) {
		if (channel[i].fd)
			FD_SET (channel[i].fd, &exIfds);
	}
	
	nFDs = select (maxFd + 1, &tempIfds, NULL, &exIfds, &timeOut);
	if (nFDs == -1 && errno != EINTR) {
		log ("Unable to select()!");
		perror ("select");
		exit (1);
	}
	/* Anything happened in the last 10 seconds? */
	if (nFDs <= 0)
		return;
	/* Has the MUD crashed out? */
	if (mudFd != -1 && FD_ISSET(mudFd, &exIfds)) {
		MudHasCrashed(0);
		return; /* Prevent falling into packet handling */
	}
	for (i = 0; i <= maxFd; ++i) {
		/* Kick out the freaky folks */
		if (FD_ISSET(i, &exIfds)) {
			AbortConnection (i);
		} else if (FD_ISSET(i, &tempIfds)) { // Pending incoming data
			/* Is it the listening connection? */
			if (i == listenSock) {
				ProcessNewConnection();
			} else if (i == mudFd) {
				ProcessMUDMessage(i);
			} else {
				/* It's a message from a user or ident - despatch it */
				int chan;
				for (chan = 0; chan < CHANNEL_MAX; ++chan) {
					if (channel[chan].fd &&
						channel[chan].identPid &&
						channel[chan].identFd[0] == i) {
						log ("[%d on %d] Incoming IdentD info", chan, i);
						IncomingIdentInfo (chan, i);
						break;
					}
				}
				if (chan == CHANNEL_MAX) {
					int nBytes;
					char buf[256];
					do {
						nBytes = read (i, buf, sizeof (buf));
						if (nBytes <= 0) {
							Packet p;
							p.type = PACKET_DISCONNECT;
							p.nExtra = 0;
							p.channel = FindChannel(i);
							SendToMUD (&p, NULL);
							if (nBytes < 0) {
								log ("[%d] Received error %d (%s) on read - closing connection", 
									p.channel, errno, sys_errlist[errno]);
							}
							AbortConnection (i);
						} else {
							if (nBytes > 0)
								IncomingData (i, buf, nBytes);
						}
						// See how many more bytes we can read
						nBytes = 0; // XXX Need to look this ioctl up
					} while (nBytes > 0);
				}
			}
		}
	}
}

/*
* Clean up any dead ident processes
*/
void CheckForDeadIdents (void)
{
	pid_t waiter;
	int status;
	
	do {
		waiter = waitpid (-1, &status, WNOHANG);
		if (waiter==-1) {
			break; // Usually 'no child processes'
		}
		if (waiter) {
			int chan;
			// Clear the zombie status
			waitpid (waiter, NULL, 0);
			log ("process %d died", waiter);
			// Find the corresponding channel, if still present
			for (chan = 0; chan < CHANNEL_MAX; ++chan) {
				if (channel[chan].fd &&
					channel[chan].identPid == waiter)
					break;
			}
			if (chan != CHANNEL_MAX) {
				// Did the process exit abnormally?
				if (!(WIFEXITED(status))) {
					log ("Zombie reaper: process on channel %d died abnormally", chan);
					close (channel[chan].identFd[0]);
					close (channel[chan].identFd[1]);
					FD_CLR (channel[chan].identFd[0], &ifds);
					channel[chan].identPid = 0;
					channel[chan].identFd[0] =
						channel[chan].identFd[1] = 0;
				} else {
					if (channel[chan].identFd[0] ||
						channel[chan].identFd[1]) {
						log ("Zombie reaper: A process died, and didn't necessarily close up after itself");
					}
				}
			}
		}
	} while (waiter);
}

// Our luvverly options go here :
struct option OurOptions[] =
{
	{ "port", 1, NULL, 'p' },
	{ "debug",0, &debug, 1 },
	{ "help", 0, NULL, 'h' },
	{ NULL,   0, NULL, 0 }
};

void usage(void)
{
	fprintf (stderr, "Usage: doorman [-h | --help] [-d | --debug] [-p | --port port] [port]\n\r");
}

// Here we go:

int main (int argc, char *argv[])
{
	struct protoent *tcpProto = getprotobyname ("tcp");
	int yup = 1;
	struct linger linger = { 1, 2 }; // linger seconds
	struct sockaddr_in sin;
	
	/*
	* Parse any arguments
	*/
	port = 9000;
	for (;;) {
		int optType;
		int index = 0;
		
		optType = getopt_long (argc, argv, "dp:h", 
			OurOptions, &index);
		if (optType == -1)
			break;
		switch (optType) {
		case 'p':
			port = atoi (optarg);
			if (port <= 0) {
				fprintf (stderr, "Invalid port '%s'\n\r", optarg);
				usage();
				exit(1);
			}	       
			break;
		case 'd':
			debug = 1;
			break;
		case 'h':
			usage();
			exit(0);
			break;
		}
	}
	
	if (optind == (argc-1)) {
		port = atoi (argv[optind]);
		if (port <= 0) {
			fprintf (stderr, "Invalid port '%s'\n\r", argv[optind]);
			usage();
			exit(1);
		}	       
	} else if (optind < (argc-1)) {
		usage();
		exit(1);
	}
	
	/* 
	* Initialise the channel array
	*/
	memset (channel, 0, sizeof (channel));
	
	/*
	* Bit of 'Hello Mum'
	*/
	log ("Doorman version 0.2 starting up");
	log ("Attempting to bind to port %d", port);
	
	/*
	* Turn on core dumping under debug
	*/
	if (debug) {
		struct rlimit coreLimit = 
		{ 0, 16*1024*1024 };
		int ret;
		log ("Debugging enabled - core limit set to 16Mb");
		ret = setrlimit (RLIMIT_CORE, &coreLimit);
		if (ret < 0) {
			log ("Unable to set limit - %d ('%s')", errno, sys_errlist[errno]);
		}
	}
	
	/*
	* Create a tcp socket, bind and listen on it
	*/
	if (tcpProto == NULL) {
		log ("Unable to get TCP number!");
		exit(1);
	}
	listenSock = socket (PF_INET, SOCK_STREAM, tcpProto->p_proto);
	if (listenSock < 0) {
		log ("Unable to create a socket!");
		exit(1);
	}
	
	/*
	* Prevent crashing on SIGPIPE if the MUD goes down, or if a connection
	* goes funny
	*/
	signal (SIGPIPE, SIG_IGN);
	
	/*
	* Set up a few options
	*/
	if (setsockopt (listenSock, SOL_SOCKET, SO_REUSEADDR, &yup, sizeof (yup)) < 0) {
		log ("Unable to listen!");
		exit(1);
	}
	if (setsockopt (listenSock, SOL_SOCKET, SO_LINGER, &linger, sizeof (linger)) < 0) {
		log ("Unable to listen!");
		exit(1);
	}
	
	memset (&sin, 0, sizeof (sin));
	sin.sin_family = PF_INET;
	sin.sin_port = htons (port);
	sin.sin_addr.s_addr = htonl (INADDR_ANY);
	if (bind (listenSock, (struct sockaddr *)&sin, sizeof (sin)) < 0) {
		log ("Unable to bind socket!");
		exit(1);
	}
	if (listen (listenSock, 4) < 0) {
		log ("Unable to listen!");
		exit(1);
	}
	
	log ("Bound to socket succesfully.");
	
	/*
	* Start the ball rolling
	*/
	FD_ZERO (&ifds);
	FD_SET (listenSock, &ifds);
	lastConnected = 0;
	mudFd = -1;
	
	log ("Doorman is ready to rock on port %d", port);
	
	/*
	* Loop forever!
	*/
	for (;;) 
	{
		// Do all the nitty-gritties
		ExecuteServerLoop();
		// Ensure we don't leave zombie processes hanging around
		CheckForDeadIdents();
	}
}
