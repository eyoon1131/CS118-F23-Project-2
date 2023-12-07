#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MACROS
#define SERVER_IP "127.0.0.1"
#define LOCAL_HOST "127.0.0.1"
#define SERVER_PORT_TO 5002
#define CLIENT_PORT 6001
#define SERVER_PORT 6002
#define CLIENT_PORT_TO 5001
#define PAYLOAD_SIZE 1190
#define WINDOW_SIZE 16
#define TIMEOUT 2
#define MAX_SEQUENCE 1024

// our implementation
#define TIMEOUT_MS 750
#define TIME_WAIT_S 60



// Packet Layout
// You may change this if you want to
struct packet {
    unsigned short seqnum;
    unsigned short acknum;
    char ack;
    char last;
    unsigned short length;
    char payload[PAYLOAD_SIZE];
    unsigned short file_packet_size;
};

// Utility function to build a packet
void build_packet(struct packet* pkt, unsigned int seqnum, unsigned int acknum, char last, char ack,unsigned int length, const char* payload, int file_packet_size) {
    pkt->seqnum = seqnum;
    pkt->acknum = acknum;
    pkt->ack = ack;
    pkt->last = last;
    pkt->length = length;
    memcpy(pkt->payload, payload, length);
    pkt->file_packet_size = file_packet_size;
}

// Utility function to print a packet
void printRecv(struct packet* pkt) {
    printf("RECV %d %d%s%s\n", pkt->seqnum, pkt->acknum, pkt->last ? " LAST": "", (pkt->ack) ? " ACK": "");
}

void printSend(struct packet* pkt, int resend) {
    if (resend)
        printf("RESEND %d %d%s%s\n", pkt->seqnum, pkt->acknum, pkt->last ? " LAST": "", pkt->ack ? " ACK": "");
    else
        printf("SEND %d %d%s%s\n", pkt->seqnum, pkt->acknum, pkt->last ? " LAST": "", pkt->ack ? " ACK": "");
}

#endif