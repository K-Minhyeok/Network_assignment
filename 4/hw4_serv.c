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

#define BUF_SIZE 100
#define WORD_CNT 15
#define MAX_CLNT 256

typedef struct Search_Record
{
	char word[BUF_SIZE];
	int num_searched;
	char *find;
} Search_Record;


typedef struct Input
{
	char input_word[BUF_SIZE];
} Input;


void *handle_clnt(void *arg);
void error_handling(char *msg);
void parse_textfile(char file_path[], Search_Record *record);
int compare(const void *a, const void *b);


int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;
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
		printf("======select======\n");
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
					FD_SET(clnt_sock, &reads); // client socket의 id 값을 세팅한다.

					if (fd_max < clnt_sock)
						fd_max = clnt_sock; // 가장 큰 fd 값 업데이트 , 그 값까지는 돌아야 하니까

					pthread_mutex_lock(&mutx);			// 전체 유저 수 +1 추가
					clnt_socks[clnt_cnt++] = clnt_sock; // ~
					pthread_mutex_unlock(&mutx);		// ~
				}
				else // read message!
					 // = client에서 이벤트가 생겼다. ##를 해줘라고 옴
				{
					pthread_t t_id;
					int event_sock = i;
					printf(" %d\n", event_sock);

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

void *handle_clnt(void *arg)
{
	int clnt_sock = *((int *)arg);
	int str_len, i;
	Search_Record search_record[WORD_CNT];

	printf("in\n");

	//  여기부터 While
	//  #############  Recv (Input 구조체로 들어온다) ################
	//  들어오는 입력 단어에 대해서 strstr로 찾아준다.
	//  해당하는 노드들의 갯수를 꺼내서 그 횟수만큼 다시 담는다.

	Input input;

	memset(&input, 0, sizeof(input));
	while ((str_len = read(clnt_sock, &input, sizeof(input))) > 0)
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


		// 파일 parsing 함수 (입력 : 텍스트 파일이름 string)
		// 결과는 구조체에 담긴다.
		parse_textfile("search.txt", search_record);

		for (int i = 0; i < WORD_CNT; i++)
		{
			printf("%s , %d \n", search_record[i].word, search_record[i].num_searched);
		}
		int num_match = 0;						// 문자열에 해당하는 노드 갯수
		Search_Record record_to_sort[WORD_CNT]; // 일치하는 갯수를 찾고 보내는 구조체, 결국엔 num_match만큼만 넘어간다.
		int index = 0;

		// 여기에서 input.word랑 분류한 record들이랑 하나하나 비교해서 있으면 저장하고 num_match ++

		for (int i = 0; i < WORD_CNT; i++)
		{
			char *tmp_for_str;
			if ((tmp_for_str = strstr(search_record[i].word, input.input_word)) != NULL)
			{
				search_record[i].find = tmp_for_str;
				memcpy(&record_to_sort[index], &search_record[i], sizeof(Search_Record));
				num_match++;
				index++;
			}
		}

		if (write(clnt_sock, &num_match, sizeof(num_match)) < 0)
			error_handling("write record error");

		// 찾은 결과에 대해 sort
		qsort(record_to_sort, num_match, sizeof(Search_Record), compare);
	


		// #############  Send () (Search_record 단위로)
		// send
		for (int i = 0; i < num_match; i++)
		{
			int write_cnt = 0;

			write_cnt = write(clnt_sock, &record_to_sort[i], sizeof(Search_Record));
			if (write_cnt <= 0)
			{
				error_handling("write error");
			}

			printf("Sent: %s , %d , size = ,pointer : %s\n", record_to_sort[i].word, record_to_sort[i].num_searched, record_to_sort[i].find);
		}
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

}
void parse_textfile(char file_path[], Search_Record *record)
{
	// 전역변수인 구조체에 정보를 담는다.
	//  텍스트 파일 [이름] 연다
	//  읽으면서 parsing
	//  전역 변수인 search_record에 접근해서 텍스트를 각 노드별로 넣는다.

	// 세부 구현:
	// 받은 파일 주소 파일을 연다.
	
	printf("hit1\n");
	FILE *fp;
	fp = fopen(file_path, "rb"); // 현재 이 파일을 열고, 이 파일을 여는 pointer 값을 return한다.
	printf("%s 입니다.", file_path);

	if (fp == NULL)
	{
		error_handling("Wrong file Entered");
	}
	printf("hit middle\n");

	// 반복 {
	// 한 줄을 읽어온다.
	//  ':' 이전은 search_record[i].word,그 이후는 search_record[i].count로 넣는다.
	char line[BUF_SIZE];
	char tmp_for_number[BUF_SIZE];
	int idx = 0;
	while (fgets(line, sizeof(line), fp) != NULL && idx < WORD_CNT)
	{
		char *cut_pointer = strchr(line, ':');
		
		// word 파싱
		int word_length = cut_pointer - line;
		if (word_length >= BUF_SIZE)
		{
			word_length = BUF_SIZE - 1;
		}
		strncpy(record[idx].word, line, word_length);
		record[idx].word[word_length] = '\0';

		// count 파싱
		record[idx].num_searched = atoi(cut_pointer + 1);

		idx++;
	}
	printf("hit4\n");
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

int compare(const void *a, const void *b) // 내림차순 정렬
{
	Search_Record *pa = (Search_Record *)a;
	Search_Record *pb = (Search_Record *)b;

	if (pa->num_searched < pb->num_searched)
		return 1;
	else if (pa->num_searched > pb->num_searched)
		return -1;
	else
		return 0;
}

// 파일 parsing 함수 (입력 : 텍스트 파일이름 string)
// 텍스트 파일 [이름] 연다
// 읽으면서 parsing
// 전역 변수인 search_record에 접근해서 텍스트를 각 노드별로 넣는다.

// parsing된 구조체  WORD_CNT 개 가지고 있기.

// #############  Recv (Content 구조체로 들어온다) ################
// 들어오는 입력 단어에 대해서 strstr(길이로 자를 수 있으면 잘라서 주기)로
// 해당하는 노드들의 갯수를 꺼내서 그 횟수만큼 다시 담는다.

// #############  Send ()
// 찾은 결과에 대해 sort후 send