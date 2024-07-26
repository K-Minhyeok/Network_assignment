#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>


#define BUF_SIZE 4096

typedef struct Packet_info
{
    char content[BUF_SIZE];
    int p_seq;
    int ack;
    int type;
    int read_size;

} Packet_info;

void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sock;
    char message[BUF_SIZE];
    int str_len;
    socklen_t clnt_adr_sz;
    Packet_info pack_send; // 보내는 용도
    Packet_info pack_recv; // 받는 용도
    FILE *fp;
    int read_cnt;
    int pack_id = 0;
    unsigned int size;
    int write_cnt = 0;

    struct sockaddr_in serv_adr, clnt_adr;
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    struct timeval time;
    time.tv_sec = 0;
    time.tv_usec = 500000;


    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    setsockopt(serv_sock,SOL_SOCKET,SO_SNDTIMEO,(char *)&time, sizeof(time));

    
    if (serv_sock == -1)
        error_handling("UDP socket creation error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    fp = fopen("whtasf2.jpg", "wb"); // 현재 이 파일을 열고, 이 파일을 여는 pointer 값을 return한다.

    clnt_adr_sz = sizeof(clnt_adr);
    recvfrom(serv_sock, &size, sizeof(unsigned int), 0,
             (struct sockaddr *)&clnt_adr, &clnt_adr_sz);   //얼마만큼 크기의 파일을 받는지에 대한 정보.
    printf("size %d \n", size);

    while (1)
    {

        int rec_size = recvfrom(serv_sock, &pack_recv, sizeof(Packet_info), 0,
                                (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        if (rec_size > 0 && pack_recv.type == 1 && pack_recv.p_seq == pack_id)
        {   //받은 파일이 있고, 그 안에 ACK == 확인 , 주고 받는 식별 번호가 같은 경우
            //즉 제대로 받은 경우

            printf("+ [packet]: %d \n", pack_recv.p_seq);

            pack_send.p_seq = 0;
            pack_send.type = 0;
            pack_send.ack = pack_recv.p_seq + 1;    //그 다음 거 줘도 된다고 알려줌
            sendto(serv_sock, &pack_send, sizeof(Packet_info), 0,
                   (struct sockaddr *)&clnt_adr, clnt_adr_sz);
            if (write_cnt + pack_recv.read_size >= size)
            {
                int last = fwrite((void *)pack_recv.content, 1, size - write_cnt, fp); // 그 내용을 쓴다.
                write_cnt += last;
                break;
            }
            int write_size = fwrite((void *)pack_recv.content, 1, pack_recv.read_size, fp); // 그 내용을 쓴다.
        
            write_cnt += pack_recv.read_size;
            pack_id++;

        }
        else
        {
            printf("- Not Received : \n");
            
        }
    }
    fclose(fp);
    printf("saved\n");

    printf("size = %d\n", size);
    printf("wrtcnt = %d\n", write_cnt);

    pack_send.p_seq = 0;
    pack_send.type = 999;
    pack_send.ack = pack_recv.p_seq + 1;
    sendto(serv_sock, &pack_send, sizeof(Packet_info), 0,
           (struct sockaddr *)&clnt_adr, clnt_adr_sz);

    close(serv_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
