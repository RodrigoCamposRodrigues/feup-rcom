// Application layer protocol implementation

#include "application_layer.h"

int sendControlPacket(const unsigned int C, int file_size, const char *filename)
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
        control_packet_start[2 + l1 - i] = file_size & 0xFF;
        file_size >>= 8;
    }

    //print file size bytes in control packet
    printf("File size bytes in control packet: ");
    for(int i = 0; i < l1; i++)
    {
        printf("0x%02X ", control_packet_start[3 + i]);
    }
    printf("\n");


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
    
    int received_file_size_bytes = received_packet[2];
    printf("Received file size bytes: %d\n", received_file_size_bytes);
    int received_file_size_aux[received_file_size_bytes];
    int received_file_size = 0;

    for (int i = 0; i < received_file_size_bytes; i++) {
        received_file_size_aux[i] = received_packet[3 + i];
    }

    for(int i = 0; i < received_file_size_bytes; i++)
    {
        received_file_size |= (received_file_size_aux[received_file_size_bytes - i - 1] << (8*i));
    }
    printf("Received file size: %d\n", received_file_size);

    int received_filename_bytes = received_packet[received_file_size_bytes + 4];
    printf("Received filename bytes: %d\n", received_filename_bytes);

    for(int i = 0; i < received_filename_bytes; i++)
    {
        received_filename[i] = received_packet[received_file_size_bytes + 5 + i];
    }

    received_filename[received_filename_bytes] = '\0';

    printf("Received filename: %s\n", received_filename);

    return received_file_size;
}

int packetRecognition(unsigned char *received_packet, int received_packet_size)
{
    unsigned char *file_name[40];
    int received_file_size;
    FILE* new_file;
    
    printf("received_packet[0]: %d\n", received_packet[0]);
    switch(received_packet[0])
    {
        case 2:
        {
            printf("Start in packetRecognition\n");
            //start packet received
            received_file_size = parseControlPacket(received_packet, received_packet_size, file_name);

            new_file = fopen(file_name, "wb+");
            if(new_file == NULL)
            {
                printf("Error opening file\n");
                exit(-1);
            }

            return TRUE;
        }

        case 1:
        {
            printf("data in packetRecognition\n");

            fwrite(received_packet + 3, 1, received_packet_size - 3, new_file);

            return TRUE;
        }

        case 3:
        {
            printf("end in packetRecognition\n");

            fclose(new_file);
            //received end packet
            // close(local_fd);
            return FALSE;
        }
        default:
            printf("Invalid control packet\n");
            exit(-1);
            break;
    }
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
            int file_size = ftell(file) - prev;
            printf("File size: %ld\n", file_size);
            fseek(file, 0, SEEK_SET);

            if(sendControlPacket(2, file_size, filename) < 0){
                printf("Error sending start control packet\n");
                exit(-1);
            }

            printf("Start control packet sent\n");

            //read data from file and store it in a content variable
            int content_size = MAX_PAYLOAD_SIZE - 3;
            unsigned char content[content_size]; // Static array for file content
            // int content_index = 0;

            int bytes_left_to_send = file_size;
            unsigned char data_to_send[MAX_PAYLOAD_SIZE];

            while (bytes_left_to_send > 0) {
                // Read a chunk of data from the file
                int data_size_to_send = (bytes_left_to_send > MAX_PAYLOAD_SIZE) ? content_size : bytes_left_to_send;
                fread(content, 1, data_size_to_send, file);

                // Copy data to data_to_send array
                memcpy(data_to_send, content, content_size);

                int data_packet_size;
                unsigned char data_packet[data_size_to_send + 3];
                data_packet_size = buildDataPacket(data_to_send, data_size_to_send, data_packet);

                if (llwrite(data_packet, data_packet_size) < 0) {
                    printf("Error sending data packet\n");
                    exit(-1);
                }

                bytes_left_to_send -= data_size_to_send;
            }

            if (sendControlPacket(3, file_size, filename) < 0) {
                printf("Error sending end control packet\n");
                exit(-1);
            }

            printf("End control packet sent\n");

            llclose(fd); 
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


