#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../util.h"

typedef struct {
	int port;
	int socket;
	struct sockaddr_in serverAddress;
	char *hostAddress;
	char *directory;
	int directoryLength;
} UDPServer;

int
main(int argc, char **argv)
{
	int clientlen;
	struct sockaddr_in clientAddress;
	struct hostent *hostp;
	int numBytesReceived;
	UDPServer server;

    if (argc != 3) {
		fprintf(stderr, "Usage: %s <Server Port> <Directory>\n", argv[0]);
		exit(1);
    }

    server.port = atoi(argv[1]);
    server.directoryLength = strlen(argv[2]);
    server.directory = (char *) malloc(server.directoryLength * sizeof(char));
    strncpy(server.directory, argv[2], server.directoryLength);

    if ((server.socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LogFatal("socket() failed");
    }

    // set address reuse
    int optval = 1;
    setsockopt(server.socket, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval , sizeof(int));

    bzero((char *) &server.serverAddress, sizeof(server.serverAddress));
    server.serverAddress.sin_family = AF_INET;
    server.serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    server.serverAddress.sin_port = htons(server.port);

    if (bind(server.socket, (struct socketaddr *) &server.serverAddress, sizeof(server.serverAddress)) < 0) {
    	LogFatal("bind() failed");
    }

    clientlen = sizeof(clientAddress);
    char buffer[RCVBUFSIZE];
    for (;;) {
    	bzero(buffer, RCVBUFSIZE);
    	numBytesReceived = recvfrom(server.socket, buffer, RCVBUFSIZE, 0, 
    		(struct sockaddr *) &clientAddress, &clientlen);
    	if (numBytesReceived < 0) {
    		LogFatal("recvfrom() failed");
    	}

    	if (buffer < RCVBUFSIZE) {
            buffer[recvMsgSize] = 0; // null terminator
        } else {
            // TODO: read more and expand the buffer
        }

        FILE *fp = fopen(buffer, "r");
        if (fp == NULL) {
#if DEBUG
        	fprintf(stderr, "Error opening file %s\n", buffer);
#endif
			char *message;
            asprintf(&message, "File %s does not exist", buffer);

            int length = strlen(message);
            if (sendto(server.socket, message, strlen(message), 0, 
            	(struct sockaddr *) &clientAddress, clientlen) < 0) {
                LogFatal("sendto() failed");
            }
        } else {
        	char fileBuffer[FILE_BUFFER_LENGTH];
            bzero(fileBuffer, FILE_BUFFER_LENGTH);
            size_t numBytesRead = 0;
            for (;;) {
                fprintf(stderr, "...\n");

                numBytesRead = fread(fileBuffer, 1, FILE_BUFFER_LENGTH, fp);
                if (sendto(server.socket, fileBuffer, strlen(fileBuffer), 0, 
                	(struct sockaddr *) &clientAddress, clientlen) < 0) {
                    LogFatal("Error sending data to the client\n");
                }

                if (numBytesRead != FILE_BUFFER_LENGTH) {
                    break;
                }
            }
        }
    }

    close(server.socket);
    
    free(server.directory);

    return 0;
}