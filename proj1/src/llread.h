#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define FALSE 0
#define TRUE 1

#define BAUDRATE B38400
#define BUF_SIZE 1000
#define FLAG 0x7E
#define A_SENDER 0x03
#define A_RECEIVER 0x01
#define C_SET 0x03
#define C_UA 0x07
#define BCC_SET (A^C_SET)
#define BCC_UA (A^C_UA)
#define C0 0x00
#define C1 0x40
#define RR_C0 0x05
#define RR_C1 0x85
#define REJ_C0 0x01
#define REJ_C1 0x81
#define DISC_C 0x0B

#define ESCAPE 0x7D
#define ESCAPE_FLAG 0x5E
#define ESCAPE_ESCAPE 0x5D

unsigned char llread(int fd, unsigned char *buffer, int sizebuf);

int SendSupervisionFrame(int fd, unsigned char C);

int ReadSupervisionFrame(int fd, unsigned char C);

int checkBCC2(unsigned char *data, int dataSize);

int llopen(int fd);

void llclose(int fd);