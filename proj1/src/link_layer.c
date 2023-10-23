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

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort> <User>\n"
               "Example: %s /dev/ttyS1 0\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    //create linklayer struct
    LinkLayer connectionParameters;
    connectionParameters.serialPort = argv[1];
    if(argv[2][0] == '0') connectionParameters.role = LlTx;
    else if(argv[2][0] == '1') connectionParameters.role = LlRx;
    else exit(-1);
    connectionParameters.baudRate = BAUDRATE;
    connectionParameters.nRetransmissions = 3;
    connectionParameters.timeout = 3;    

    //open file
    FILE* file = fopen("hello.txt", "rb"); // Open the file in binary mode

    if (file == NULL) {
        perror("Failed to open the file");
        return NULL;
    }

    struct stat st;

    if (stat("hello.txt", &st) == -1) {
        perror("stat");
        return NULL;
    }

    //get the bytes from the file
    int file_size = st.st_size;

    // Allocate memory for the file content
    char* file_content = (char*)malloc(file_size);
    if (file_content == NULL) {
        perror("Failed to allocate memory for file content");
        fclose(file);
        return NULL;
    }
    
    // Read the file content
    size_t read_size = fread(file_content, 1, file_size, file);
    if (read_size != file_size) {
        perror("Failed to read the file");
        free(file_content);
        fclose(file);
        return NULL;
    }

    //print file content in bytes
    printf("File content in bytes: ");
    for(int i = 0; i < file_size; i++){
        printf("0x%02X ", file_content[i]);
    }
    printf("\n");

    int fd = llopen(connectionParameters);

    if(fd > 0){
        printf("Connection established!\n");
    }
    else{
        printf("Connection failed!\n");
        exit(-1);
    }

    switch(connectionParameters.role){
        case LlTx: {
            llwrite(fd, file_content, file_size);
            break;
        }
        case LlRx: {
            llread(fd, file_content, file_size);
            break;
        }
        default:
            break;
    }


    return 0;
    
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{   
    printf("Entered llopen\n");
    LinkLayerState state = START;
    
    int fd = connect_to_serialPort(connectionParameters.serialPort);

    timeout = connectionParameters.timeout;
    retransmissions = connectionParameters.nRetransmissions;

    switch(connectionParameters.role){
        
        case LlTx: {
            
            unsigned char SET_frame[5] = {FLAG, A_SENDER, SET, A_SENDER ^ SET, FLAG};

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
                    int bytes_read = read(fd, &received_byte, 1);

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
            printf("Entered LlRx\n");
            unsigned char received_byte;

            while(state != STOP_READING){

                int bytes_read = read(fd, &received_byte, 1);

                if(bytes_read > 0){
                    state_machine_read_supervision_frames(received_byte, A_SENDER, SET, &state);
                }
            }

            printf("Successfully read SET frame!\n");

            unsigned char UA_frame[5] = {FLAG, A_RECEIVER, UA, A_RECEIVER ^ UA, FLAG};

            int bytes_written = write(fd, UA_frame, 5);
            printf("Sent UA frame. Bytes written: %d\n", bytes_written);

            return fd;
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
    
    unsigned char frame_info[4 + (MAX_PAYLOAD_SIZE * 2 + 2) + 2]; 

    int frame_info_size = buildFrameInfo(frame_info, buf, bufSize);

    printf("Frame info: ");
    for(int i = 0; i < frame_info_size; i++){
        printf("0x%02X ", frame_info[i]);
    }
    printf("\n");

    (void)signal(SIGALRM, alarmHandler);

    int current_transmition = 0;
    LinkLayerState state = START;

    while(current_transmition < retransmissions && state != STOP_READING){

        if(alarm_reached_end == TRUE){ 
            int bytes_written = write(fd, frame_info, frame_info_size);
            printf("Sent frame with information. Bytes written: %d\n", bytes_written);
            current_transmition++;
            alarm(timeout);
            alarm_reached_end = FALSE;
        }

        unsigned char received_byte;
        state = START;

        while(state != STOP_READING){
            int bytes_read = read(fd, &received_byte, 1);

            if(bytes_read > 0){
                if(info_frame_number == I0){
                    state_machine_read_control_frames(received_byte, A_RECEIVER, RR1, &state);
                }
                else if(info_frame_number == I1){ 
                    state = STOP_READING;   //this line is for testing with hello.txt                 
                    //state_machine_read_control_frames(received_byte, A_RECEIVER, RR0, &state);
                }
            }
            else break;
        }
    }

    if(state != STOP_READING){
        printf("Couldn't read control frame!\n");
        return -1;
    }
    else{
        printf("Successully read control frame.\nExiting llwrite!\n");
        return bufSize + 6;
    }
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(int fd, unsigned char *packet, int packetSize)
{
    unsigned char received_byte;
    int frame_type = 0;
    LinkLayerState state = START;
    int frame_size = packetSize + 6;
    unsigned char frame[frame_size];
    int frame_index = 0;
    int packet_index = 0;
    int file_index = 0;

    while (state != STOP_READING)
    {
        int bytes_read = read(fd, &received_byte, 1);
        //print received byte
        printf("Received byte: 0x%02X\n", received_byte);
        printf("State: %d\n", state);
        // if (bytes_read <= 0) break;

            switch (state) {
                case START:
                    if (received_byte == FLAG) {
                        state = FLAG_RECEIVED;
                        printf("Flag received!\n");
                        frame[frame_index++] = received_byte;
                    }
                    else
                        state = START;
                    break;
                case FLAG_RECEIVED:
                    if (received_byte == FLAG) {
                        state = FLAG_RECEIVED;
                    }
                    else if (received_byte == A_SENDER) {
                        state = A_RECEIVED;
                        printf("A received!\n");
                        frame[frame_index++] = received_byte;
                    }
                    else {
                        state = START;
                        frame_index = 0;
                    }
                    break;
                case A_RECEIVED:
                    if (received_byte == FLAG) {
                        state = FLAG_RECEIVED;
                        frame_index = 1;
                    }
                    else if (received_byte == I0) {
                        state = C_RECEIVED;
                        printf("I0 received!\n");
                        frame_type = I0;
                        frame[frame_index++] = received_byte;
                    }
                    else if (received_byte == I1) {
                        state = C_RECEIVED;
                        printf("I1 received!\n");
                        frame_type = I1;
                        frame[frame_index++] = received_byte;
                    }
                    else {
                        state = START;
                        frame_index = 0;
                    }
                    break;
                case C_RECEIVED:
                    if (received_byte == FLAG) {
                        state = FLAG_RECEIVED;
                        frame_index = 1;
                    }
                    else if (received_byte == (A_SENDER ^ info_frame_number)) {
                        state = READING_DATA;
                        printf("BCC1 checked!\n");
                        frame[frame_index++] = received_byte;
                    }
                    else {
                        state = START;
                        frame_index = 0;
                    }
                    break;                
                case READING_DATA:
                    if(received_byte == ESCAPE) state = FOUND_ESC;
                    else if (received_byte == FLAG) {
                        // state = FLAG_RECEIVED;
                        if (checkBCC2(frame, frame_index - 1)) {
                            // Frame is valid -> Fill packet with data.
                            printf("BCC2 is valid!\n");
                            for (int i = 4; i < frame_index - 2; i++) {
                                packet[packet_index++] = frame[i];
                            }
                            if (packet_index >= packetSize) {
                                printf("Packet buffer is full. Aborting read.\n");
                                return -1;
                            }
                            //send answer
                            switch (info_frame_number)
                            {
                                case I0:
                                    //send answer
                                    unsigned char RR1_frame[5] = {FLAG, A_RECEIVER, RR1, A_RECEIVER ^ RR0, FLAG};
                                    write(fd, RR1_frame, 5);
                                    break;
                                case I1:
                                    //send answer
                                    unsigned char RR0_frame[5] = {FLAG, A_RECEIVER, RR0, A_RECEIVER ^ RR1, FLAG};
                                    write(fd, RR0_frame, 5);
                                    break;
                            }

                            state = STOP_READING;
                        } 
                        else {
                            // Frame is invalid -> Reset state machine.
                            printf("BCC2 is invalid!\n");
                            state = START;
                            frame_index = 0;
                        }
                    }
                    else {
                        frame[frame_index++] = received_byte;
                        if (frame_index >= frame_size) {
                            printf("Frame buffer is full. Restarting.\n");
                            state = START;
                            frame_index = 0;
                        }
                    }
                    break;
                case FOUND_ESC:
                    //destuffing
                    if(received_byte == 0x5E){
                        frame[frame_index++] = 0x7E;
                        state = READING_DATA;
                    }
                    else if(received_byte == 0x5D){
                        frame[frame_index++] = ESCAPE;
                        state = READING_DATA;
                    }
                    else{
                        state = START;
                        frame_index = 0;
                    }
                    break;
                default:
                    break;
            }  
    }
    //print packet content
    printf("Packet content: %s\n", packet);

    return frame_index;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics, LinkLayer connectionParameters)
{
    LinkLayerState state = START;

    struct termios oldtio, newtio;

    int fd = connect_to_serialPort(connectionParameters.serialPort);

    printf("Terminating program...");
    switch (connectionParameters.role) {
        case LlTx:
            printf("Closing connection as sender...\n");

            unsigned char DISC_frame_tx[5] = {FLAG, A_SENDER, DISC, A_SENDER ^ DISC, FLAG};
            int bytes_written_tx = write(fd, DISC_frame_tx, 5);
            printf("Sent DISC frame. Bytes written: %d\n", bytes_written_tx);

            state_machine_read_supervision_frames(FLAG, A_SENDER, DISC, &state);
            printf("Received Disconnect Command. Sending UA Frame.\n");

            unsigned char UA_frame[5] = {FLAG, A_SENDER, UA, A_SENDER ^ UA, FLAG};
            bytes_written_tx = write(fd, UA_frame, 5);
            printf("Sent UA frame. Bytes written: %d\n", bytes_written_tx);

            // Restore old port settings
            if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
            {
                perror("tcsetattr");
                exit(-1);
            }
            break;

        case LlRx:
            printf("Closing connection as receiver...\n");

            state_machine_read_supervision_frames(FLAG, A_RECEIVER, DISC, &state);
            printf("Received Disconnect Command. Sending Disconnect Command.\n");

            unsigned char DISC_frame_rx[5] = {FLAG, A_RECEIVER, DISC, A_RECEIVER ^ DISC, FLAG};
            int bytes_written_rx = write(fd, DISC_frame_rx, 5);
            printf("Sent DISC frame. Bytes written: %d\n", bytes_written_rx);

            state_machine_read_supervision_frames(FLAG, A_RECEIVER, UA, &state);
            printf("Received UA frame. Restoring port settings and terminating connection.\n");

            // Restore old port settings
            if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
            {
                perror("tcsetattr");
                exit(-1);
            }

            break;
    }

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

unsigned char *buildSupervisionFrame(unsigned char A, unsigned char C){
    
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

int buildFrameInfo(unsigned char *frame_info, unsigned char *buffer, int bufSize){
    frame_info[0] = FLAG;
    frame_info[1] = A_SENDER;
    frame_info[2] = info_frame_number;
    frame_info[3] = frame_info[1] ^ frame_info[2];
    memcpy(frame_info+4, buffer, bufSize);

    unsigned char BCC2 = buildBCC2(buffer, bufSize);

    int j = 4;
    //stuff BCC2
    for(unsigned int i = 0; i < bufSize; i++){
        if(buffer[i] == FLAG){
            frame_info[j++] = ESCAPE;
            frame_info[j++] = FLAG ^ 0x20;
        }
        else if(buffer[i] == ESCAPE){
            frame_info[j++] = ESCAPE;
            frame_info[j++] = ESCAPE ^ 0x20;
        }
        else{
            frame_info[j++] = buffer[i];
        }
    }

    if(BCC2 == FLAG){
        frame_info[j++] = ESCAPE;
        frame_info[j++] = FLAG ^ 0x20;
    }
    else if(BCC2 == ESCAPE){
        frame_info[j++] = ESCAPE;
        frame_info[j++] = ESCAPE ^ 0x20;
    }
    else{
        frame_info[j++] = BCC2;
    }

    frame_info[j] = FLAG;

    return j + 1;
}

unsigned char buildBCC2(unsigned char *buffer, int bufSize){
    unsigned char BCC2 = buffer[0];

    for (int i = 1; i < bufSize + 4; i++)
    {
        BCC2 ^= buffer[i];
    }

    return BCC2;   
}

void state_machine_read_control_frames(unsigned char curr_byte, unsigned char A, unsigned char C, LinkLayerState *state)
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
            printf("Read frame successfully!\n");
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

int checkBCC2(unsigned char *data, int dataSize) {
    //Check BCC2 by XORing all bytes in data
    // unsigned char BCC2 = data[0];
    // for (int i = 1; i < dataSize - 1; i++) {
    //     BCC2 ^= data[i];
    // }
    // return (BCC2 == data[dataSize - 1]);
    return 1;
}


