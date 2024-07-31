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
#include "hw_3.h"

#define BUF_SIZE 100
#define NAME_SIZE 20
#define MAX_CLNT 256
#define MAX_DIR_LEN 256
#define MAX_FILE_NUM 1024
#define MAX_CMD_LEN 3



int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;
Client_info client[MAX_CLNT];
fd_set reads, cpy_reads;

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    struct timeval timeout;
    socklen_t adr_sz;
    int fd_max, str_len, fd_num, i;
    char buf[BUF_SIZE];

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;

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
            if (FD_ISSET(i, &cpy_reads)) // i번째 descriptor에서 read 이벤트가 발생했는지 보는 list에서 i번째에 이벤트가 있다면
            {
                if (i == serv_sock) // connection request!
                                    // 서버에 event가 생겼다. = 서버에 Client가 연결을 요청했다.
                {
                    adr_sz = sizeof(clnt_adr);
                    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);

                    client[clnt_sock].sockfd = clnt_sock;

                    pthread_mutex_lock(&mutx);          // 전체 유저 수 +1 추가
                    clnt_socks[clnt_cnt++] = clnt_sock; // ~
                    pthread_mutex_unlock(&mutx);        // ~

                    FD_SET(clnt_sock, &reads); // client socket의 id 값을 세팅한다.

                    if (fd_max < clnt_sock)
                        fd_max = clnt_sock; // 가장 큰 fd 값 업데이트 , 그 값까지는 돌아야 하니까
                    printf("connected client: %d \n", clnt_sock);
                    strcpy(client[clnt_sock].dir, "/home/s22000082/");
                }
                else // read message!
                     // = client에서 이벤트가 생겼다. ##를 해줘라고 옴
                {
                    pthread_t t_id;

                    int event_sock = i;
                    printf("client: %d\n", event_sock);
                    pthread_create(&t_id, NULL, handle_clnt, (void *)&event_sock);
                    pthread_detach(t_id);
                    FD_CLR(i, &reads);
                }
            }
        }
    }
    close(serv_sock);
    return 0;
}

