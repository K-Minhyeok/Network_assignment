#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>

#define BUF_SIZE 100
#define MAX_CMD_LEN 3

#define COLOR_RED "\033[38;2;255;0;0m"
#define COLOR_RESET "\033[0m"

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
void show_with_color(char word_to_color[], char input[]);
char *get_input();

int main(int argc, char *argv[])
{
	int sock;
	char line[BUF_SIZE];
	int str_len, recv_len, recv_cnt;
	struct sockaddr_in serv_adr;
	int input_len;
	int idx = 0;
	int read_cnt;
	int num_result;
	int result = 1;
	int tmp;
	int ch;

	struct termios new_termio, old_termio; // Termios 설정
	tcgetattr(STDIN_FILENO, &old_termio);  //
	new_termio = old_termio;			   //

	new_termio.c_lflag &= ~(ICANON | ECHO);		   //
	new_termio.c_cc[VMIN] = 1;					   //
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termio); //
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
	Input input;
	input.message[0] = '\0';
	printf("\033[s");

	while (1)
	{

		printf("Enter a word (ESC to quit): ");

		// 입력 받기
		// 문자를 입력받았다면 문자열에 추가
		// escape 코드를 쳤으면 break ( esc로 )
		// backspace 를 쳤다면 현재 인덱스를 /0 로 바꾼다.

		ch = getchar();

		if (ch == 27)
		{
			printf("bye");
			exit(1);
		}
		else if (ch == 127)
		{
			if (idx > 0)
			{
				input.message[idx] = '\0';
				idx--;
				printf("\b");
			}
		}
		else
		{
			input.message[idx] = ch;
			idx++;
		}
		// 입력 받은 문자를 서버에 전송
		write(sock, &input, sizeof(input));

		// 입력한 문자에 대한 결과 갯수(sort 되어있다.)
		if (read(sock, &num_result, sizeof(num_result)) < 0)
		{
			error_handling("read result error");
		}

		Search_Record record_to_receive[num_result];
		printf("\n");
		// 입력한 문자에 대한 결과를 받아온다.
		for (int i = 0; i < num_result; i++)
		{
			read_cnt = 0;
			read_cnt = read(sock, &record_to_receive[i], sizeof(Search_Record));
			if (read_cnt <= 0)
			{
				error_handling("read error");
			}

			show_with_color(record_to_receive[i].word, input.message);
		}

		printf("\033[J\033[K\033[u");
	}
	close(sock);
	tcsetattr(STDIN_FILENO, TCSANOW, &old_termio);

	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void show_with_color(char word_to_color[], char input[])
{
	// 검색어 단어가 시작하는 곳
	char *finder = strstr(word_to_color, input);

	// 검색어와, 전체 기록에 대한 길이
	int word_len = strlen(word_to_color);
	int input_len = strlen(input);

	// 색을 칠하기 시작해야하는 곳의 위치
	int start_pos = finder - word_to_color;

	// 시작 ~ 감지된 위치 전까지 그냥 출력.
	for (int i = 0; i < start_pos; i++)
	{
		printf("%c", word_to_color[i]);
	}

	// 감지된 위치부터 input len만큼 색칠
	for (int i = start_pos; i < start_pos + input_len; i++)
	{
		printf("%s%c%s", COLOR_RED, word_to_color[i], COLOR_RESET);
	}

	// 나머지는 원래 색대로
	for (int i = start_pos + input_len; i < word_len; i++)
	{
		printf("%c", word_to_color[i]);
	}

	printf("\n");
}
