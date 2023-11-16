// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include "link_layer.h"

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.

// Application layer main function.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

int sendControlPacket(const unsigned int C, int file_size, const char *filename);

int buildDataPacket(unsigned char sequence, unsigned char *data_to_send, int data_size_to_send, unsigned char *data_packet);

int parseControlPacket(unsigned char *received_packet, int received_packet_size);

int packetRecognition(unsigned char *received_packet, int received_packet_size, FILE *new_file, unsigned char sequence_number_checker, int *flag_to_change_sequence_number);

#endif // _APPLICATION_LAYER_H_
