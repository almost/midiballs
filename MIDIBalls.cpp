// MIDIBalls, OSX MIDI->OSC bridge for Felix's Machines
// tom@almostobsolete.net
// Thomas Parslow
// Quick, dirty and feature-lite, but it does what we need.
// ___________________________________________________________________________________________

#include <CoreMIDI/MIDIServices.h>
#include <CoreFoundation/CFRunLoop.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h> /* for close() for socket */
#include <arpa/inet.h>



MIDIPortRef		gOutPort = NULL;
MIDIEndpointRef	gDest = NULL;
int				gChannel = 0;

int sock;
struct sockaddr_in addr;

#define ALIGN4(x) if (x & 3) {x += 4-(x & 3);}

static void	MyReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
		MIDIPacket *packet = (MIDIPacket *)pktlist->packet;	// remove const (!)
		for (unsigned int j = 0; j < pktlist->numPackets; ++j) {
			for (int i = 0; i < packet->length; ++i) {
				// send note-on and note-offs
				if (packet->data[i] >= 0x80 && packet->data[i] < 0xF0) {
					// Make an OSC packet, I only every want to send this one type so just do it manually
					int bytes_sent;
					char buffer[200];
                                        memset(buffer, 0, 200);
                                        int offset=0;
                                        char *command;
                                        if (packet->data[i] >= 0x90 && packet->data[i+2] > 0) {
                                          command = "on";
                                        } else {
                                          command = "off";
                                        }
                                        offset+= (sprintf(buffer+offset,"/MIDIBalls/%d/%d/%s", 
                                                          (packet->data[i] & 0x0F)+1, 
                                                          packet->data[i+1],
                                                          command)+1);
                                        ALIGN4(offset);
                                        offset+= (sprintf(buffer+offset,",i")+1);
                                        ALIGN4(offset);
                                        *((int*)(buffer+offset)) = htonl(packet->data[i+2]);
                                        offset += 4;
					bytes_sent = sendto(sock, buffer, offset, 0,(struct sockaddr*) &addr, sizeof(struct sockaddr_in) );
					if(bytes_sent < 0)
						printf("Error sending packet: %s\n", strerror(errno) );
				}
			}

			packet = MIDIPacketNext(packet);
		}	
}

int		main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("usage: midiballs ip port\n");
		return 0;
	}
	
	int port;
	sscanf(argv[2], "%d", &port);
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_port = htons(port);

    // Create sockets for OSC sending
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(-1 == sock) /* if socket failed to initialize, exit */
        {
		printf("Error Creating Socket");
		return 0;
	}


	// create client and destinations
	MIDIClientRef client = NULL;
	MIDIClientCreate(CFSTR("MIDI Balls"), NULL, NULL, &client);

	MIDIEndpointRef dest = NULL;
	MIDIDestinationCreate(client, CFSTR("MIDIBalls"), MyReadProc, NULL, &dest);

	printf("MIDIBalls ACTIVATE!\n");
	CFRunLoopRun();
	close(sock); /* close the socket */
	// run until aborted with control-C

	return 0;
}
