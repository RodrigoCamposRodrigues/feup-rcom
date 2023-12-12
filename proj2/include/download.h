#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#define MAX_LENGTH 500

#define FTP_PORT 21

// Server responses
#define SV_AUTHENTICATE 220
#define SV_PASSWD 331
#define SV_AUTHSUCCESS 230
#define SV_QUIT 221
#define SV_PASSIVE 227
#define SV_TRANSFER_START 150
#define SV_TRANSFER_COMPLETE 226

// Default login credentials
#define DEFAULT_USER "anonymous"
#define DEFAULT_PASSWD "password"

// URL structure
struct URL {
    char host[MAX_LENGTH];
    char path[MAX_LENGTH];
    char filename[MAX_LENGTH];
    char user[MAX_LENGTH];
    char passwd[MAX_LENGTH];
    char ip[MAX_LENGTH];
};

typedef enum {
    FTP_START_REPLY,
    FTP_END_REPLY,
    FTP_SINGLE_LINE,
    FTP_MULTI_LINE,
} FTPstate;

// Function to get IP from hostname
char* getIPAddress(const char* host);

// Create and connect socket
int createAndConnectSocket(char *ip, int port);

// Handle replies from server
int handleResponses(const int socket, char *buffer);

// Parse URL
int parseURL(const char *url, struct URL *parsed_url);

// Authenticate and connect to the server
int authenticateAndConnect(const int socket, const char* user, const char* password);

// Enable passive mode for data transfer
int enablePassiveMode(const int socket, char *ip, int *port);

// Request and get a file from the server
int requestAndGetFile(const int CommandSocket, const int DataSocket, char *path, char *filename);

// Close the FTP connection
int CloseConnection(const int CommandSocket, const int DataSocket);

#endif  // DOWNLOAD_H
