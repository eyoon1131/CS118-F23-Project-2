#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "utils.h"
#include <time.h>

int main() {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in server_addr, client_addr_from, client_addr_to;
    struct packet buffer;
    socklen_t addr_size = sizeof(client_addr_from);
    unsigned short expected_seq_num = 0; // first unsent byte
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

    // TODO: Receive file from the client and save it as output.txt

    struct packet* data_arr;

    clock_t start = clock(), elapsed;
    unsigned int msec_timer = 0;
    
    bool send_FIN = false;
    struct packet FIN;
    build_packet(&FIN, 0, 0, 1, 0, 0, NULL, 0);

    bool first_recv = true;

    while (true) {
        elapsed = clock() - start;
        msec_timer = 1000 * elapsed / CLOCKS_PER_SEC;
        
        // send FIN packet
        if (send_FIN && msec_timer > TIMEOUT_MS) {
            sendto(send_sockfd, &FIN, sizeof(FIN), 0, 
                (struct sockaddr *) &client_addr_to, sizeof(client_addr_to));
            //printSend(&FIN, 1);
            start = clock();
            continue;
        } 

        recv_len = recvfrom(listen_sockfd, &buffer, sizeof(buffer), MSG_DONTWAIT, 
            (struct sockaddr *) &client_addr_from, &addr_size);
        if (recv_len <= 0) 
            continue;    
        //printRecv(&buffer);
        if (first_recv) {
            data_arr = new struct packet[buffer.file_packet_size + 1];
            first_recv = false;
        }

        // if packet is FINACK
        if (send_FIN && buffer.ack)
            break;

        // if packet is FIN packet from client send FINACK
        if (buffer.last) {
            struct packet FINACK;
            build_packet(&FINACK, 0, buffer.seqnum + 1, 1, 1, 0, NULL, 0);
            sendto(send_sockfd, &FINACK, sizeof(FINACK), 0, 
                (struct sockaddr *) &client_addr_to, sizeof(client_addr_to));
            //printSend(&FINACK, 0);
            send_FIN = 1;
            continue;
        }

        if (buffer.seqnum >= expected_seq_num) {
            data_arr[buffer.seqnum] = buffer;

            if (buffer.seqnum == expected_seq_num){
                fprintf(fp, "%.*s", data_arr[buffer.seqnum].length, data_arr[buffer.seqnum].payload);
                expected_seq_num++;
                data_arr[buffer.seqnum].length = 0;

                int i;
                for (i = buffer.seqnum + 1; i < buffer.file_packet_size; i++) {
                    //for the packets that have been received and buffered
                    if (data_arr[i].length != 0) {
                        fprintf(fp, "%.*s", data_arr[i].length, data_arr[i].payload);
                        expected_seq_num++;

                        // mark data that have been uploaded to file
                        data_arr[i].length = 0;                      
                    }
                    else 
                        break;
                    
                }                 
                //expected_seq_num = (expected_seq_num + i * buffer.length);
            } 
            build_packet(&ack_pkt, 0, expected_seq_num, 0, 1, 0, NULL, 0);
        }
        else {
            build_packet(&ack_pkt, 0, expected_seq_num, 0, 1, 0, NULL, 0);
        }
        sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, 
            (struct sockaddr *) &client_addr_to, sizeof(client_addr_to));
    }

    delete[] data_arr;
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}