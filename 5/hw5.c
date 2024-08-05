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
    // option 받기
    // sender ,receiver 판단.
    int opt;
    int flag_s = 0, flag_r = 0;

    if (argc != 10 && argc != 7)
    {
        printf("Wrong command\n");
        exit(1);
    }

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
        sender_input.recv_max = atoi(argv[6]);
        strcpy(sender_input.file_name, argv[7]);
        sender_input.seg_size = atoi(argv[8]);

        // 연결 받고 들어온 유저 관리하는 sock init
        serv_sock = socket(PF_INET, SOCK_STREAM, 0);
        memset(&serv_adr, 0, sizeof(serv_adr));
        serv_adr.sin_family = AF_INET;
        serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_adr.sin_port = htons(atoi(argv[9]));

        Receiver_Info receiver_info_to_send[sender_input.recv_max];

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
                        // 새로 연결이 들어오면 해당 유저의 socket 번호를 배열에 담는다.
                        // 유저 관련 정보에 몇 번째 유저인지 / 주소 / port 를 저장할 수 있다.
                        pthread_mutex_lock(&mutx);
                        clnt_socks[clnt_cnt] = clnt_sock; // ~
                        receiver_info_to_send[clnt_cnt].nth_receiver = clnt_cnt;
                        receiver_info_to_send[clnt_cnt].addr = clnt_adr.sin_addr.s_addr;
                        // receiver_info_to_send[clnt_cnt].port = clnt_adr.sin_port;
                        // printf("%d\n",inet_addr(clnt_adr.sin_addr.s_addr));
                        clnt_cnt++;
                        pthread_mutex_unlock(&mutx); // ~
                        printf("%d명 들어옴 \n", clnt_cnt);

                        if (clnt_cnt == sender_input.recv_max)
                        {
                            // 정해둔 인원이 다 들어오면 Sending peer가 Send
                            sender_send(sender_input, receiver_info_to_send);
                        }
                    }
                }
            }
        }
    }

    // receiver일 경우
    else if (flag_r)
    {

        // 연결 요청 sock init

        // 해당 Receiver의 id 값을 받는다.
        // sender 한테 전체 seg의 갯수 , 몇 명의 유저 , 모든 유저의 정보 , seg 정보를 받는다.
        // id 값을 통해 유저 정보들 안에서 자신이 받아야 하는 seg의 갯수를 구한다.
        // 받아야 하는 seg를 받는다.

        // 다른 receiver들한테 보내는 thread
        // 다른 receiver들한테 받는 thread


        // 연결 요청 sock init
        struct sockaddr_in serv_adr, clnt_adr;
        socklen_t adr_sz;
        int sock;
        int num_seg;
        int idx = 0;
        int receiver_cnt;
        int id;

        sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock == -1)
            error_handling("socket() error");

        memset(&serv_adr, 0, sizeof(serv_adr));
        serv_adr.sin_family = AF_INET;
        serv_adr.sin_addr.s_addr = inet_addr(argv[4]);
        serv_adr.sin_port = htons(atoi(argv[5]));

        if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
            error_handling("connect() error!");

        else
            puts("Connected...........");

        // 해당 Receiver의 id 값을 받는다.
        read(sock, &id, sizeof(id));
        printf("%d번 유저입니다", id);

        // sender 한테 전체 seg의 갯수 , 몇 명의 유저 , 모든 유저의 정보 , seg 정보를 받는다.
        read(sock, &receiver_cnt, sizeof(receiver_cnt));
        Receiver_Info receiver_info[receiver_cnt];

        read(sock, &num_seg, sizeof(num_seg));
        printf("전체 seg는 %d개 입니다\n", num_seg);

        for (int i = 0; i < receiver_cnt; i++)
            read(sock, &receiver_info[i], sizeof(receiver_info));

        printf("받을 seg 수는 %d개 입니다 .\n", receiver_info[id].num_from_sender);
        receiver_info[id].port = (atoi(argv[6]));
        printf("port: %d\n", receiver_info[id].port);

        // id 값을 통해 유저 정보들 안에서 자신이 받아야 하는 seg의 갯수를 구한다.
        int to_get = receiver_info[id].num_from_sender;
        Segment final_segment[num_seg];
        Segment tmp_segment[num_seg];
        memset(&tmp_segment, 0, sizeof(Segment));
        // 받아야 하는 seg를 받는다.
        while (idx < to_get)
        {

            if (read(sock, &tmp_segment[idx], sizeof(tmp_segment)) <= 0)
            {
                error_handling("read seg error");
            }
            else
            {

                printf("===== %d 번 반복 =======\n", idx);
                printf("%d 번 seg 받음\n", tmp_segment[idx].nth_seg);
                to_get = receiver_info[tmp_segment[idx].to].num_from_sender;
                //이거 final 에 nth_seg 번째 인덱스에 넣어야 함
                idx++;
            }
        }

      

        // 다른 receiver들한테 보내는 thread
        // 다른 receiver들한테 받는 thread
        pthread_t t_receive, t_send;
        int event_sock = id;
        pthread_create(&t_receive, NULL, receiver_receive, (void *)&event_sock);
        pthread_create(&t_send, NULL, receiver_send, (void *)&event_sock);
        pthread_detach(t_receive);
        pthread_detach(t_send);
    }

    close(serv_sock);
    return 0;
}

