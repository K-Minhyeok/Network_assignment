// header.h

#ifndef hw3_H
# define hw3_H

# include <stdio.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define MAX_CLNT 256
#define MAX_DIR_LEN 256
#define MAX_FILE_NUM 1024
#define MAX_CMD_LEN 3

typedef struct Client_info
{
    int sockfd;                 // socket num (저장 필요)
    char dir[MAX_DIR_LEN];      // 현재 작업 위치 (저장 필요)
    char last_dir[MAX_DIR_LEN]; // 가장 최근 파일 이름 (cd ..했을 때 뺴기 위함) (저장 필요)
} Client_info;

typedef struct File_info
{
    char file_name[100];
    unsigned int size;
} File_info;

typedef struct Content
{
    char message[BUF_SIZE];
} Content;

typedef struct Command
{
    char command[MAX_CMD_LEN]; // 어떤 명령을 하는지
    char param[MAX_DIR_LEN];   // 명령에 대한 인자값
} Command;

void *handle_clnt(void *arg);
void send_msg(char *msg, int len);
void error_handling(char *msg);
void change_dir(Client_info info, char param_in[MAX_DIR_LEN]);
void upload_file(Client_info info, char param_in[MAX_DIR_LEN]);
void download_file(Client_info info, char param_in[MAX_DIR_LEN]);
void list_up(Client_info info);

#endif