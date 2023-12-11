#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#include <string.h>

#define MAX_LENGTH 500

#define FTP_PORT 21
#define SERVER_ADDR "192.168.28.96"

#define h_addr h_addr_list[0]

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
