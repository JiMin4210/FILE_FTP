#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BUFFSIZE 512 // 버퍼 사이즈 설정
#define SERV_IP "220.149.128.100"
#define SERV_PORT 4460

#define MY_IP "220.149.128.101"
#define MY_PORT "4461"

void login_check(int sockfd);
void creat_filelist();
void FTP_Transfer(char *file_name, int sockfd);
void FTP_Receiver(char *file_name, int sockfd);

char id[20];
char pw[20];

char file_list[20][20]; // 파일 리스트를 저장
int file_count; // 현재 몇개의 파일이 존재 하는지?

int main(void)
{
	system("clear"); // 켜질 때 화면 깨끗하게
	int sockfd;
	struct sockaddr_in dest_addr;

	char buf[512];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Client-socket() error 1o1!");
		exit(1);
	}
	else printf("Client-socket() sockfd is OK...\n");

	dest_addr.sin_family = AF_INET;

	dest_addr.sin_port = htons(SERV_PORT);
	dest_addr.sin_addr.s_addr = inet_addr(SERV_IP);

	memset(&(dest_addr.sin_zero), 0, 8);


	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == -1) {
		perror("Client-connect() error 1o1!");
		exit(1);
	}
	else printf("Client-connect() is OK...\n");

	read(sockfd, buf, sizeof(buf)); // 연결 성공 메세지
	printf("%s", buf);

	login_check(sockfd);
	

	creat_filelist(sockfd); // 현재 파일리스트 생성
	send(sockfd, "1", 2, 0); // 처음 시작하자마자 파일 리스트 한번 보냄
	FTP_Transfer("file_list.txt", sockfd); // 리스트 파일 전달
	for (int i = 0; i < 5000; i++) // 너무 빨리보내면 server쪽에서 데이터를 받지 못하는 경우가 발생했다
		for (int i = 0; i < 5000; i++); // 실험 결과 4000도 안되고 5000부터 전송이 잘 되었다..
	send(sockfd, MY_IP, strlen(MY_IP) + 1, 0); // ip주소 전달
	send(sockfd, MY_PORT, strlen(MY_PORT) + 1, 0); // 포트번호 전달

	while (1) // 로그인 성공 시 FTP 시작
	{
		printf("===============Select Menu================\n");
		printf("1.file update   2.file add   3.file select\n");
		printf("==========================================\n");
		printf("select number : ");
		scanf("%s", buf); // 원하는 메뉴 선택
		printf("\n");
		system("clear");
		printf("\n\n\n\n\n");
		send(sockfd, buf, sizeof(buf), 0); // 번호 전달

		if (!strcmp(buf, "1")) // 현재 파일 리스트 최신화 + 파일 리스트 내용 전송
		{
			creat_filelist(sockfd);
			FTP_Transfer("file_list.txt", sockfd); // 리스트 파일 전달
			for (int i = 0; i < 5000; i++) // 너무 빨리보내면 server쪽에서 데이터를 받지 못하는 경우가 발생했다
				for (int i = 0; i < 5000; i++); // 실험 결과 4000도 안되고 5000부터 전송이 잘 되었다..
			send(sockfd, MY_IP, strlen(MY_IP) + 1, 0); // ip주소 전달
			send(sockfd, MY_PORT, strlen(MY_PORT) + 1, 0); // 포트번호 전달
		}

		if (!strcmp(buf, "2")) // 파일 추가시에도 파일 리스트 최신화 + 파일리스트 내용 자동 전송 
		{
			creat_filelist(sockfd);
		}

		if (!strcmp(buf, "3")) // 파일 리스트를 전송받고 선택하는 과정
		{
			char file_num[10];
			FTP_Receiver("server_filelist.txt", sockfd); // 서버 리스트 파일 수신후 저장
			
			int line = 0; // 몇번째 라인인지?

			FILE *fp; // 리스트 파일에서 ip, port번호 찾는 과정
			fp = fopen("server_filelist.txt", "r");
			fread(buf, 1, BUFFSIZE, fp); // 파일 읽기
			fseek(fp, 0, SEEK_SET); // fread 했으니 다음 fgets를 위해 포인터를 초기화 시켜놓는게 필수이다.

			printf("==============SERVER FILE LIST===============\n");
			printf("%s", buf);
			printf("=============================================\n");
			do {
				printf("select file number(0~99) : ");
				scanf("%s", file_num);
			} while (!(atoi(file_num) > 0 || atoi(file_num) < 99)); // 숫자가 아닌 다른걸 입력하면 다시 입력
			
			char file_info[BUFFSIZE] = { 0, }; // 초기화 필수 NULL이라면 원하는 번호를 찾지 못한 것이다.
			while (fgets(buf, sizeof(buf), fp) != NULL) // 원하는 라인을 찾는 과정이다.
			{
				if (++line == atoi(file_num))
				{
					strcpy(file_info, buf);
					printf("\nwant = %s\n", file_info);
					break;
				}
			}

			if (file_info[0] != NULL)
			{

			}
			else // file_info가 비어있다면
			{
				printf("\nsearch error\n");
			}

		}
	}
	system("clear"); // 꺼질 때 화면 깨끗하게
	close(sockfd);
	return 0;
}

