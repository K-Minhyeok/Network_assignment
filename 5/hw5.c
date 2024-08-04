#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/select.h>
#include <pthread.h>
#include <math.h>
#include "hw5.h"



int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct timeval timeout;
    int fd_max, str_len, fd_num, i;
    int clnt_max;

    if (argc != 8 && argc != 5)
    {
        printf("Wrong command\n");
        exit(1);
    }
    // option 받기
    // sender ,receiver 판단.
    int opt;
    int flag_s = 0, flag_r = 0;

    // 옵션이 어떻게 들어오는지 판단한다.
    while ((opt = getopt(argc, argv, "srnfgap")) != -1)
    {
        switch (opt)
        {
        case 's':
            flag_s = 1;
            break;
        case 'r':
            flag_r = 1;
            break;
        case '?':
            printf("Unknown flag : %c \n", opt);
        }
    }

    // sender일 경우
    if (flag_s)
    {
        struct sockaddr_in serv_adr, clnt_adr;
        socklen_t adr_sz;

        // 명령어 별로 인자들을 구조체 값에 넣어준다.
        // ex) 최대 인원 , 파일 이름 , seg 크기
        Sender_Input sender_input;
        sender_input.recv_max = atoi(argv[5]);
        strcpy(sender_input.file_name, argv[6]);
        sender_input.seg_size = atoi(argv[7]);

        // 연결 받고 들어온 유저 관리하는 sock init
        serv_sock = socket(PF_INET, SOCK_STREAM, 0);
        memset(&serv_adr, 0, sizeof(serv_adr));
        serv_adr.sin_family = AF_INET;
        serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
        // serv_adr.sin_port = htons(atoi(argv[1]));
        serv_adr.sin_port = htons(21198); // client port 와 같아야 함.

        Receiver_Info receiver_info[sender_input.recv_max];

        if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
            error_handling("bind() error");
        if (listen(serv_sock, 5) == -1)
            error_handling("listen() error");

        FD_ZERO(&reads);
        FD_SET(serv_sock, &reads);
        fd_max = serv_sock;

        printf("최대 : %d\n", sender_input.recv_max);
        printf("파일 이름 %s\n", sender_input.file_name);
        printf("seg 크기 %ld\n", sender_input.seg_size);

        while (1)
        {

            cpy_reads = reads;
            timeout.tv_sec = 5;
            timeout.tv_usec = 5000;
            // select 해서 read 관련 이벤트가 발생한 곳을 찾는다.
            if ((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == -1)
                break;

            if (fd_num == 0)
                continue;

            for (i = 0; i < fd_max + 1; i++)
            {
                // i번째 descriptor에서 read 이벤트가 발생했는지 보는 list에서 i번째에 이벤트가 있다면
                if (FD_ISSET(i, &cpy_reads))
                {
                    // connection request!
                    // 서버에 event가 생겼다. = 서버에 Client가 연결을 요청했다.

                    if (i == serv_sock)
                    {
                        adr_sz = sizeof(clnt_adr);
                        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);
                        FD_SET(clnt_sock, &reads); // client socket의 id 값을 세팅한다.

                        if (fd_max < clnt_sock)
                            fd_max = clnt_sock; // 가장 큰 fd 값 업데이트 , 그 값까지는 돌아야 하니까

                        // 유저 관리 정보 업데이트
                        pthread_mutex_lock(&mutx);
                        clnt_socks[clnt_cnt] = clnt_sock; // ~
                        receiver_info[clnt_cnt].nth_receiver = clnt_cnt;
                        receiver_info[clnt_cnt].addr = clnt_adr.sin_addr.s_addr;
                        receiver_info[clnt_cnt].port = clnt_adr.sin_port;
                        clnt_cnt++;
                        pthread_mutex_unlock(&mutx); // ~
                        printf("%d명 들어옴 \n", clnt_cnt);

                        if (clnt_cnt == sender_input.recv_max)
                        {
                            // 정해둔 인원이 다 들어오면 Sending peer가 Send
                            sender_send(sender_input, receiver_info);
                        }
                    }
                }
            }
        }

        // sender handler로 각 쪼개진 내용을 보낸다.
    }

    // receiver일 경우
    else if (flag_r)
    {
        // 연결 요청 sock init

        struct sockaddr_in serv_adr, clnt_adr;
        socklen_t adr_sz;
        int sock;
        int num_seg;

        sock = socket(PF_INET , SOCK_STREAM, 0);
        if (sock == -1)
            error_handling("socket() error");

        memset(&serv_adr, 0, sizeof(serv_adr));
        serv_adr.sin_family = AF_INET;
        serv_adr.sin_addr.s_addr = inet_addr(argv[3]);
        serv_adr.sin_port = htons(atoi(argv[4]));

        if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
            error_handling("connect() error!");

        else
            puts("Connected...........");

        read(sock,&num_seg,sizeof(num_seg));
        printf("seg 는 %d개 입니다",num_seg );

        

    }

    close(serv_sock);
    return 0;
}

