#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
#define MAX_DIR_LEN 256
#define MAX_FILE_NUM 126
#define MAX_CMD_LEN 3


typedef struct Command
{
	char command[MAX_CMD_LEN];	 // 어떤 명령을 하는지
	char param[MAX_DIR_LEN]; // 명령에 대한 인자값
} Command;

typedef struct File_info
{
	char file_name[100];
	unsigned int size;
} File_info;

void error_handling(char *message);

int main(int argc, char *argv[])
{
	int sock;
	char input[MAX_DIR_LEN];
	char message[BUF_SIZE];
	int str_len, recv_len, recv_cnt;
	struct sockaddr_in serv_adr;
	int num_file;
	char com[BUF_SIZE];
	char path_param[BUF_SIZE];

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

	while (1)
	{
		Command com;
		char input[MAX_DIR_LEN];
    com.command[0] = '\0';
    com.param[0] = '\0';
			fputs("==================\n", stdout);

		fputs("Input message(Q to quit): ", stdout);

    printf("Enter command: ");
	printf("Enter command: ");
    if (fgets(input, sizeof(input), stdin) == NULL) {
			error_handling("input error");
    }
    input[strcspn(input, "\n")] = '\0';

	char *token = strtok(input, " ");
    if (token != NULL) {
        snprintf(com.command, sizeof(com.command), "%s", token);
    }

    token = strtok(NULL, " ");
    if (token != NULL) {
        snprintf(com.param, sizeof(com.param), "%s", token);
    }

    printf("Command: %s\n", com.command);
    printf("Parameter: %s\n", com.param);


		printf("Sending - command: %s, param: %s\n", com.command, com.param);
	
		if (strcmp(com.command, "q\n") ==0|| strcmp(com.command, "Q\n")==0)
			break;

		str_len = write(sock, &com, sizeof(com));

		if (strcmp(com.command, "cd") == 0)
		{
			printf("moved\n");

		}
		else if (strcmp(com.command, "u") == 0)
		{
			// 올리는 처리
			FILE *fp;
			FILE *fp_tmp;
			unsigned int size;
			fp = fopen(com.param, "rb");	 // 현재 이 파일을 열고, 이 파일을 여는 pointer 값을 return한다.
			fp_tmp = fopen(com.param, "rb"); // 현재 이 파일을 열고, 이 파일을 여는 pointer 값을 return한다.

			fseek(fp_tmp, 0, SEEK_END);
			size = ftell(fp_tmp);
			fclose(fp_tmp);

			if (fp == NULL)
			{
				error_handling("Wrong file Entered");
			}

			write(sock,&size,sizeof(size));

			int read_cnt = 0;

			while (1)
			{
				read_cnt = fread((void *)message, 1, BUF_SIZE, fp); // 버퍼에 fp가 가리키는 걸 읽어서 담는다. (저장할 곳 , 읽는 문자열 크기 , 반복 횟수 , 읽을 파일 포인터 )
				if (read_cnt < BUF_SIZE)							// 버퍼 사이즈보다 읽은 게 적으면
				{
					write(sock, message, read_cnt); // 그냥 버퍼에 있는 거 쓰면 됨
					break;
				}
				write(sock, message, BUF_SIZE); // 그게 아니면 버퍼 크기만큼만 적는다.
			}
			fclose(fp);
		}

		// 파일 다운로드
		else if (strcmp(com.command, "d") == 0)
		{
			// 받는 처리
			FILE *fp;
	fp = fopen(com.param, "wb");
	char message[BUF_SIZE];
	int read_cnt;
	unsigned int size;

	read(sock, &size, sizeof(size));

	// 2. client에서 write한 값을 read한다.


	int write_cnt = 0;
	while ((read_cnt = read(sock, message, BUF_SIZE)) != 0)
	{											  // 버퍼가 비어있지 않다면
		fwrite((void *)message, 1, read_cnt, fp); // 그 내용을 쓴다.
		write_cnt += read_cnt;
		if (write_cnt >= size)
		{ 
			break;
		}
	}
	puts("\nReceived file data");
	fclose(fp);
		}
		else if (strcmp(com.command, "ls") == 0){
				File_info files_list[MAX_FILE_NUM];

			if ((read(sock, &num_file, sizeof(num_file))) > 0)
			{
				for (int i = 0; i < num_file; i++)
					// while () //100M 읽을 예정인데, 10M 읽었을 때, 포인터를 그 다음으로 옮겨야 함
					read(sock, &files_list[i], sizeof(File_info));
			}

			for (int i = 0; i < num_file; i++)
			{
				printf("%d : %s  | %d \n", i, files_list[i].file_name, files_list[i].size);
			}
		}else {
			error_handling("Invalid input");
		}
	}
	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
