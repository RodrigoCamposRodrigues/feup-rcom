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
    server_addr.sin_port = htons(SERVER_PORT);        /*server TCP port must be network byte ordered */

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

    char userInput[5+strlen(user)+1];
    char passwordInput[5+strlen(password)+1];
    char serverResponse[500];

    return 0;
}

int handleResponses(const int socket, char *buffer) {
    return 0;
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

    // ... 
    return 0;
}