// receiving peer 가 다른 receiver peer로 부터  받는 함수
void *receiver_receive(void *arg)
{
    int receiver_sock = *((int *)arg);
    printf("thread rr %d\n", receiver_sock);



    //
}

// receiving peer 가 다른 receiving peer에게 보내는 함수
void *receiver_send(void *arg)
{
    int receiver_sock = *((int *)arg);
    printf("thread rs %d\n", receiver_sock);

    // receiver 수 -1 만큼 반복 (본인 제외) {
    // 다른 receiver들이랑 연결
    // segment.from_sender==1 인 것만 골라서
    // segment.from_sender =0 으로 바꿔서 보낸다.
    // }
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
    int total_seg;
    int read_cnt = 0;
    int id_receiver = 0;
    int seg_cnt = 0;
    Segment segment;

    segment.buf = malloc(input.seg_size);

    for (int i = 0; i < input.recv_max; i++)
    {
        write(clnt_socks[i], &i, sizeof(i));
    }

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

    // 몇 명이 받을지 알려준다.
    for (int i = 0; i < clnt_cnt; i++)
    {
        write(clnt_socks[i], &input.recv_max, sizeof(input.recv_max));
    }

    // 각 receiver마다 몇 개를 받아야 하는지 정한 뒤 정보를 보낸다.
    total_seg = (total + input.seg_size - 1) / input.seg_size;

    for (int i = 0; i < input.recv_max; i++)
    {
        info[i].num_from_sender = total_seg / input.recv_max;
    }

    int extra_segments = total_seg % input.recv_max;
    for (int i = 0; i < extra_segments; i++)
    {
        info[i].num_from_sender++;
    }

    // 디버깅 출력
    for (int i = 0; i < input.recv_max; i++)
    {
        printf("%d는 %d개 받을 예정\n", i, info[i].num_from_sender);
    }
    // for (int i = 0; i < input.recv_max; i++)
    // {
    //     info[i].num_from_sender = total_seg / input.recv_max;
    //     if (i <= (total_seg % input.recv_max))
    //     {
    //         info[i].num_from_sender++;
    //     }
    //     printf("%d는 %d개 받을 예정\n", i, info[i].num_from_sender);
    // }

    printf("seg 는 %d개 입니다", total_seg);
    printf("크기는 %ld 입니다\n", total);

    // 한 receiver당 최대 몇 개의 seg가 갈지,  알려준다.
    for (int i = 0; i < clnt_cnt; i++)
    {
        write(clnt_socks[i], &total_seg, sizeof(total_seg));
    }

    for (int i = 0; i < clnt_cnt; i++)
    {
        for (int j = 0; j < clnt_cnt; j++)
        {

            write(clnt_socks[i], &info[j], sizeof(info[j]));
        }
    }

    segment.total_size = total;
    strcpy(segment.file_name, input.file_name);
    segment.is_from_sender = 1;

    while (seg_cnt < total_seg)
    {
        printf("hit");
        segment.nth_seg = seg_cnt;
        segment.to = id_receiver;
        read_cnt = fread(segment.buf, 1, input.seg_size, fp);
        if (read_cnt <= 0)
        {
            error_handling("read error");
        }
        segment.seg_size = read_cnt;

        if (write(clnt_socks[id_receiver], &segment, sizeof(Segment)) <= 0)
            error_handling("write error");

        printf("=====cnt : %d, to: %d , total : %d\n", seg_cnt, clnt_socks[id_receiver], total_seg);
        id_receiver = (id_receiver + 1) % input.recv_max;
        seg_cnt++;
    }

    printf("done");
    printf("byee");
    free(segment.buf);
    fclose(fp);
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
