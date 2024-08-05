// header.h

#ifndef hw5_H
# define hw5_H

# include <stdio.h>
# include <sys/socket.h>


#define BUF_SIZE 60
#define MAX_CLNT 10

int clnt_cnt = 0; // 전체 유저 수
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;
fd_set reads, cpy_reads;


typedef struct Sender_Input
{
    int recv_max;
    char file_name[BUF_SIZE];
    long seg_size;

} Sender_Input;

typedef struct Segment
{
    long total_size;          // 공통
    char file_name[BUF_SIZE]; // 공통
    long seg_size;            // 마지막 하나가 다르기 때문에 공통이 아님. = 이걸로 마지막 seg 판단
    char *buf;
    int nth_seg;
    int to;
    int is_from_sender; // 1 = is from sender , 0 = from receiver
} Segment;

typedef struct Receiver_Info
{
    int nth_receiver;
    int num_from_sender;
    in_port_t port; // 해당 receiver의 port번호
    in_addr_t addr; // 해당 receiver의 address

} Receiver_Info;

void error_handling(char *msg);
void sender_send(Sender_Input input, Receiver_Info info[]);
void *receiver_receive(void *arg);
void *receiver_send(void *arg);


#endif