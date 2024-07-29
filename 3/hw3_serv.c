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
#define NAME_SIZE 20
#define MAX_CLNT 256
#define MAX_DIR_LEN 256
#define MAX_FILE_NUM 1024
#define MAX_CMD_LEN 3

typedef struct Client_info
{
	int sockfd;					// socket num (저장 필요)
	char dir[MAX_DIR_LEN];		// 현재 작업 위치 (저장 필요)
	char last_dir[MAX_DIR_LEN]; // 가장 최근 파일 이름 (cd ..했을 때 뺴기 위함) (저장 필요)

} Client_info;

typedef struct File_info
{
	char file_name[100];
	unsigned int size;
} File_info;

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
	pthread_t t_id;
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

					pthread_mutex_lock(&mutx);			// 전체 유저 수 +1 추가
					clnt_socks[clnt_cnt++] = clnt_sock; // ~
					pthread_mutex_unlock(&mutx);		// ~

					FD_SET(clnt_sock, &reads); // client socket의 id 값을 세팅한다.

					if (fd_max < clnt_sock)
						fd_max = clnt_sock; // 가장 큰 fd 값 업데이트 , 그 값까지는 돌아야 하니까
					printf("connected client: %d \n", clnt_sock);
					strcpy(client[clnt_sock].dir, "/home/s22000082/");
				}
				else // read message!
					 // = client에서 이벤트가 생겼다. ##를 해줘라고 옴
				{
					printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
					pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
					pthread_detach(t_id);
				}
			}
		}
	}
	close(serv_sock);
	return 0;
}

void change_dir(Client_info info, char param_in[MAX_DIR_LEN])
{ // 현재 client에 해당하는 구조체 가져오기

	// 1.어디로 갈건지
	// 2.그 나머지에 해당하는 문자열 -> dir open.
	// 3.이동했을 때 있는 정보들 담기
	// 4.해당 clnt의 구조체 업데이트
	// 5.읽은 값을 연결된 socket에 send

	int sock = info.sockfd;
	printf("%d th clnt\n", sock);

	File_info files_list[MAX_FILE_NUM];
	struct dirent *entry = NULL; // 현재 작업 주소에 있는 자료들

	int param_len = strlen(param_in);

	// 어디로 갈건지
	strncat(client[sock].dir, param_in, param_len);
		strcat(client[sock].dir, "/");

	printf("dir 주소는 %s",client[sock].dir);

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


	// 4.해당 clnt의 구조체 업데이트
	strcpy(client[sock].last_dir, param_in);

// 	// 5.읽은 값을 연결된 socket에 send
// 	write(sock, &num_file, sizeof(num_file));

// 	for (int i = 0; i < num_file; i++)
// 		write(sock, &files_list[i], sizeof(File_info));
}

void upload_file(Client_info info, char param_in[MAX_DIR_LEN])
{ // 현재 client에 해당하는 구조체 가져오기
	printf("upload \n");
	int sock = info.sockfd;

	// 1. 현재 dir에 받는 파일 이름으로 fopen
	FILE *fp;
	fp = fopen(param_in, "wb");
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

void download_file(Client_info info, char param_in[MAX_DIR_LEN])
{ // 현재 client에 해당하는 구조체 가져오기
	int sock = info.sockfd;
	printf("download \n");

	// 현재 dir에서 입력받은 파일을 연다.
	FILE *fp;
	FILE *fp_tmp;
	unsigned int size;
	char message[BUF_SIZE];

	fp = fopen(param_in, "rb");
	fp_tmp = fopen(param_in, "rb");
	fseek(fp_tmp, 0, SEEK_END);
	size = ftell(fp_tmp);
	fclose(fp_tmp);

	if (fp == NULL)
	{
		error_handling("Wrong file Entered");
	}

	write(sock, &size, sizeof(size));

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

	// server : write , client : read
}



void list_up(Client_info info){

	int sock = info.sockfd;
	File_info files_list[MAX_FILE_NUM];
	struct dirent *entry = NULL; // 현재 작업 주소에 있는 자료들


	// 어디로 갈건지

	printf("ls dir 주소는 %s\n",client[sock].dir);

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
	write(sock, &num_file, sizeof(num_file));

	for (int i = 0; i < num_file; i++)
		write(sock, &files_list[i], sizeof(File_info));

}


void *handle_clnt(void *arg)
{
	int clnt_sock = *((int *)arg);
	int str_len = 0, i;
	Command com;

	while ((str_len = read(clnt_sock, &com, sizeof(Command))) > 0)
	{

		if (str_len == -1)
		{
			error_handling("read() error");
			break;
		}

		if (str_len == 0) // close request! = 읽었는데 내용이 없음 = 파일의 끝임
		{
			FD_CLR(i, &reads); // list 에서 해당 연결에 대한 이벤트 끝
			close(i);
			printf("closed client: %d \n", i);
		}
		else
		{ // 여기가 작업 처리하는 곳임!!
		  // 아래에서 요청한 작업 해서 보내준다. i번째 socket 번호로 다시 보낸다.

			// 그 내용 구조체에 저장하기

			printf("Received - command: %s, param: %s\n", com.command, com.param);

			// 파일 이동
			if (strcmp(com.command, "cd") == 0)
			{

				change_dir(client[clnt_sock], com.param);
			}

			// 파일 업로드
			else if (strcmp(com.command, "u") == 0)
			{

				upload_file(client[clnt_sock], com.param);
			}

			// 파일 다운로드
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
				// 다시 입력하라고 보내기
			}
			memset(&com, 0, sizeof(com));
			str_len = 0;
		}
		pthread_mutex_lock(&mutx);
		for (i = 0; i < clnt_cnt; i++) // remove disconnected client
		{
			if (clnt_sock == clnt_socks[i])
			{
				while (i++ < clnt_cnt - 1)
					clnt_socks[i] = clnt_socks[i + 1];
				break;
			}
		}
	}
	clnt_cnt--;
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