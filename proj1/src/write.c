#include "write.h"

struct termios oldtio, newtio;

int alarmEnabled = FALSE;
int alarmCount = 0;
int alarm_reached_end = TRUE;
int STOP = FALSE;
int state = 0;
int READ_SUCCESS = FALSE;
unsigned char info_frame_number = I0;

int main(int argc, char **argv)
{
    // Program usage: Uses either COM1 or COM2
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

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

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

    if(llopen(fd) > 0){
        printf("Connection established!\n");
    }
    else{
        printf("Connection failed!\n");
        exit(-1);
    }

    llwrite(fd, file_content, file_size);

    return 0;
    
}

void state_machine(unsigned char curr_byte, unsigned char A, unsigned char C, unsigned char BCC1)
{
    printf("Current byte: 0x%02X state: %d\n", curr_byte, state);
    switch (state)
    {
    case 0:
        if (curr_byte == FLAG)
            state = 1;
        else
            state = 0;
        break;
    case 1:
        if (curr_byte == FLAG)
            state = 1;
        else if (curr_byte == A)
            state = 2;
        else
            state = 0;
        break;
    case 2:
        if (curr_byte == FLAG)
            state = 1;
        else if (curr_byte == C)
            state = 3;
        else
            state = 0;
        break;
    case 3:
        if (curr_byte == FLAG)
            state = 1;
        else if (curr_byte == BCC1)
            state = 4;
        else
            state = 0;
        break;
    case 4:
        if (curr_byte == FLAG)
        {
            STOP = TRUE;
            printf("Read successfully!\n");
            READ_SUCCESS = TRUE;
            alarmCount = 4;

            if(C == RR0){
                info_frame_number = I0;
            }
            else if(C == RR1){
                info_frame_number = I1;
            }
        }
        else
            state = 0;
        break;
    default:
        break;
    }
}

// Alarm function handler
void alarmHandler(int signal)
{

    printf("Alarm #%d\n", alarmCount);    

    alarmEnabled = FALSE;
    alarm_reached_end = TRUE;
    alarmCount++;
}

int llopen(int fd)
{
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

    printf("New termios structure set\n");

    // Write SET
    unsigned char SET_msg[BUF_SIZE];

    SET_msg[0] = FLAG;
    SET_msg[1] = A_SENDER;
    SET_msg[2] = SET;
    SET_msg[3] = A_SENDER ^ SET;
    SET_msg[4] = FLAG;


    (void)signal(SIGALRM, alarmHandler);

    while(alarmCount < 4){

        if(alarm_reached_end == TRUE){ //quando passam os 3 segundos
            int bytes_written = write(fd, SET_msg, 5);
            printf("Bytes written %d\n", bytes_written);
            alarm(3);
            alarmEnabled = TRUE;
            alarm_reached_end = FALSE;
            sleep(1); //wait for bytes to be written
        }

        unsigned char receiver[BUF_SIZE] = {0}; 

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

    printf("Exiting llopen.\n");     
    
    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    if(READ_SUCCESS == TRUE)
        return fd;
    else
        return -1;
}

int llwrite(int fd, unsigned char *buffer, int length)
{
    printf("Entered llwrite\n");
    
    unsigned char *frame_info = buildFrameInfo(buffer, length);

    (void)signal(SIGALRM, alarmHandler);

    alarmCount = 0;

    while(alarmCount < 4){
        if(alarm_reached_end == TRUE){ //quando passam os 3 segundos
            int bytes_written = write(fd, frame_info, length + 6);
            printf("Bytes written %d\n", bytes_written);
            alarm(3);
            alarmEnabled = TRUE;
            alarm_reached_end = FALSE;
            sleep(1); //wait for bytes to be written
        }

        unsigned char receiver[BUF_SIZE] = {0}; 
        
        STOP = FALSE;
        state = 0;

        while(STOP == FALSE){
            int bytes_read = read(fd, receiver, 1);
            printf("Bytes read %d\n", bytes_read);

            if(bytes_read > 0){
                if(info_frame_number == I0){
                    state_machine(receiver[0], A_RECEIVER, RR1, A_RECEIVER ^ RR1);
                }
                else if(info_frame_number == I1){ 
                    //STOP = TRUE;   //this line is for testing with hello.txt                 
                    state_machine(receiver[0], A_RECEIVER, RR0, A_RECEIVER ^ RR0);
                }
            }
            else break;
        }

    }

    printf("Exiting llwrite.\n");

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    if(READ_SUCCESS == TRUE)
        return length;
    else
        return -1;
}

char* my_read_file(const char* filename, long *file_size) {
    FILE* file = fopen(filename, "rb"); // Open the file in binary mode

    if (file == NULL) {
        perror("Failed to open the file");
        return NULL;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the file content
    char* file_content = (char*)malloc(*file_size);
    if (file_content == NULL) {
        perror("Failed to allocate memory for file content");
        fclose(file);
        return NULL;
    }

    // Read the file content
    size_t read_size = fread(file_content, 1, *file_size, file);
    if (read_size != *file_size) {
        perror("Failed to read the file");
        free(file_content);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return file_content;
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
