#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dirent.h>
#include "hw1_server.h"

#define BUF_SIZE 30
#define PATH_MAX 4096
#define MAX_NUM_FILE 20

void error_handling(char *message);

int main(int argc, char *argv[])
{

    int serv_sock, clnt_sock;    // server 와 client 의 주소를 저장하기 위한 변수
    char buf[BUF_SIZE];          // 메시지 저장용
    char path[PATH_MAX];         // 현재 작업하는 Path 를 절대경로로 받는다.
    int str_len, i;              // 메시지 길이 (반복문 도는 용도)
    struct sockaddr_in serv_adr; // 서버 주소 관련 정보 구조체
    struct sockaddr_in clnt_adr; // Client 주소 관련 정보 구조체
    socklen_t clnt_adr_sz;       //
    DIR *dp;                     // dir 포인터
    FILE *fp_tmp;
    struct dirent *entry = NULL; // 현재 작업 주소에 있는 자료들
    int read_cnt;
    File_info files_list[MAX_NUM_FILE];
    int idx;

    // 현재 경로 파악
    if (getcwd(path, PATH_MAX) == NULL)
    {
        error_handling("getcwd() error");
    }
    printf("result of cwd is %s \n", path);

    // 현재 경로에 있는 파일들에 대해 structure list를 만든다.
    char *current_dir = getcwd(path, PATH_MAX); // 현재 작업 공간을 받는다. + 에러 처리
    dp = opendir(current_dir);

    // 디렉토리에 있는 파일을 읽어서, 이름+크기를 저장한다.
    int num_file = 0;
    while (((entry = readdir(dp))) != NULL)
    {
        strcpy(files_list[num_file].file_name, entry->d_name);
        fp_tmp = fopen(entry->d_name, "rb");
        fseek(fp_tmp, 0, SEEK_END);
        files_list[num_file].size = ftell(fp_tmp);
        num_file++;
    }

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    } // 입력이 제대로 들어오지 않음

    serv_sock = socket(PF_INET, SOCK_STREAM, 0); // IPv4 UDP default 를 이용하는 소켓을 만들고 해당 소켓의 fd를 return 받는다.

    if (serv_sock == -1) // 소켓이 정상적으로 작동하지 않을 경우
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));       // 메모리를 해당 server 주소부터 그 크기만큼 지정
    serv_adr.sin_family = AF_INET;                // server 의 주소 체계
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY); // IPv4 주소
    serv_adr.sin_port = htons(atoi(argv[1]));     // Port 번호

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1) // 위에서 만들 정보를 바탕으로 Bind한다. (주소 정보를 할당할 소켓 , 관련된 정보가 있는 구조체 변수의 주소값 , 구조체 변수의 길이)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1) // Client 가 (혹은 해당 Ip-Port로) 넘어오는 게 있는지 본다.
        error_handling("listen() error");

    clnt_adr_sz = sizeof(clnt_adr);

    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz); // 인자 : accept 주체, accept 요청하는 주소 ,
    if (clnt_sock == -1)
        error_handling("accept() error");
    else
        printf("Connected client %d \n", i + 1);

    // 그 list를 client로 보낸다. 파일의 갯수를 먼저 보내줘야한다.
    write(clnt_sock, &num_file, sizeof(num_file));

    for (int i = 0; i < num_file; i++)
        write(clnt_sock, &files_list[i], sizeof(File_info));

    idx = 0;
    // client에서 인덱스를 받는다. (여기서부터 while 해야함)
    while (1)
    {

        int tmp;
        if (read(clnt_sock, &tmp, sizeof(tmp)) == -1)
        {
            error_handling("index error");
        }
        

        idx = tmp;
       
        char path_tmp[PATH_MAX];
        strcpy(path_tmp, path);
        strcat(path_tmp, "/");
        strcat(path_tmp, files_list[idx].file_name);
        FILE *fp;
        fp = fopen(path_tmp, "rb"); // 현재 이 파일을 열고, 이 파일을 여는 pointer 값을 return한다.

        read_cnt = 0;

        // 밑에는 파일 읽는 거
        while (1)
        {
            read_cnt = fread((void *)buf, 1, BUF_SIZE, fp); // 버퍼에 fp가 가리키는 걸 읽어서 담는다. (저장할 곳 , 읽는 문자열 크기 , 반복 횟수 , 읽을 파일 포인터 )
            if (read_cnt < BUF_SIZE)                        // 버퍼 사이즈보다 읽은 게 적으면
            {
                write(clnt_sock, buf, read_cnt); // 그냥 버퍼에 있는 거 쓰면 됨
                break;
            }
            write(clnt_sock, buf, BUF_SIZE); // 그게 아니면 버퍼 크기만큼만 적는다.
        }
        fclose(fp);
    }
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}