#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>

#define BUF_SIZE 100
#define MAX_DIR_LEN 256
#define MAX_FILE_NUM 126
#define MAX_CMD_LEN 3

#define COLOR_RED	"\033[38;2;255;0;0m"
#define COLOR_RESET	"\033[0m"

typedef struct Search_Record
{
	char word[BUF_SIZE];
	int num_searched;
	char *find;
} Search_Record;

typedef struct Input
{
	char message[BUF_SIZE];

} Input;

void error_handling(char *message);
void set_terminal_mode(int enable);

int main(int argc, char *argv[])
{
	int sock;
	char message_line[BUF_SIZE];
	int str_len, recv_len, recv_cnt;
	struct sockaddr_in serv_adr;
	char com[BUF_SIZE];
	char path_param[BUF_SIZE];
	int input_len;

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

	// set_terminal_mode(1);
	int idx = 0;
	char ch;
	while (1)
	{

		Input input;
		input.message[0] = '\0';

		int num_result;

		printf("=========== Enter a word ===========\n");
		if (scanf("%s", input.message) < 1)
		{
			error_handling("scanf() error");
		}
		input_len = strlen(input.message);

		while (strlen(input.message) < 1)
			;

		write(sock, &input, sizeof(input));
		
		if (read(sock, &num_result, sizeof(num_result)) < 0)
		{
			error_handling("read result error");
		}


		Search_Record record_to_receive[num_result];

		int read_cnt;
		printf("result====================\n");
		for (int i = 0; i < num_result; i++)
		{
			int read_cnt = 0;
			int tmp_idx =0;
			read_cnt = read(sock, &record_to_receive[i],sizeof(Search_Record));
			if (read_cnt <= 0)
			{
				error_handling("read error");
			}
			//감지된 위치 전까지 그냥 출력.
			//감지된 위치부터 input len만큼 색칠
			//나머지는 원래 색대로 

			// printf("Received: ");
			// char *tmp_finder=record_to_receive[i].word;

			// while(tmp_finder!=NULL){

			// 	if(tmp_finder == record_to_receive[i].find){

			// 		for(int i=0; i<input_len;i++){
			// 		printf("%s %s %s",COLOR_RED,tmp_finder,COLOR_RESET);
			// 		tmp_finder++;
			// 		tmp_idx++;
			// 		}

					
			// 	}

			// 	printf("%c",tmp_finder[tmp_idx]);
			// 	tmp_finder++;
			// 	tmp_idx++;


			// }
			// printf("/n");

			printf("Received: %s  : \n", record_to_receive[i].word);
		}

		int result = 1;
		int tmp = write(sock, &result, sizeof(int));
		if (tmp<0)
		{
			error_handling("result error");
		}
	}
	close(sock);
	return 0;
}

void set_terminal_mode(int enable)
{
	struct termios t;

	tcgetattr(STDIN_FILENO, &t);

	if (enable)
	{

		t.c_lflag &= ~(ICANON | ECHO);
		t.c_cc[VMIN] = 1;
	}
	else
	{
		t.c_lflag |= ICANON | ECHO;
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
