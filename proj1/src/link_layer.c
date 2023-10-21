// Link layer protocol implementation

#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int alarmCount = 0;
int alarm_reached_end = TRUE;
int timeout = 0;
int retransmissions = 0;
int STOP = FALSE;
unsigned char info_frame_number = I0;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    LinkLayerState state = START;
    
    int fd = connect_to_serialPort(connectionParameters.serialPort);

    timeout = connectionParameters.timeout;
    retransmissions = connectionParameters.nRetransmissions;

    switch(connectionParameters.role){
        
        case LlTx: {
            
            unsigned char SET_frame = buildSupervisionFrame(A_SENDER, SET);

            (void)signal(SIGALRM, alarmHandler);

            while(retransmissions != 0 && state != STOP_READING){

                if(alarm_reached_end == TRUE){ 
                    int bytes_written = write(fd, SET_frame, 5);
                    printf("Sent SET frame. Bytes written: %d\n", bytes_written);
                    retransmissions--;
                    alarm(timeout);
                    alarm_reached_end = FALSE;
                }

                unsigned char received_byte; 

                while(alarm_reached_end != TRUE && state != STOP_READING){
                    int bytes_read = read(fd, received_byte, 1);

                    if(bytes_read > 0){
                        state_machine_read_supervision_frames(received_byte, A_RECEIVER, UA, &state);
                    }
                    else break;
                }

            }
            
            alarm_reached_end = TRUE;
            retransmissions = connectionParameters.nRetransmissions;

            if(state != STOP_READING){
                printf("Couldn't read UA frame.\n");
                return -1;
            }
            else{
                printf("Successfully read UA frame!\n");
                return fd;
            }
        }

        case LlRx: {
            //TODO implement receiver llopen

            unsigned char received_byte;

            while(state != STOP_READING){

                int bytes_read = read(fd, received_byte, 1);

                if(bytes_read > 0){
                    state_machine_read_supervision_frames(received_byte, A_RECEIVER, UA, &state);
                }
                else break;
            }

            printf("Successfully read SET frame!\n");
            
            unsigned char UA_frame = buildSupervisionFrame(A_RECEIVER, UA);
            int bytes_written = write(fd, UA_frame, 5);
            printf("Sent UA frame. Bytes written: %d\n", bytes_written);

        }

        default:
            return -1;
            break;
    }

    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int fd, const unsigned char *buf, int bufSize)
{
    printf("Entered llwrite\n");
    
    unsigned char *frame_info = buildFrameInfo(buf, bufSize);

    (void)signal(SIGALRM, alarmHandler);

    int current_transmition = 0;
    LinkLayerState state = START;

    while(current_transmition < retransmissions){

        if(alarm_reached_end == TRUE){ 
            int bytes_written = write(fd, frame_info, bufSize + 6);
            printf("Sent frame with information. Bytes written: %d\n", bytes_written);
            alarm(timeout);
            alarm_reached_end = FALSE;
        }

        unsigned char received_byte;
        state = START;

        while(state != STOP_READING){
            int bytes_read = read(fd, received_byte, 1);
            printf("Bytes read %d\n", bytes_read);

            if(bytes_read > 0){
                if(info_frame_number == I0){
                    state_machine_read_control_frames(received_byte, A_RECEIVER, RR1, &state);
                }
                else if(info_frame_number == I1){ 
                    //state = STOP_READING;   //this line is for testing with hello.txt                 
                    state_machine_read_control_frames(received_byte, A_RECEIVER, RR0, &state);
                }
            }
            else break;
        }
    }

    printf("Exiting llwrite.\n");

    if(state != STOP_READING){
        printf("Couldn't read control frame!\n");
        return -1;
    }
    else{
        printf("Exiting llwrite.\n");
        return bufSize + 6;
    }
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}

int connect_to_serialPort(const char *serialPort){

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPort);
        exit(-1);
    }

    struct termios oldtio, newtio;

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
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // Flushes data received but not read
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    return fd;
}

