#include "llread.h"

struct termios oldtio;
struct termios newtio;

int state = 0;

int main(int argc, char *argv[]) {
    int fd;
    int sizebuf = 0;
    unsigned char buffer[BUF_SIZE];
    

    // Check for correct number of arguments and serial port name.
    const char *serialPortName = argv[1];
    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty.
    fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    //unsigned char buffer[BUF_SIZE];
    llopen(fd);
    llread(fd, &buffer, sizebuf);

    return 0;
}


unsigned char llread(int fd, unsigned char *buffer, int sizebuf) {
    printf("Entered llread.\n");
    volatile int STATE_FINAL = FALSE;
    unsigned char* file_content = (unsigned char*) malloc(sizebuf);
    int n_frame = 0;
    state = 0;
    int index = 0;

    while (!STATE_FINAL) {
        read(fd, buffer, 1);
        // State machine for I frame
        switch (state)
        {
        case 0:
            if (buffer[0] == FLAG) state = 1;
            else state = 0;
            break;
        case 1:
            if (buffer[0] == A_SENDER) state = 2;
            else if (buffer[0] == FLAG) state = 1;
            else state = 0;
            break;
        case 2:
            if (buffer[0] == C0) {
                state = 3;
                n_frame = 0;
            }
            else if (buffer[0] == C1) {
                state = 3;
                n_frame = 1;
            }
            else if (buffer[0] == FLAG) state = 1;
            else state = 0;
            break;
        case 3:
            if (buffer[0] == (A_SENDER^C0) || buffer[0] == (A_SENDER^C1)) state = 4;
            else if (buffer[0] == FLAG) state = 1;
            else state = 0;
            break;
        case 4:
        //...
            if (buffer[0] == FLAG) {

                unsigned bcc2 = file_content[index -1];
                index--;
                file_content[index] = '\0';
                unsigned char checker = file_content[0];

                for(int j = 1; j < index; j++){
                    checker ^= file_content[j];
                }

                if (bcc2 == checker){
                    SendSupervisionFrame(fd, RR_C1);
                    STATE_FINAL = TRUE;
                    state = 0;
                }

                // if (checkBCC2(buffer, sizebuf)) {
                //     if (n_frame == 0) {
                //         SendSupervisionFrame(fd, RR_C1);
                //     }
                //     else {
                //         SendSupervisionFrame(fd, RR_C0);
                //     }
                // }
                // else {
                //     if (n_frame == 0) {
                //         SendSupervisionFrame(fd, REJ_C0);
                //     }                    
                //     else {
                //         SendSupervisionFrame(fd, REJ_C1);
                //     }
                // }
                
            }
            else{
                file_content[index++] = buffer[0];
                printf("index: %d\n", index);
            }
            // else if (buffer[0] = ESCAPE) {
            //    state = 5;
            //}
            // else {
            //    buffer[sizebuf] = current_byte;
            //    sizebuf++;
            //}
            
            break;
        case 5:
            if (buffer[0] == ESCAPE_FLAG) {
                buffer[sizebuf] = FLAG;
                sizebuf++;
            }
            else if (buffer[0] == ESCAPE_ESCAPE) {
                buffer[sizebuf] = ESCAPE;
                sizebuf++;
            }
            else {
                perror("Invalid escape sequence.\n");
                exit(-1);
            }
            state = 4;
            break;
        }
    }
    // Print file_content
    for (int i = 0; i < index; i++) {
        printf("%c", file_content[i]);
    }
    return file_content;

    free(file_content);
}

int llopen(int fd) {
    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0.1; // Inter-character timer unused
    newtio.c_cc[VMIN] = 1;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Check for first set frame
    if (ReadSupervisionFrame(fd, C_SET)) {
        printf("Successfully received SET frame.\n");
        printf("Sending UA frame.\n");
        SendSupervisionFrame(fd, C_UA); // Send UA frame
        printf("Successfully sent UA frame.\n");
        return 0;
    }
    return -1;
}

void llclose(int fd) {
    // Read DISC_C, send it back and read UA frame.
    ReadSupervisionFrame(fd, DISC_C);
    printf("Successfully received DISC frame.\n");
    printf("Sending DISC frame.\n");        
    SendSupervisionFrame(fd, DISC_C);
    printf("Successfully sent DISC frame.\n");
    ReadSupervisionFrame(fd, C_UA);
    printf("Successfully received UA frame.\n");

    // Restore old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
}

int SendSupervisionFrame(int fd, unsigned char C) {
    unsigned char FRAME[5];
    FRAME[0] = FLAG;
    FRAME[1] = 0x01;
    FRAME[2] = C;
    FRAME[3] = FRAME[1]^FRAME[2];
    FRAME[4] = FLAG;
    write(fd, FRAME, 5);
    return 0;
}

int ReadSupervisionFrame(int fd, unsigned char C) {
    unsigned char buf[BUF_SIZE] = {0};

    while (state != 5) {
        read(fd, buf, 1);
        printf("Current byte: %x\n", buf[0]);
        switch (state) {
        case 0:
            if (buf[0] == FLAG) state = 1;
            else state = 0;
            break;
        case 1:
            if (buf[0] == A_SENDER) state = 2;
            else if (buf[0] == FLAG) state = 1;
            else state = 0;
            break;
        case 2:
            if (buf[0] == C) state = 3;
            else if (buf[0] == FLAG) state = 1;
            else state = 0;
            break;
        case 3:
            if (buf[0] == (A_SENDER^C)) state = 4;
            else state = 0;
            break;
        case 4:
            if (buf[0] == FLAG) {
                state = 5;
            }
            else {
                state = 0;
            }
            break;
        }
    }
    return TRUE;
}

int checkBCC2(unsigned char *data, int dataSize) {
    // Check BCC2 by XORing all bytes in data
    unsigned char BCC2 = data[0];
    for (int i = 1; i < dataSize - 1; i++) {
        BCC2 ^= data[i];
    }
    return (BCC2 == data[dataSize - 1]);
}
