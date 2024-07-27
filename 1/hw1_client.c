#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "hw1_client.h"
#include <ctype.h>

#define BUF_SIZE 1024
#define MAX_NUM_FILE 20

void error_handling(char *message);

int main(int argc, char *argv[])
{

    int sock;
    char message[BUF_SIZE];
    int num_file;
    int str_len;
    int read_cnt;

    struct sockaddr_in serv_adr;
    File_info files_list[MAX_NUM_FILE];

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);

    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));

    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("connect() error!");

    else
        puts("Connected...........");

    ;

    if ((read(sock, &num_file, sizeof(num_file))) > 0)
    {
        for (int i = 0; i < num_file; i++)
            // while () //100M 읽을 예정인데, 10M 읽었을 때, 포인터를 그 다음으로 옮겨야 함  
            read(sock, &files_list[i], sizeof(File_info));

    while (1)
    {
            int option;

        for (int i = 0; i < num_file; i++)
        {
            printf("%d : %s  | %d \n", i, files_list[i].file_name, files_list[i].size);
        }

        fputs("Select a file to download: \n", stdout);
        fputs("press 999 to exit \n", stdout);

        scanf("%d", &option);

        if(option == 999){
            break;
        }
        

        write(sock, &option, sizeof(option));

        FILE *fp;

        fp = fopen(files_list[option].file_name, "wb"); // 파일 열기
        memset(message, 0, sizeof(message));
        int write_cnt=0;
        while ((read_cnt = read(sock, message, BUF_SIZE)) != 0)
        {                                             // 버퍼가 비어있지 않다면
            fwrite((void *)message, 1, read_cnt, fp); // 그 내용을 쓴다.
            write_cnt+= read_cnt;
            if(write_cnt>=files_list[option].size){
                break;
            }
        }
        puts("\nReceived file data");
        fclose(fp);
    }

    close(sock);
    return 0;
}
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