// receiving peer 가 다른 receiver peer로 부터  받는 함수
void *receiver_receive(void *arg)
{

}

// receiving peer 가 다른 receiving peer에게 보내는 함수
void *receiver_send(void *arg)
{

}
 
// sending peer가 보내는 함수
void sender_send(Sender_Input input, Receiver_Info info[])
{

    // v receiver 들에게 서로에 대한 모든 연결 정보를 보내준다.
    // v input 에서 입력한 파일을 열고 파일의 크기를 구한다.
    // v segment에 공통적인 것들만 미리 담아놔준다.
    //  (해당 파일의 크기) / (Segment의 크기) 를 통해  Segment의 갯수를 구해서 보낸다..
    // v 파일을 읽고 각 내용과 대응되는 정보들을 segment에 넣고 보낸다.
    // v + 보내는 크기가 Segment보다 작아지면 그만

    FILE *fp;
    FILE *fp_tmp; // 파일 사이즈 구하는 용도
    long total;
    int num_from_sender;
    int read_cnt = 0;
    int i = 0;
    int seg_cnt=0;
    Segment segment;
    fp = fopen(input.file_name, "rb");
    fp_tmp = fopen(input.file_name, "rb");

    if (fp == NULL)
    {
        error_handling("fopen error");
    }
    else
    {
        fseek(fp_tmp, 0, SEEK_END);
        total = ftell(fp_tmp);
        fclose(fp_tmp);
    }

   
    num_from_sender = ceil(total / input.seg_size)+1;
    printf("seg 는 %d개 입니다",num_from_sender );
    printf("크기는 %ld 입니다",total );

    //한 receiver당 최대 몇 개의 seg가 갈지 알려준다.
    for(int i=0; i<clnt_cnt;i++){
        write(clnt_socks[i],&num_from_sender,sizeof(num_from_sender));
    }
    
    for (int i = 0; i < clnt_cnt; i++)
    {
        for (int j = 0; j < clnt_cnt; j++)
        {
            if (i == j)
                continue; // 자기 정보는 필요없음
            write(clnt_socks[i], &info[j], sizeof(info[j]));
            printf("%d가 %d정보 받음 \n",clnt_socks[i], info[j].nth_receiver);
        }
    }

    // segment에 공통적인 것들만 미리 담아놔준다.
    segment.total_size = total;
    strcpy(segment.file_name, input.file_name);
    segment.is_from_sender = 1;
    segment.has_buf = 0;
    printf("여기까지 옴");
    
    while (read_cnt < total)
    {

        read_cnt = fread((void *)segment.buf, 1, input.seg_size, fp); // 버퍼에 fp가 가리키는 걸 읽어서 담는다. (저장할 곳 , 읽는 문자열 크기 , 반복 횟수 , 읽을 파일 포인터 )
        segment.seg_size = read_cnt;
        segment.nth_seg = seg_cnt;
        segment.has_buf = 1;

        if (read_cnt < input.seg_size) // 버퍼 사이즈보다 읽은 게 적으면
        {
            write(clnt_socks[i], &segment, read_cnt); // 그냥 버퍼에 있는 거 쓰면 됨
            break;
        }
        write(clnt_socks[i], segment.buf, input.seg_size); // 그게 아니면 버퍼 크기만큼만 적는다.
        total += read_cnt;
        i = ((i + 1) % input.recv_max);
        seg_cnt++;
    }
    fclose(fp);
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
