#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "utils.h"
#include <climits>
#include <vector>

#define TIMEOUT_MS 750

int main() {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in server_addr, client_addr_from, client_addr_to;
    struct packet buffer;
    socklen_t addr_size = sizeof(client_addr_from);
    unsigned int expected_seq_num = 0; // first unsent byte
    int recv_len;
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

    // // TODO: Receive file from the client and save it as output.txt

    
    // char data_window[4][PAYLOAD_SIZE + 1];
    // char server_isLast = 0;
    // char client_isLast = 0;
    // for (int i = 0; i < 4; i++)
    //     data_window[i][0] = '\0';
    // while (true) {
    //     recv_len = recvfrom(listen_sockfd, &buffer, sizeof(buffer), 0, 
    //         (struct sockaddr *) &client_addr_from, &addr_size);
    //     printRecv(&buffer);
    //     printf("expected seq num: %d\n", expected_seq_num);
    //     //printf("payload: %s\n",buffer.payload);
    //     if (buffer.last)
    //         break;
    //     // if (buffer.length < PAYLOAD_SIZE)
    //     //     isLast = 1;
    //     if (buffer.seqnum >= expected_seq_num) {
    //         char payload[PAYLOAD_SIZE + 1];
    //         memcpy(payload, buffer.payload, buffer.length);
    //         payload[PAYLOAD_SIZE] = '\0';
    //         unsigned int window_pos = ceil((double)(buffer.seqnum - expected_seq_num) / PAYLOAD_SIZE);
    //         memcpy(data_window[window_pos], payload, sizeof(payload));
    //         if (buffer.seqnum == expected_seq_num){
    //             fprintf(fp, "%s", data_window[0]);
    //             //printf("%s\n",data_window[0]);
    //             expected_seq_num+=strlen(data_window[0]);
    //             int i;
    //             for (i = 1; i < 4; i++) {
    //                 //for the packets in the window
    //                 if (data_window[i][0] != '\0') {
    //                     fprintf(fp, "%s", data_window[i]);
    //                     expected_seq_num+=strlen(data_window[i]);

    //                 }
    //                 else {
    //                     int j;
    //                     for (j = 0; j < 4 - i; j++)
    //                         memcpy(data_window[j], data_window[j + i], sizeof(data_window[j + i]));
    //                     for (; j < 4; j++)
    //                         data_window[j][0] = '\0';
    //                     break;
    //                 }
                    
    //             }                 
    //             //expected_seq_num = (expected_seq_num + i * buffer.length);
    //         } 
    //         build_packet(&ack_pkt, 0, expected_seq_num, server_isLast, 1, 0, NULL);
    //     }
    //     else {
    //         build_packet(&ack_pkt, 0, expected_seq_num, buffer.last, 1, 0, NULL);
    //     }
    //     sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, 
    //         (struct sockaddr *) &client_addr_to, sizeof(client_addr_to));
    //     printSend(&ack_pkt, 0);
    //     //printf("%s", buffer.payload);
    //     //printRecv(&buffer);
    //     if (buffer.last)
    //         break;
    // }

    //printf("%s", )

    // TODO: Receive file from the client and save it as output.txt

    
    struct packet data_window[WINDOW_SIZE];
    bool send_FIN = false;

    char server_isLast = 0;
    char client_isLast = 0;
    for (int i = 0; i < WINDOW_SIZE; i++)
        data_window[i].length = 0;

    clock_t start = clock(), elapsed;
    unsigned int msec_timer = 0;

    struct packet FIN;
    build_packet(&FIN, 0, 0, 1, 0, 0, NULL);

    while (true) {
        elapsed = clock() - start;
        msec_timer = 1000 * elapsed / CLOCKS_PER_SEC;
        
        // send FIN packet
        if (send_FIN && msec_timer > TIMEOUT_MS) {
            sendto(send_sockfd, &FIN, sizeof(FIN), 0, 
                (struct sockaddr *) &client_addr_to, sizeof(client_addr_to));
            printSend(&FIN, 1);
            start = clock();
            continue;
        } 

        recv_len = recvfrom(listen_sockfd, &buffer, sizeof(buffer), MSG_DONTWAIT, 
            (struct sockaddr *) &client_addr_from, &addr_size);
        if (recv_len <= 0) 
            continue;    
        printRecv(&buffer);
        //printf("expected seq num: %d\n", expected_seq_num);
        //printf("payload: %s\n",buffer.payload);

        // if packet is FINACK
        if (send_FIN && buffer.ack)
            break;


        // if packet is FIN packet from client send FINACK
        if (buffer.last) {
            struct packet FINACK;
            build_packet(&FINACK, 0, buffer.seqnum + 1, 1, 1, 0, NULL);
            sendto(send_sockfd, &FINACK, sizeof(FINACK), 0, 
                (struct sockaddr *) &client_addr_to, sizeof(client_addr_to));
            printSend(&FINACK, 0);
            send_FIN = 1;
            continue;
        }


        // if (buffer.length < PAYLOAD_SIZE)
        //     isLast = 1;
        if (buffer.seqnum >= expected_seq_num) {
            //char payload[PAYLOAD_SIZE + 1];
            //memcpy(payload, buffer.payload, buffer.length);
            //payload[PAYLOAD_SIZE] = '\0';
            // unsigned int window_pos = ceil((double)(buffer.seqnum - expected_seq_num) / PAYLOAD_SIZE);
            unsigned short window_pos = buffer.seqnum - expected_seq_num;
            //memcpy(&data_window[window_pos], &buffer, sizeof(buffer));
            data_window[window_pos] = buffer;
            // if(data_window[window_pos].length<PAYLOAD_SIZE){
            //    // printf("payload altered\n");
            //     data_window[window_pos].payload[data_window[window_pos].length]='\0';
            // }

            if (buffer.seqnum == expected_seq_num){
                 fprintf(fp, "%.*s",data_window[0].length,data_window[0].payload);
                //printf("payload: %.*s\n",data_window[0].length,data_window[0].payload);
                expected_seq_num++;
                data_window[0].length=0;

                if(data_window[0].last){
                    server_isLast=1;
                }
                int i;
                for (i = 1; i < WINDOW_SIZE; i++) {
                    //for the packets in the window
                    if (data_window[i].length != 0) {
                        fprintf(fp, "%.*s", data_window[i].length,data_window[i].payload);
                        //expected_seq_num+=data_window[i].length;
                        expected_seq_num++;

                        //reset datawindow:
                        data_window[i].length=0;
                        //
                        if(data_window[i].last){
                            server_isLast=1;
                        }
                    }
                    else {
                        int j;
                        for (j = 0; j < WINDOW_SIZE - i; j++){
                            //memcpy(&data_window[j], &data_window[j + i], sizeof(data_window[j + i]));
                            data_window[j]=data_window[j+i];
                        }
                        for (; j < WINDOW_SIZE; j++){
                            data_window[j].length = 0;
                            //data_window[j].payload[0]='\0';
                            //memset(data_window[j].payload, 0, sizeof(data_window[j].payload));
                        }
                        break;
                    }
                    
                }                 
                //expected_seq_num = (expected_seq_num + i * buffer.length);
            } 
            build_packet(&ack_pkt, 0, expected_seq_num, 0, 1, 0, NULL);
        }
        else {
            build_packet(&ack_pkt, 0, expected_seq_num, 0, 1, 0, NULL);
        }
        sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, 
            (struct sockaddr *) &client_addr_to, sizeof(client_addr_to));
        printSend(&ack_pkt, 0);
        //printf("%s", buffer.payload);
        //printRecv(&buffer);
        // if (server_isLast){
        //     for(int i =0;i<3;i++){
        //         sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0,  (struct sockaddr *) &client_addr_to, sizeof(client_addr_to));
        //          printSend(&ack_pkt, 0);
        //     }
        //     break;
        // }
    }

    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}
