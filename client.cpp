#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <string>
#include "utils.h"


int main(int argc, char *argv[]) {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in client_addr, server_addr_to, server_addr_from;
    socklen_t addr_size = sizeof(server_addr_to);
    struct timeval tv;
    struct packet pkt;
    struct packet ack_pkt;
    char buffer[PAYLOAD_SIZE];
    unsigned short seq_num = 0;
    unsigned short ack_num = 0;
    char last = 0;
    char ack = 0;

    // read filename from command line argument
    if (argc != 2) {
        printf("Usage: ./client <filename>\n");
        return 1;
    }
    char *filename = argv[1];

    // Create a UDP socket for listening
    listen_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_sockfd < 0) {
        perror("Could not create listen socket");
        return 1;
    }

    // Create a UDP socket for sending
    send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_sockfd < 0) {
        perror("Could not create send socket");
        return 1;
    }

    // Configure the server address structure to which we will send data
    memset(&server_addr_to, 0, sizeof(server_addr_to));
    server_addr_to.sin_family = AF_INET;
    server_addr_to.sin_port = htons(SERVER_PORT_TO);
    server_addr_to.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Configure the client address structure
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT);
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the listen socket to the client address
    if (bind(listen_sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(listen_sockfd);
        return 1;
    }

    // Open file for reading
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        close(listen_sockfd);
        close(send_sockfd);
        return 1;
    }

    // TODO: Read from file, and initiate reliable data transfer to the server

    int HEADER_SIZE = 2; // only seq_num for now
    char packet[HEADER_SIZE + PAYLOAD_SIZE];
    // char packet[PAYLOAD_SIZE];
    unsigned int file_length;
    // char* file_content;
    fseek(fp, 0, SEEK_END);
    file_length = ftell(fp);
    // file_content = (char*) malloc(file_length);
    fseek(fp, 0, SEEK_SET);
    seq_num = 0;
    
    while (true){
        //printf("while loop running\n");
        unsigned int bytes_read = fread(buffer, 1, PAYLOAD_SIZE, fp);
        if (bytes_read < PAYLOAD_SIZE) {
            buffer[bytes_read] = '\0';
            bytes_read++;
        }
        // memcpy(packet, &seq_num, sizeof(seq_num));
        // memcpy(&packet[2], buffer, bytes_read + 1);
        // //packet[HEADER_SIZE + PAYLOAD_SIZE] = '\0';
        // printf("%x", packet[0]);
        // printf("%s", packet + 2);
        build_packet(&pkt, seq_num, 0, 0, 0, bytes_read, buffer);
        printRecv(&pkt);
        printf("%s", pkt.payload);
        sendto(send_sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *) &server_addr_to, addr_size);
        if (seq_num == 0)
            seq_num = 1;
        else 
            seq_num = 0;
        if (bytes_read < PAYLOAD_SIZE)
            break;
    }
    
    
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

