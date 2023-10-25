// Application layer protocol implementation

#include "application_layer.h"

int sendControlPacket(const unsigned int C, long int file_size, const char *filename)
{
    const int l1 = sizeof(file_size);
    const int l2 = strlen(filename);

    unsigned int packet_size = 5 + l1 + l2;
    unsigned char control_packet_start[packet_size];

    control_packet_start[0] = C;
    control_packet_start[1] = 0;
    control_packet_start[2] = l1;

    for(int i = 0; i < l1; i++)
    {
        control_packet_start[3 + i] = (file_size >> (8 * i)) & 0xFF;
    }

    control_packet_start[3 + l1] = 1;
    control_packet_start[4 + l1] = l2;
    memcpy(control_packet_start + 5 + l1, filename, sizeof(char) * l2);

    //print control packet
    printf("Control packet: ");
    for(int i = 0; i < packet_size; i++)
    {
        printf("0x%02X ", control_packet_start[i]);
    }
    printf("\n");

    return llwrite(control_packet_start, packet_size);
}

int buildDataPacket(unsigned char *data_to_send, int data_size_to_send, unsigned char *data_packet)
{
    int data_packet_size = 3 + data_size_to_send;

    data_packet[0] = 1;
    data_packet[1] = data_size_to_send / 256;
    data_packet[2] = data_size_to_send % 256;


    memcpy(data_packet + 3, data_to_send, data_size_to_send);

    return data_packet_size;
}

int parseControlPacket(unsigned char *received_packet, int received_packet_size, unsigned char *received_filename){
    int received_file_size = 0;
    for(int i = 0; i<received_packet_size; i++) {
        if(received_packet[i] == 0) {
            i++;
            for(int j = 0; j < received_packet[i]; j++) {
                received_file_size |= (received_packet[i+j+1] << (8*j));
            }
        }
    }
    received_filename = 'cok';
    return received_file_size;
}

void parseDataPacket(const unsigned char* received_packet, const unsigned int received_packet_size, unsigned char* content_to_write){
    memcpy(content_to_write,received_packet + 4,received_packet_size - 4);
    content_to_write += received_packet_size + 4;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    printf("Entered application layer\n");

    LinkLayer linkLayer;
    linkLayer.serialPort = serialPort;
    printf("  - Serial port: %s\n", linkLayer.serialPort);
    linkLayer.role = strcmp(role, "tx") == 0 ? LlTx : LlRx;
    printf("  - Role: %s\n", linkLayer.role == LlTx ? "Transmitter" : "Receiver");
    linkLayer.baudRate = baudRate;
    printf("  - Baudrate: %d\n", linkLayer.baudRate);
    linkLayer.nRetransmissions = nTries;
    printf("  - Number of tries: %d\n", linkLayer.nRetransmissions);
    linkLayer.timeout = timeout;
    printf("  - Timeout: %d\n", linkLayer.timeout);

    int fd = llopen(linkLayer);

    if(fd < 0)
    {
        printf("Error opening serial port\n");
        exit(-1);
    }

    switch(linkLayer.role)
    {
        case LlTx:{
            
            //open the file
            FILE* file = fopen(filename, "rb");

            if(file == NULL)
            {
                printf("Error opening file\n");
                exit(-1);
            }

            //determine file size
            int prev = ftell(file);
            fseek(file, 0, SEEK_END);
            long int file_size = ftell(file) - prev;
            fseek(file, 0, SEEK_SET);

            if(sendControlPacket(2, file_size, filename) < 0){
                printf("Error sending start control packet\n");
                exit(-1);
            }

            printf("Start control packet sent\n");

            //read data from file and store it in a content variable
            unsigned char *content = (unsigned char *)malloc(file_size);
            if (content == NULL) {
                printf("Error allocating memory for content\n");
                exit(-1);
            }            
            fread(content, 1, file_size, file);
            int content_index = 0;

            int bytes_left_to_send = file_size;

            unsigned char data_to_send[MAX_PAYLOAD_SIZE];


            while(bytes_left_to_send > 0)
            {
                int data_size_to_send = bytes_left_to_send > MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytes_left_to_send;

                memcpy(data_to_send, &content[content_index], data_size_to_send);

                int data_packet_size;
                unsigned char data_packet[MAX_PAYLOAD_SIZE];
                data_packet_size = buildDataPacket(data_to_send, data_size_to_send, data_packet);


                if(llwrite(data_packet, data_packet_size) < 0)
                {
                    printf("Error sending data packet\n");
                    exit(-1);
                }

                bytes_left_to_send -= data_size_to_send;
                content_index += data_size_to_send;
            }
            if(sendControlPacket(3, file_size, filename) < 0)
            {
                free(content);
                printf("Error sending end control packet\n");
                exit(-1);
            }

            printf("End control packet sent\n");
            
            free(content);
            llclose(fd); //mudar declaração do llclose
            break;}
        case LlRx:
{
            while(1){
                //call llread
                unsigned char received_packet[MAX_PAYLOAD_SIZE];
                int received_packet_size = llread(received_packet);
                //print packet
                printf("Received packet with size: %d ", received_packet_size);
                for(int i = 0; i < received_packet_size; i++)
                {
                    printf("0x%02X ", received_packet[i]);
                }
                printf("\n");

                if(received_packet_size < 0)
                {
                    printf("Error receiving packet\n");
                    exit(-1);
                }

                if(packetRecognition(received_packet, received_packet_size) == FALSE)
                    break;
            }

            llclose(fd);
            break;}
        default:
            printf("Invalid role\n");
            exit(-1);
            break;
    } 
}


int packetRecognition(unsigned char *received_packet, int received_packet_size)
{
    unsigned char *file_name[40];
    int received_file_size;
    static int local_fd;
    
    printf("received_packet[0]: %d\n", received_packet[0]);
    switch(received_packet[0])
    {
        case 2:
        {
            printf("Start in packetRecognition\n");
            //start packet received
            received_file_size = parseControlPacket(received_packet, received_packet_size, file_name);
            printf("final of thing\n");
            local_fd = open(file_name, O_WRONLY | O_CREAT, 0777);
            if(local_fd < 0){
                printf("Error opening file\n");
                exit(-1);
            }

            return TRUE;
        }

        case 1:
        {
            printf("data in packetRecognition\n");

            int bytes_to_write = received_packet[2] * 256 + received_packet[3];
            
            if(write(local_fd, received_packet + 4, bytes_to_write) < 0 ){
                printf("Error writing to file\n");
                exit(-1);
            }

            return TRUE;
        }

        case 3:
        {
            printf("end in packetRecognition\n");

            //received end packet
            close(local_fd);
            return FALSE;
        }
        default:
            printf("Invalid control packet\n");
            exit(-1);
            break;
    }
}