// Alarm function handler
void alarmHandler(int signal)
{

    printf("Alarm #%d\n", alarmCount);    

    alarm_reached_end = TRUE;
    alarmCount++;
}

unsigned char buildSupervisionFrame(unsigned char A, unsigned char C){
    
    unsigned char supervision_frame[5] = {FLAG, A, C, A ^ C, FLAG};

    return supervision_frame;
}

void state_machine_read_supervision_frames(unsigned char curr_byte, unsigned char A, unsigned char C, LinkLayerState *state)
{
    printf("Current byte: 0x%02X state: %d\n", curr_byte, *state);
    switch (*state)
    {
    case START:
        if (curr_byte == FLAG)
            *state = FLAG_RECEIVED;
        else
            *state = START;
        break;
    case FLAG_RECEIVED:
        if (curr_byte == FLAG)
            *state = FLAG_RECEIVED;
        else if (curr_byte == A)
            *state = A_RECEIVED;
        else
            *state = START;
        break;
    case A_RECEIVED:
        if (curr_byte == FLAG)
            *state = FLAG_RECEIVED;
        else if (curr_byte == C)
            *state = C_RECEIVED;
        else
            *state = START;
        break;
    case C_RECEIVED:
        if (curr_byte == FLAG)
            *state = FLAG_RECEIVED;
        else if (curr_byte == A ^ C)
            *state = BCC1_CHECKED;
        else
            *state = START;
        break;
    case BCC1_CHECKED:
        if (curr_byte == FLAG)
        {
            *state = STOP_READING;
            printf("Read successfully!\n");
        }
        else
            *state = START;
        break;
    default:
        break;
    }
}

unsigned char *buildFrameInfo(unsigned char *buffer, int length){
    unsigned char *frame_info = malloc((length + 6) * sizeof(unsigned char));
    frame_info[0] = FLAG;
    frame_info[1] = A_SENDER;
    frame_info[2] = info_frame_number;
    frame_info[3] = frame_info[1] ^ frame_info[2];
    memcpy(frame_info+4, buffer, length);

    unsigned char BCC2 = buildBCC2(buffer, length);

    frame_info[length + 4] = BCC2;
    frame_info[length + 5] = FLAG;

    return frame_info;
}

unsigned char buildBCC2(unsigned char *buffer, int length){
    unsigned char BCC2 = buffer[0];

    for (int i = 1; i < length + 4; i++)
    {
        BCC2 ^= buffer[i];
    }

    return BCC2;   
}

void state_machine_read_control_frames(unsigned char curr_byte, unsigned char A, unsigned char C, LinkLayerState *state)
{
    printf("Current byte: 0x%02X state: %d\n", curr_byte, state);
    
    switch (*state)
    {
    case START:
        if (curr_byte == FLAG)
            *state = FLAG_RECEIVED;
        else
            *state = START;
        break;
    case FLAG_RECEIVED:
        if (curr_byte == FLAG)
            *state = FLAG_RECEIVED;
        else if (curr_byte == A)
            *state = A_RECEIVED;
        else
            *state = START;
        break;
    case A_RECEIVED:
        if (curr_byte == FLAG)
            *state = FLAG_RECEIVED;
        else if (curr_byte == C)
            *state = C_RECEIVED;
        else
            *state = START;
        break;
    case C_RECEIVED:
        if (curr_byte == FLAG)
            *state = FLAG_RECEIVED;
        else if (curr_byte == A ^ C)
            *state = BCC1_CHECKED;
        else
            *state = START;
        break;
    case BCC1_CHECKED:
        if (curr_byte == FLAG)
        {
            printf("Read successfully!\n");
            *state = STOP_READING;

            if(C == RR0){
                info_frame_number = I0;
            }
            else if(C == RR1){
                info_frame_number = I1;
            }
        }
        else
            *state = START;
        break;
    default:
        break;
    }
}

