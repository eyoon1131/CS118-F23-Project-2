#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "utils.h"
#include <climits>

int main() {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in server_addr, client_addr_from, client_addr_to;
    struct packet buffer;
    socklen_t addr_size = sizeof(client_addr_from);
    unsigned int expected_seq_num = 0;
    unsigned int recv_len;
    struct packet ack_pkt;

    // Create a UDP socket for sending
    send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_sockfd < 0) {
        perror("Could not create send socket");
        return 1;
    }

    // Create a UDP socket for listening
    listen_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_sockfd < 0) {
        perror("Could not create listen socket");
        return 1;
    }

    // Configure the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the listen socket to the server address
    if (bind(listen_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(listen_sockfd);
        return 1;
    }

    // Configure the client address structure to which we will send ACKs
    memset(&client_addr_to, 0, sizeof(client_addr_to));
    client_addr_to.sin_family = AF_INET;
    client_addr_to.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    client_addr_to.sin_port = htons(CLIENT_PORT_TO);

    // Open the target file for writing (always write to output.txt)
    FILE *fp = fopen("output.txt", "wb");

    // TODO: Receive file from the client and save it as output.txt

    unsigned int next_expected_seq = PAYLOAD_SIZE;
    
    char data_window[4][PAYLOAD_SIZE + 1];
    char isLast = 0;
    for (int i = 0; i < 4; i++)
        data_window[i][0] = '\0';
    while (true) {
        recv_len = recvfrom(listen_sockfd, &buffer, sizeof(buffer), 0, 
            (struct sockaddr *) &client_addr_from, &addr_size);
        printRecv(&buffer);
        printf("expected seq num: %d", expected_seq_num);
        if (buffer.last)
            break;
        if (buffer.length < PAYLOAD_SIZE)
            isLast = 1;
        if (buffer.seqnum >= expected_seq_num) {
            char payload[PAYLOAD_SIZE + 1];
            memcpy(payload, buffer.payload, buffer.length);
            payload[PAYLOAD_SIZE] = '\0';
            unsigned int window_pos = ceil((double)(buffer.seqnum - expected_seq_num) / PAYLOAD_SIZE);
            memcpy(data_window[window_pos], payload, sizeof(payload));
            if (buffer.seqnum == expected_seq_num){
                fprintf(fp, "%s", data_window[0]);
                int i;
                for (i = 1; i < 4; i++) {
                    //for the packets in the window
                    if (data_window[i][0] != '\0') {
                        fprintf(fp, "%s", data_window[i]);
                    }
                    else {
                        int j;
                        for (j = 0; j < 4 - i; j++)
                            memcpy(data_window[j], data_window[j + i], sizeof(data_window[j + i]));
                        for (; j < 4; j++)
                            data_window[j][0] = '\0';
                        break;
                    }
                    
                }                 
                expected_seq_num = (buffer.seqnum + i * buffer.length);
            } 
            build_packet(&ack_pkt, 0, expected_seq_num, isLast, 1, 0, NULL);
        }
        else {
            build_packet(&ack_pkt, 0, expected_seq_num, buffer.last, 1, 0, NULL);
        }
        sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, 
            (struct sockaddr *) &client_addr_to, sizeof(client_addr_to));
        printSend(&ack_pkt, 0);
        //printf("%s", buffer.payload);
        //printRecv(&buffer);
        if (buffer.last)
            break;
    }

    //printf("%s", )

    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}