void change_dir(Client_info info, char param_in[MAX_DIR_LEN])
{
    int sock = info.sockfd;
    printf("%d th clnt\n", sock);

    File_info files_list[MAX_FILE_NUM];
    struct dirent *entry = NULL; // 현재 작업 주소에 있는 자료들

    int param_len = strlen(param_in);

    if (strcmp(param_in, "..") == 0)
    {
        // 가장 뒤는 /다. = len-2부터 시작해야함
        // 가장 뒤에 있는 '/' 이전부터 시직해서 그 전 '/'까지 찾는다.
        // '/' 위 다음을 잘라버린다.

        // 원래 dir 크기 - 방금 읽은 길이 크기만큼을 strncpy
        // strcpy로 dir 업데이트

        int len = strlen(client[sock].dir);
        if (len > 1)
        {
            int i;
            for (i = len - 2; i >= 0; i--)
            {
                if (client[sock].dir[i] == '/')
                {
                    client[sock].dir[i + 1] = '\0';
                    break;
                }
            }
            // 만약 루트 디렉토리까지 왔다면 "/"만 남김
            if (i < 0)
            {
                strcpy(client[sock].dir, "/");
            }
        }
        // 현재 디렉토리 정보 업데이트
        strcpy(client[sock].last_dir, "..");
    }
    else
    {
        // 어디로 갈건지
        strncat(client[sock].dir, param_in, param_len);
        strcat(client[sock].dir, "/");

        printf("dir 주소는 %s", client[sock].dir);

        // 2.그 나머지에 해당하는 문자열 -> dir open.
        DIR *dp;

        FILE *fp;
        FILE *fp_tmp;

        if ((dp = opendir(client[sock].dir)) == NULL)
        {
            error_handling("Wrong path Entered");
        }

        // 3.이동했을 때 있는 정보들 담기
        int num_file = 0;
        char full_path[MAX_DIR_LEN + 256];

        while ((entry = readdir(dp)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            strncpy(files_list[num_file].file_name, entry->d_name, sizeof(files_list[num_file].file_name) - 1);

            unsigned int file_name_size = sizeof(files_list[num_file].file_name);
            files_list[num_file].file_name[file_name_size - 1] = '\0';

            snprintf(full_path, sizeof(full_path), "%s/%s", client[sock].dir, entry->d_name);

            fp = fopen(full_path, "rb");
            fp_tmp = fopen(full_path, "rb");

            if (fp == NULL)
            {
                files_list[num_file].size = 0;
            }
            else
            {
                fseek(fp_tmp, 0, SEEK_END);
                files_list[num_file].size = ftell(fp_tmp);
                fclose(fp_tmp);
            }

            num_file++;
        }
        closedir(dp);
        printf("moved\n");
    }

    // 4.해당 clnt의 구조체 업데이트
    strcpy(client[sock].last_dir, param_in);

    printf("finished\n");
}

void upload_file(Client_info info, char param_in[MAX_DIR_LEN])
{
    printf("hit \n");
    int sock = info.sockfd;

    DIR *dp;
    char tmp_dir[MAX_DIR_LEN+256];
    int param_len = strlen(param_in);

    strcpy(tmp_dir, client[sock].dir);
	printf("hit 1\n");

	strncat(tmp_dir, param_in, param_len);
	printf("hit 2\n");
    printf("주소는 %s\n",tmp_dir);
    
    printf("hit 2\n");

    if ((dp = opendir(client[sock].dir)) == NULL)
    {
        error_handling("Wrong path Entered");
    }

    // 1. 현재 dir에 받는 파일 이름으로 fopen
    FILE *fp;

    fp = fopen(tmp_dir, "wb");
    if (fp == NULL)
    {
        error_handling("Failed to open file");
    }
    printf("hit 4\n");

    char message[BUF_SIZE];
    int read_cnt;
    unsigned int size;

    if (read(sock, &size, sizeof(size)) == -1)
    {
        error_handling("Failed to read file size");
    }

    printf("파일 크기는 %d\n",size);

    // 2. client에서 write한 값을 read한다.

    int write_cnt = 0;
	while ((read_cnt = read(sock, message, BUF_SIZE)) != 0)
	{											 
        printf("1반복\n"); // 버퍼가 비어있지 않다면
		fwrite((void *)message, 1, read_cnt, fp); // 그 내용을 쓴다.
		write_cnt += read_cnt;
         printf("2반복\n");
		if (write_cnt >= size)
		{
			break;
		}
	}
    puts("\nReceived file data");

    fclose(fp);
    printf("finished\n");
}

void download_file(Client_info info, char param_in[MAX_DIR_LEN])
{
    int sock = info.sockfd;
    printf("download \n");

    // 현재 dir에서 입력받은 파일을 연다.
    FILE *fp;
    FILE *fp_tmp;
    unsigned int size;
    char message[BUF_SIZE];

    DIR *dp;
    char tmp_dir[MAX_DIR_LEN+256];
    int param_len = strlen(param_in);

    strcpy(tmp_dir, client[sock].dir);
	printf("hit 1\n");

	strncat(tmp_dir, param_in, param_len);
	printf("hit 2\n");
    printf("주소는 %s\n",tmp_dir);
    

    fp = fopen(tmp_dir, "rb");
    if (fp == NULL)
    {
        error_handling("Wrong file Entered");
    }

    fp_tmp = fopen(tmp_dir, "rb");
    fseek(fp_tmp, 0, SEEK_END);
    size = ftell(fp_tmp);
    fclose(fp_tmp);

    write(sock, &size, sizeof(size));

    int read_cnt = 0;

    while (1)
    {
        read_cnt = fread((void *)message, 1, BUF_SIZE, fp); // 버퍼에 fp가 가리키는 걸 읽어서 담는다.
        if (read_cnt < BUF_SIZE)                            // 버퍼 사이즈보다 읽은 게 적으면
        {
            write(sock, message, read_cnt); // 그냥 버퍼에 있는 거 쓰면 됨
            break;
        }
        write(sock, message, BUF_SIZE); // 그게 아니면 버퍼 크기만큼만 적는다.
    }
    fclose(fp);

    // server : write , client : read
}

void list_up(Client_info info)
{
    int sock = info.sockfd;
    File_info files_list[MAX_FILE_NUM];
    struct dirent *entry = NULL; // 현재 작업 주소에 있는 자료들

    printf("ls dir 주소는 %s\n", client[sock].dir);

    // 2.그 나머지에 해당하는 문자열 -> dir open.
    DIR *dp;

    if ((dp = opendir(client[sock].dir)) == NULL)
    {
        error_handling("Wrong path Entered");
    }

    // 3.이동했을 때 있는 정보들 담기
    int num_file = 0;
    char full_path[MAX_DIR_LEN + 256];

    while ((entry = readdir(dp)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        strncpy(files_list[num_file].file_name, entry->d_name, sizeof(files_list[num_file].file_name) - 1);

        unsigned int file_name_size = sizeof(files_list[num_file].file_name);
        files_list[num_file].file_name[file_name_size - 1] = '\0';

        snprintf(full_path, sizeof(full_path), "%s/%s", client[sock].dir, entry->d_name);

        FILE *fp = fopen(full_path, "rb");
        FILE *fp_tmp = fopen(full_path, "rb");

        if (fp == NULL)
        {
            files_list[num_file].size = 0;
        }
        else
        {
            fseek(fp_tmp, 0, SEEK_END);
            files_list[num_file].size = ftell(fp_tmp);
            fclose(fp_tmp);
        }

        num_file++;
    }
    closedir(dp);

    write(sock, &num_file, sizeof(num_file));

    for (int i = 0; i < num_file; i++)
        write(sock, &files_list[i], sizeof(File_info));

    printf("finished\n");
}
void *handle_clnt(void *arg)
{
    int clnt_sock = *((int *)arg);
    int str_len = 0, i;
    Command com;
    memset(&com, 0, sizeof(com));

    while ((str_len = read(clnt_sock, &com, sizeof(com))) > 0)
    {

        if (str_len <= 0)
        {
            // 연결 종료 또는 읽기 오류
            if (str_len == -1)
                error_handling("read() error");

            FD_CLR(clnt_sock, &reads);
            close(clnt_sock);
            printf("closed client: %d \n", clnt_sock);

            // 클라이언트 목록에서 제거
            pthread_mutex_lock(&mutx);
            for (i = 0; i < clnt_cnt; i++)
            {
                if (clnt_sock == clnt_socks[i])
                {
                    while (i++ < clnt_cnt - 1)
                        clnt_socks[i] = clnt_socks[i + 1];
                    clnt_cnt--;
                    break;
                }
            }
            pthread_mutex_unlock(&mutx);
            return NULL;
        }

        printf("Received - command: %s, param: %s\n", com.command, com.param);

        if (strcmp(com.command, "cd") == 0)
        {
            change_dir(client[clnt_sock], com.param);
        }
        else if (strcmp(com.command, "u") == 0)
        {
            upload_file(client[clnt_sock], com.param);
        }
        else if (strcmp(com.command, "d") == 0)
        {
            download_file(client[clnt_sock], com.param);
        }
        else if (strcmp(com.command, "ls") == 0)
        {
            list_up(client[clnt_sock]);
        }
        else
        {
            // 잘못된 명령어 처리
            printf("Invalid command: %s\n", com.command);
        }
        memset(&com, 0, sizeof(com)); // 명령어 처리 후 초기화
    }

    // 클라이언트 연결 종료 후 처리
    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++)
    {
        if (clnt_sock == clnt_socks[i])
        {
            while (i++ < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            clnt_cnt--;
            break;
        }
    }
    pthread_mutex_unlock(&mutx);

    close(clnt_sock);
    return NULL;
}

void send_msg(char *msg, int len) // send to all
{
    int i;
    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutx);
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