void login_check(int sockfd)
{
	char buf[BUFFSIZE];
	while (1)
	{
		printf("ID: ");
		scanf("%s", id); // id 입력 받기
		send(sockfd, id, strlen(id) + 1, 0);

		printf("PW: ");
		scanf("%s", pw); // pw 입력 받기
		send(sockfd, pw, strlen(pw) + 1, 0);

		recv(sockfd, buf, sizeof(buf),0); // 성공여부 수신
		printf("%s\n", buf);

		if (strstr(buf, "fail"))
			printf("===============Log in again===============\n");
		else
		{
			printf("****************FTP Start*****************\n\n");
			return;
		}
	}
}

void creat_filelist()  // 파일리스트 만들기 + 정보 전달
{
	DIR *dp; // 폴더 관련
	struct dirent *dir; 
	FILE *fp; // 파일 관련
	fp = fopen("file_list.txt", "w");
	
	char buf[BUFFSIZE];
	file_count = 0; // 카운트 초기화
	memset(file_list, 0, sizeof(file_list)); // 파일리스트 변수 초기화 - 파일리스트가 존재하지 않으면 전송도 X하자

	if ((dp = opendir("./file")) == NULL) // .는 현재 경로를 의미
	{ 
		fprintf(stderr, "error\n");
		exit(-1);
	}

	printf("================FILE LIST=================\n");
	while ((dir = readdir(dp)) != NULL) 
	{
		if (!dir->d_ino || !strstr(dir->d_name, ".txt")) continue; // 오직 txt 파일만 본다
		strcpy(file_list[file_count++], dir->d_name);
		sprintf(buf, "%d. %s\n", file_count, dir->d_name); 
		printf("%s", buf);
		fputs(buf, fp); // 파일 리스트를 만들기 위함.
	}
	printf("==========================================\n\n");

	fclose(fp); // 파일 종료
	closedir(dp); // 폴더 종료
}

void FTP_Transfer(char *file_name, int sockfd)
{
	FILE *fp;
	char buf[BUFFSIZE] = { 0, }; // 메모리 초기화 필수
	fp = fopen(file_name, "r");
	fread(buf, 1, sizeof(buf), fp);
	send(sockfd, buf, sizeof(buf), 0);
	fclose(fp);
}

void FTP_Receiver(char *file_name, int sockfd)
{
	char buf[BUFFSIZE];
	recv(sockfd, buf, sizeof(buf), 0); // 서버 리스트 파일을 전송받음
	FILE *fp; // 파일 저장
	fp = fopen(file_name, "w");
	fwrite(buf, 1, strlen(buf), fp);
	fclose(fp);
}


