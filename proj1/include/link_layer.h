// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

typedef enum
{
    LlTx,
    LlRx
} LinkLayerRole;

typedef enum
{
   START,
   FLAG_RECEIVED,
   A_RECEIVED,
   C_RECEIVED,
   BCC1_CHECKED,
   FOUND_ESC,
   READING_DATA,
   STOP_READING,
   DISCONNECTED,
   BCC2_CHECKED
} LinkLayerState;

typedef struct
{
    const char *serialPort;
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

// MISC
#define FALSE 0
#define TRUE 1

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
#define DISC 0x0B
#define ESCAPE 0x7D
#define ESCAPE_FLAG 0x5E
#define ESCAPE_ESCAPE 0x5D


// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);

int connect_to_serialPort(const char *serialPort);

void alarmHandler(int signal);

unsigned char *buildSupervisionFrame(unsigned char A, unsigned char C);

void state_machine_read_supervision_frames(unsigned char curr_byte, unsigned char A, unsigned char C, LinkLayerState *state);

int buildFrameInfo(unsigned char *frame_info, unsigned char *buffer, int bufSize);

unsigned char buildBCC2(unsigned char *buffer, int length);

void state_machine_read_control_frames(unsigned char curr_byte, unsigned char A, unsigned char C, LinkLayerState *state);

int checkBCC2(unsigned char *packet, int packetSize, unsigned char bcc2);

#endif // _LINK_LAYER_H_