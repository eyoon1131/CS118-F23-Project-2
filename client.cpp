#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <string>
#include "utils.h"
#include <math.h>


int main(int argc, char *argv[]) {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in client_addr, server_addr_to, server_addr_from;
    socklen_t addr_size = sizeof(server_addr_to);
    struct timeval tv;
    struct packet pkt;
    struct packet ack_pkt;
    char buffer[PAYLOAD_SIZE];
    unsigned int seq_num = 0;
    unsigned int ack_num = 0;
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

    // make listen_sockfd non-blocking
    // tv.tv_sec = 0;
    // tv.tv_usec = 10;
    // setsockopt(listen_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

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
    // file_length = ftell(fp);
    unsigned int file_packet_size = ceil(ftell(fp) / (double) PAYLOAD_SIZE);
        printf("file length in packets: %d\n", file_packet_size);

    // file_content = (char*) malloc(file_length);
    fseek(fp, 0, SEEK_SET);
    seq_num = 0;
    unsigned int expected_seq_num = 0;
    char server_isLast = 0;
    char client_isLast = 0;
    bool timeout = false;
    struct packet window[4];
    int last_sent_pkt_pos = -1;
    

    bool reset = false;
    clock_t start = clock(), elapsed;
    unsigned int msec_timer = 0;
    unsigned int send_base = 0;
    bool start_close = 0;
    unsigned short dup_count = 0;
    unsigned int last_ACK = 100000;
    //unsigned int rwnd = 4;
    while (true) {
        if (reset) {
            start = clock();
            reset = false;
            msec_timer = 0;
        }
        elapsed = clock() - start;
        msec_timer = 1000 * elapsed / CLOCKS_PER_SEC;
        // if (ACK_received) {
         //   for (int i = last_sent_pkt_pos + 1; i < 4; i++) {
        // last_sent_pkt_pos starts as -1, since no pkts sent at start

        if (last_sent_pkt_pos + 1 < 4) {
            if (last_sent_pkt_pos == -1)
                reset = true;
            
            // send FIN packet
            if (start_close) {
                build_packet(&window[0], file_packet_size, 0, 1, 0, 0, NULL);
                start_close = 0;
                sendto(send_sockfd, &window[0], sizeof(window[0]), 
                    0, (struct sockaddr *) &server_addr_to, addr_size);
                // dont need to create any more packets
                printSend(&window[0], 0); 
                last_sent_pkt_pos = 4;
            }
            
            if (seq_num != file_packet_size) {
                unsigned int bytes_read = fread(buffer, 1, PAYLOAD_SIZE, fp);
                // if (seq_num+bytes_read == file_length) {
                //     //buffer[bytes_read] = '\0';
                //     //bytes_read++;
                //     client_isLast=1;
                // }
                build_packet(&window[last_sent_pkt_pos + 1], seq_num, 0, client_isLast, 0, bytes_read, buffer);
                sendto(send_sockfd, &window[last_sent_pkt_pos + 1], sizeof(window[last_sent_pkt_pos + 1]), 
                    0, (struct sockaddr *) &server_addr_to, addr_size);
                printSend(&window[last_sent_pkt_pos + 1], 0);
                //printf("%d\n", last_sent_pkt_pos + 1);


                if (server_isLast){
                    break; 
                }


                last_sent_pkt_pos++;
                //seq_num = seq_num + bytes_read;
                seq_num++;
                //printf("seq num after packet sent: %d\n",seq_num);
                continue;
            }
        }
        if (msec_timer > 750) {
            sendto(send_sockfd, &window[0], sizeof(window[0]), 
                0, (struct sockaddr *) &server_addr_to, addr_size);
            //printf("timeout\n");
            printSend(&window[0], 1);
            reset = true;
            continue;
        }
        if (recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_DONTWAIT, 
            (struct sockaddr *) &server_addr_from, &addr_size) > 0) {
            printRecv(&ack_pkt);
            // if(ack_pkt.last){
            //     break;
            // }            
            ack_num = ack_pkt.acknum;
            // if packet is FINACK
            if (ack_num == file_packet_size + 1)
                break;
            // if acknum == last packet of file + 1, whole file received
            if (ack_num == file_packet_size) {
                start_close = 1;
                last_sent_pkt_pos = -1;
                continue;
            }
            if (ack_num == last_ACK)
                dup_count++;
            else   
                dup_count = 0;
            if (dup_count == 3) {
                msec_timer = 751;
                continue;
            }
            if (ack_num > send_base) {
                send_base = ack_num;
                int i;
                for (i = 0; i < 4; i++) {
                    if (window[i].seqnum == send_base) {
                        for (int j = 0; j < 4 - i; j++) {
                            window[j] = window[j + i];
                            last_sent_pkt_pos = j;
                        }
                        reset = true;
                        break;
                    }
                }
                if (i == 4) 
                    last_sent_pkt_pos = -1;
            }         
            last_ACK = ack_num;
        }

    }

    // wait for FIN packet from server
    struct packet FIN;
    recvfrom(listen_sockfd, &FIN, sizeof(FIN), 0, 
            (struct sockaddr *) &server_addr_from, &addr_size);
    printRecv(&FIN);
    struct packet FINACK;
    build_packet(&FINACK, 0, 0, 1, 1, 0, NULL);
    sendto(send_sockfd, &FINACK, sizeof(FINACK), 
        0, (struct sockaddr *) &server_addr_to, addr_size);
    printSend(&FINACK, 0);
    // once FIN received, enter wait state
    start = clock();
    unsigned int sec_timer = 0;
    while (true) {
        elapsed = clock() - start;
        sec_timer = elapsed / CLOCKS_PER_SEC;
        if (sec_timer > 10) { 
            break; 
            // wait over, shut down
        }
        // if another FIN received, then FINACK not received by server
        if (recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_DONTWAIT, 
            (struct sockaddr *) &server_addr_from, &addr_size) > 0) {
            printRecv(&ack_pkt);
            sec_timer = 0;
            sendto(send_sockfd, &FINACK, sizeof(FINACK), 
                0, (struct sockaddr *) &server_addr_to, addr_size);
            printSend(&FINACK, 1);
        }
    }
    
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

