#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <dirent.h>

#define BUF_SIZE 30
#define PATH_MAX 4096

void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sd, clnt_sd;   //socket descriptor
    char path[PATH_MAX];    //현재 작업하는 Path 를 절대경로로 받는다.
    FILE *fp;               //file 포인터
    DIR *dp;                //dir 포인터  
    char buf[BUF_SIZE];     //받은 문자 저장하는 곳
    int read_cnt;           //글자 수
    struct dirent* entry = NULL;    //현재 작업 주소에 있는 자료들
    struct sockaddr_in serv_adr, clnt_adr;  // socket 주소 저장하는 구조체
    socklen_t clnt_adr_sz;      //client 구조체에 있는 size?
    

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
 
    char *current_dir = getcwd(path,PATH_MAX);   //현재 작업 공간을 받는다. + 에러 처리
    dp = opendir(current_dir);
    if( dp == NULL){
        printf("Directory Unavailable");
        exit(1);
    }


    serv_sd = socket(PF_INET, SOCK_STREAM, 0);  //IPv4 UDP default 를 이용하는 소켓을 만들고 해당 소켓의 fd를 return 받는다.


    memset(&serv_adr, 0, sizeof(serv_adr));

    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    bind(serv_sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
    listen(serv_sd, 5);

    clnt_adr_sz = sizeof(clnt_adr); //accept 요청에 필요한 정보 초기화
    clnt_sd = accept(serv_sd, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

    int i=0;
    
    char *file_name[100];

    while(1){

//지금 여기에서 파일 이름이 제대로 parsing 안 돼서 그런 거 같음    

    while(((entry = readdir(dp))) != NULL){   //디렉토리 안에 Entry가 있다면 , file인지도 판단해야 할 듯
        printf("%s\n",entry->d_name);
        write(clnt_sd, entry->d_name, strlen(entry->d_name));              
        file_name[i] = entry->d_name;  //파일 이름을 array 에 넣는다.
        i++;
    }

        shutdown(clnt_sd, SHUT_WR);
        read(clnt_sd, buf, BUF_SIZE);  //입력받으면 [#번째] 를 fd로 지정한다.
      
        int idx = atoi(buf);
        printf("num : %d",idx);
        fp = fopen(file_name[idx], "rb");              //현재 이 파일을 열고, 이 파일을 여는 pointer 값을 return한다.

        if(fp== NULL){
            error_handling("file open error");
        }



        
    while (1)
    {
        read_cnt = fread((void *)buf, 1, BUF_SIZE, fp); //버퍼에 fp가 가리키는 걸 읽어서 담는다. (저장할 곳 , 읽는 문자열 크기 , 반복 횟수 , 읽을 파일 포인터 )
        if (read_cnt < BUF_SIZE)                        // 버퍼 사이즈보다 읽은 게 적으면 
        {
            write(clnt_sd, buf, read_cnt);              //그냥 버퍼에 있는 거 쓰면 됨
            break;
        }
        write(clnt_sd, buf, BUF_SIZE);                  //그게 아니면 버퍼 크기만큼만 적는다.
    }
    

    printf("Message from client: %s \n", buf);

    
    }

    fclose(fp);
    close(clnt_sd);
    close(serv_sd);
    return 0;
    }
    

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n', stderr);
    exit(1);
}