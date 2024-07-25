#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#define BUF_SIZE 30

typedef struct Packet_info
{
    char content[BUF_SIZE];
    int p_seq;
    int ack;
    int type;
} Packet_info;

void error_handling(char *message);

int check_id(Packet_info packet);


int main(int argc, char *argv[])
{
    int sock;
    char message[BUF_SIZE];
    int rec_len;
    socklen_t adr_sz;
    Packet_info pack_send; // 보내는 용도
    Packet_info pack_recv; // 받는 용도
    FILE *fp, *fp_tmp;
    int read_cnt;
    int ack_check=1;
    int pack_id = 0;
    unsigned int size;
    

 
    struct sockaddr_in serv_adr, from_adr;
    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    struct timeval time;
    time.tv_sec = 0;
    time.tv_usec = 500000;
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char *)&time, sizeof(time));
    if (sock == -1)
        error_handling("socket() error");

    
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    

    fp = fopen("hello", "rb");     
    fp_tmp = fopen("hello", "rb"); // 현재 이 파일을 열고, 이 파일을 여는 pointer 값을 return한다.

    fseek(fp_tmp, 0, SEEK_END);
    size = ftell(fp_tmp);
    sendto(sock, &size, sizeof(int), 0,
               (struct sockaddr *)&serv_adr, sizeof(serv_adr));

    clock_t start = clock();
    while (1)
    {

        if(ack_check==1){
        pack_send.p_seq = pack_id;
        pack_send.ack = 0;
        pack_send.type = 1;

        read_cnt = fread((void *)message, 1, BUF_SIZE, fp);
        strcpy(pack_send.content,message);
        sendto(sock, &pack_send, sizeof(Packet_info), 0,
               (struct sockaddr *)&serv_adr, sizeof(serv_adr));
        }
         if(ack_check==0)
             sendto(sock, &pack_send, sizeof(Packet_info), 0,
               (struct sockaddr *)&serv_adr, sizeof(serv_adr));
    
        adr_sz = sizeof(serv_adr);
        
        rec_len = recvfrom(sock, &pack_recv, sizeof(Packet_info), 0,
                           (struct sockaddr *)&serv_adr, &adr_sz);
       
        
        if(pack_recv.type == 999)break;
     
        if( rec_len >0 && pack_recv.p_seq==0 && pack_recv.ack==pack_id+1){
            printf("+ %d recieved \n" ,pack_id);
            ack_check = 1;
            pack_id++;

        }else{
            printf("- not recieved\n");
            ack_check = 0;
            printf("받 %d  \n" ,pack_recv.ack);
            printf("필 %d  \n" ,pack_id);
            printf("seq %d  \n" ,pack_recv.p_seq);
            printf("len %d  \n" ,rec_len);



            }

    }
    close(sock);
    printf("\n통신이 끝났습니다.");
    clock_t end = clock();
    double spent = end - start;

    printf("소요 시간 : 0.%lf\n",spent);
    printf("파일 사이즈 : %d", size);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}