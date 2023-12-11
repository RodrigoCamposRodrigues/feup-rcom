#include "include/download.h"

// Function to get IP from hostname
char* getIPAddress(const char* host) {
    struct hostent *h;

    if ((h = gethostbyname(host)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }

    return strdup(inet_ntoa(*((struct in_addr *) h->h_addr)));
}

int createAndConnectSocket(char *ip) {
    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(FTP_PORT);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }

    return sockfd;
}

// Handle replies from server, take into account if they are single or multiline
int handleResponses(const int socket, char *buffer) {
    
    char byte;
    int i = 0;
    int respCode = 0;
    FTPstate state = FTP_START_REPLY;

    memset(buffer, 0, MAX_LENGTH);

    while (state != FTP_END_REPLY) {
        if (read(socket, &byte, 1) < 0) {
            perror("read()");
            exit(-1);
        }

        switch (state) {
            case FTP_START_REPLY:
                if (byte == ' ') state = FTP_SINGLE_LINE;
                else if (byte == '-') state = FTP_MULTI_LINE;
                else if (byte == '\n') state = FTP_END_REPLY;
                else buffer[i++] = byte;
                break;
            case FTP_SINGLE_LINE:
                if (byte == '\n') state = FTP_END_REPLY;
                else buffer[i++] = byte;
                break;
            case FTP_MULTI_LINE:
                if (byte == '\n') {
                    memset(buffer, 0, MAX_LENGTH);
                    i = 0;
                    state = FTP_START_REPLY;
                }
                else buffer[i++] = byte;
                break;
            case FTP_END_REPLY:
                break;
            default:
                break;
        }
    }

    sscanf(buffer, "%d", &respCode);
    return respCode;
}

int parseURL(const char *url, struct URL *parsed_url) {

    memset(parsed_url, 0, sizeof(struct URL));

    // Check if it starts with ftp://
    if (strncmp(url, "ftp://", 6) != 0) {
        printf("Invalid URL\n");
        return -1;
    }

    const char *user_start = url + 6;

    const char *user_end = strchr(user_start, ':');
    const char *password_start = user_end ? user_end + 1 : NULL;

    const char *password_end = strchr(user_start, '@');
    const char *host_start = password_end ? password_end + 1 : user_start;

    const char *host_end = strchr(password_start ? password_start : user_start, '/');
    const char *path_start = host_end ? host_end + 1 : NULL;

    const char *path_end = strchr(host_start, '\0');

    const char *last_slash = strrchr(path_start ? path_start : host_start, '/');
    const char *filename_start = last_slash ? last_slash + 1 : path_start;

    // Extract components
    size_t user_len = user_end ? user_end - user_start : 0;
    size_t password_len = password_end ? password_end - password_start : 0;
    size_t host_len = host_end ? host_end - host_start : 0;
    size_t path_len = path_end ? path_end - path_start : 0;
    size_t filename_len = strlen(filename_start);

    strncpy(parsed_url->user, user_start, user_len);
    parsed_url->user[user_len] = '\0';

    strncpy(parsed_url->passwd, password_start, password_len);
    parsed_url->passwd[password_len] = '\0';

    strncpy(parsed_url->host, host_start, host_len);
    parsed_url->host[host_len] = '\0';

    strncpy(parsed_url->path, path_start, path_len);
    parsed_url->path[path_len] = '\0';

    strncpy(parsed_url->filename, filename_start, filename_len);
    parsed_url->filename[filename_len] = '\0';

    // Resolve the IP address and fill in the URL structure
    char *ip = getIPAddress(parsed_url->host);
    strncpy(parsed_url->ip, ip, strlen(ip));

    // Assign default values to user and password if not present
    if (strlen(parsed_url->user) == 0) {
        strcpy(parsed_url->user, DEFAULT_USER);
        strcpy(parsed_url->passwd, DEFAULT_PASSWD);
    }

    return !(strlen(parsed_url->user) && strlen(parsed_url->passwd) && strlen(parsed_url->host) && strlen(parsed_url->path) && strlen(parsed_url->filename));
}

int authenticateAndConnect(const int socket, const char* user, const char* password) {

    // Store user and password commands
    char userInput[5+strlen(user)+1];
    sprintf(userInput, "USER %s\n", user);

    char passwordInput[5+strlen(password)+1];
    sprintf(passwordInput, "PASS %s\n", password);

    char serverResponse[MAX_LENGTH];

    // Write user command to socket
    if (write(socket, userInput, strlen(userInput)) < 0) {
        perror("write()");
        exit(-1);
    }

    // Read server response
    if (handleResponses(socket, serverResponse) != SV_PASSWD) {
        printf("Invalid username '%s'. Aborting.\n", user);
        exit(-1);
    }

    // Write password command to socket
    if (write(socket, passwordInput, strlen(passwordInput)) < 0) {
        perror("write()");
        exit(-1);
    }

    return handleResponses(socket, serverResponse);
}

int main(int argc, char **argv) {

    // Check if the URL is provided
    if (argc != 2) {
        printf("Usage: %s <URL>\n", argv[0]);
        exit(-1);
    }

    // Parse the URL
    struct URL url;
    memset(&url, 0, sizeof(url));
    if (parseURL(argv[1], &url) != 0) {
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    // Print fields
    printf("User: %s\n", url.user);
    printf("Password: %s\n", url.passwd);
    printf("Host: %s\n", url.host);
    printf("Path: %s\n", url.path);
    printf("Filename: %s\n", url.filename);
    printf("IP: %s\n", url.ip);

    char serverResponse[MAX_LENGTH];

    // Create Command socket
    int CommandSocket = createAndConnectSocket(url.ip);
    if (handleResponses(CommandSocket, serverResponse) != SV_AUTHENTICATE || CommandSocket < 0) {
        printf("Error connecting to %s. Aborting.\n", url.host);
        exit(-1);
    }

    // Attempt to authenticate
    if (authenticateAndConnect(CommandSocket, url.user, url.passwd) != SV_AUTHSUCCESS) {
        printf("Error authenticating to %s. Aborting.\n", url.host);
        exit(-1);
    }

    return 0;
}