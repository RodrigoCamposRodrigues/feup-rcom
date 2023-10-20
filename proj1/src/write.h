#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1
#define FLAG 0x7E
#define A_SENDER 0x03
#define A_RECEIVER 0x01
#define I0 0x00
#define I1 0x40
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81
#define UA 0x07
#define SET 0x03

#define BUF_SIZE 1000

void state_machine(unsigned char curr_byte, unsigned char A, unsigned char C, unsigned char BCC1);

void alarmHandler(int signal);

int llopen(int fd);

int llwrite(int fd, unsigned char *buffer, int length);

char* my_read_file(const char* filename, long *file_size);

unsigned char *buildFrameInfo(unsigned char *buffer, int length);

unsigned char buildBCC2(unsigned char *buffer, int length);








