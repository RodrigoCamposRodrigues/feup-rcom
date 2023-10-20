// Link layer protocol implementation

#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int alarmEnabled = FALSE;
int alarmCount = 0;
int alarm_reached_end = TRUE;
int timeout = 0;
int retransmissions = 0;
int STOP = FALSE;
int state = 0;
int READ_SUCCESS = FALSE;
unsigned char info_frame_number = I0;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    // TODO
    LinkLayerState state = START;
    
    int fd = connect_to_serialPort(connectionParameters.serialPort);

    timeout = connectionParameters.timeout;

    retransmissions = connectionParameters.nRetransmissions;

    switch(connectionParameters.role){
        
        case LlTx: {
            
            unsigned char SET_frame = buildSupervisionFrame(A_SENDER, SET);

            (void)signal(SIGALRM, alarmHandler);

            while(alarmCount < 4){

                if(alarm_reached_end == TRUE){ //quando passam os 3 segundos
                    int bytes_written = write(fd, SET_frame, 5);
                    printf("Bytes written %d\n", bytes_written);
                    alarm(3);
                    alarmEnabled = TRUE;
                    alarm_reached_end = FALSE;
                    sleep(1); //wait for bytes to be written
                }

                unsigned char receiver[MAX_PAYLOAD_SIZE] = {0}; 

                while(STOP == FALSE){
                    int bytes_read = read(fd, receiver, 1);
                    printf("Bytes read %d\n", bytes_read);

                    if(bytes_read > 0){
                        state_machine(receiver[0], A_RECEIVER, UA, A_RECEIVER ^ UA);
                    }
                    else break;
                }

            }
            
            alarm_reached_end = TRUE;

            if(READ_SUCCESS == TRUE){
                printf("Exiting llopen.\n");     
                return fd;
            }
            else
                return -1;
        }
    }

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
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

    alarmEnabled = FALSE;
    alarm_reached_end = TRUE;
    alarmCount++;
}

unsigned char *buildSupervisionFrame(unsigned char A, unsigned char C){
    
    unsigned char SET_frame[5] = {FLAG, A, C, A ^ C, FLAG};
}
