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
#define BACKLOG 10

#define FILE_ERROR "file does not exist\n\n"

void login_check(int sockfd);
void make_server();
int connect_server();
void creat_filelist();
void send_filelist(int sockfd, int flag); // 파일 리스트 만들고 서버에 파일 리스트 전송
int FTP_Transfer(char *file_name, int sockfd);
int FTP_Receiver(char *file_name, int sockfd);

char id[20];
char pw[20];
char file_list[20][30]; // 파일 리스트를 저장 - 최대 파일 개수 = 20

char target_filename[20]; // 원하는 파일 이름
char target_id[20]; // 원하는 파일 이름
char target_ip[16]; // 파일 전송받을 ip 주소
int target_port; // 파일 전송받을 port 주소

int main(void)
{
	system("clear"); // 켜질 때 화면 깨끗하게
	int sockfd;
	struct sockaddr_in dest_addr;

	pid_t pid; // FTP송신용 서버를 만드는 용도

	char buf[BUFFSIZE] = { 0, };

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

	pid = fork();

	if(pid == 0) // 로그인 한 시점부터 FTP용 서버를 생성해준다.
	{
		make_server();
	}	

	send_filelist(sockfd,1); // 파일리스트 만들고 서버에 전송

	while (1) // 로그인 성공 시 FTP 시작
	{
		printf("===============Select Menu===================\n");
		printf("  1.file update            2.file receive\n");
		printf("  3.file add               4.file remove\n");
		printf("  5.file view              6.???????????\n");
		printf("=============================================\n");
		printf("select menu number : ");
		scanf("%s", buf); // 원하는 메뉴 선택
		printf("\n");
		system("clear");
		printf("\n\n\n");
		send(sockfd, buf, sizeof(buf), 0); // 번호 전달

		if (!strcmp(buf, "1")) // 현재 파일 리스트 최신화 + 파일 리스트 내용 전송
		{
			send_filelist(sockfd,0); // 수동적인 파일리스트 업데이트 - 프로그램으로 파일 추가한게 아닌 직접 파일경로로가서 생성한 경우 이용
		}

		if (!strcmp(buf, "2")) // 파일 리스트를 전송받고 선택하는 과정
		{
			int file_num;
			FTP_Receiver("server_filelist.txt", sockfd); // 서버 리스트 파일 수신후 저장

			FILE *fp; // 리스트 파일에서 ip, port번호 찾는 과정
			fp = fopen("./file/server_filelist.txt", "r");
			memset(buf, 0, sizeof(buf)); // fread 전엔 초기화가 필수
			fread(buf, 1, BUFFSIZE, fp); // 파일 읽기
			printf("==============SERVER FILE LIST===============\n");
			printf("%s", buf);
			printf("=============================================\n");
			
			printf("select file number(1~99) : ");
			scanf("%d", &file_num);

			char file_info[BUFFSIZE] = { 0, }; // 초기화 필수 NULL이라면 원하는 번호를 찾지 못한 것이다.
			fseek(fp, 0, SEEK_SET); // fread 했으니 다음 fgets를 위해 포인터를 초기화 시켜놓는게 필수이다.
			while (fgets(buf, sizeof(buf), fp) != NULL) // 원하는 라인을 찾는 과정이다.
			{
				if (!--file_num) // 원하는 라인까지 가는 방법
				{
					strcpy(file_info, buf);
					break;
				}
			}

			if (file_info[0]) // file_info[0]이 NULL값이 아니라면 즉 값이 존재 한다면
			{
				char word[4][50] = { 0, }; // file_name, ip, port 구분을 위한 버퍼
				char *point = &file_info[3]; // 무조건 index=3부터 file_name 시작
				int topic_count = 0;
				int count = 0;
				while (*point) // 단어들을 구분하는 알고리즘 (포인터가 가리키는 값이 NULL이 아닐 때)
				{
					word[topic_count][count++] = *point;
					point++;
					if (*point == '\t')
					{
						point++;
						topic_count++;
						count = 0;
					}
				}
				strcpy(target_filename, word[0]); // 대상의 파일 이름
				strcpy(target_id, word[1]); // 대상의 ip주소
				strcpy(target_ip, word[2]); // 대상의 ip주소
				target_port = atoi(word[3]); // 대상의 port번호 복사

				if (!strcmp(target_id, id)) // 자신의 파일인지 검사
				{
					printf("\n================error msg====================\n");
					printf("         %s is your file\n", target_filename);
					printf("=============================================\n\n");
					continue; // 자신의 파일이라면 메뉴로 돌아감
				}

				printf("\n================target_info==================\n");
				printf("target_filename = %s\n", target_filename);
				printf("target_id       = %s\n", target_id);
				printf("target_ip       = %s\n", target_ip);
				printf("target_port     = %d\n", target_port);
				printf("=============================================\n\n");
				printf("Would you like to have the file transferred?(yes/no) : ");
				scanf("%s", buf); // 파일을 전송 받을지 말지 결정하는 scanf
				system("clear");

				if (!strcmp(buf, "yes")) // 파일 전송 최종 승인 시
				{
					int ftp_sock = connect_server(); // 서버와의 연결
					if (ftp_sock)
					{
						send(ftp_sock, target_filename, sizeof(target_filename), 0); // 원하는 파일 이름 전송
						if (FTP_Receiver(target_filename, ftp_sock)) // 파일 전송받고 에러가 없다면
						{
							printf("File transfer successful!!\n\n"); // 성공 메세지 출력

							send_filelist(sockfd, 1); // 파일 리스트 서버로 최신화 시켜주고 출력해줌

							sprintf(buf, "./file/%s", target_filename); // 파일 열고 내용 출력
							fp = fopen(buf, "r");
							memset(buf, 0, BUFFSIZE); // fread는 값을 그냥 덮어 씌우는거라 오류 발생을 방지하기위해 메모리 초기화해준다.
							fread(buf, 1, BUFFSIZE, fp);
							fclose(fp);
							printf("=================File View===================\n");
							printf("file name : %s\n", target_filename);
							printf("=============================================\n");
							printf("%s", buf);
							printf("=============================================\n\n");
						}
						else
							printf("%s", FILE_ERROR);
						close(ftp_sock);
					}
				}
				else // 파일전송을 하지 않겠다.
				{
					printf("You did not enter yes.\n\n");
				}

			}
			else // file_info가 비어있다면
			{
				printf("\nsearch error\n\n");
			}

		}
		if (!strcmp(buf, "3")) // 파일 추가
		{
			char path[512] = "./file/"; 
			printf("=============================================\n");
			printf("file name : ");
			scanf("%s", buf);
			strcat(path, buf);
			printf("* Enter 'exit' to exit the input mode.\n");
			printf("=============================================\n");
			getchar(); // 입력버퍼 한번 비워줘야한다.
			FILE *fp;
			fp = fopen(path, "w"); // 파일 만드는 도중에 ctrl+c 종료해도 파일 만들어짐
			while (1)
			{
				printf("input mode : ");
				gets(buf);
				if (!strcmp(buf, "exit")) break;
				fputs(buf, fp);
				fputs("\n", fp); // 개행문자 넣어준다.
			}
			printf("=============================================\n\n");
			fclose(fp);
			printf("File creation successful!!\n\n");

			send_filelist(sockfd,1); // 자동 파일 업데이트 + 리스트 출력	
		}
		if (!strcmp(buf, "4")) // 파일 제거
		{
			int file_num;
			system("clear");
			creat_filelist();
			printf("select file num for remove(1~99) : "); 
			scanf("%d", &file_num);
			file_num--;
			if (file_num >= 0 && file_num < 20)
			{
				char path[512] = "./file/";
				strcat(path, file_list[file_num]);
				if(remove(path)) // 파일 삭제
					printf("%s",FILE_ERROR); // 파일 삭제 실패 시
				else
					printf("\n%s File removal successful!!\n\n", file_list[file_num]); // 파일 삭제 성공 시

			}
			else
				printf("\n%s", FILE_ERROR);

			send_filelist(sockfd,1); // 자동 파일 업데이트 + 리스트 출력
		}
		if (!strcmp(buf, "5")) // 파일 내용 확인
		{
			int file_num;
			system("clear");
			creat_filelist();
			printf("select file num for view(1~99) : ");
			scanf("%d", &file_num);
			file_num--;
			if ((file_num >= 0 && file_num < 20) && file_list[file_num][0])
			{
				char path[512] = "./file/";
				strcat(path, file_list[file_num]);
				FILE *fp;
				fp = fopen(path, "r");
				if (fp)
				{
					memset(buf, 0, sizeof(buf)); //fread 전 메모리 초기화 필수
					fread(buf, 1, BUFFSIZE, fp);
					printf("\n=================File View===================\n");
					printf("file name : %s\n", file_list[file_num]);
					printf("=============================================\n");
					printf("%s", buf);
					printf("=============================================\n\n");
					fclose(fp);
				}
				else
					printf("\n%s", FILE_ERROR);
			}
			else
				printf("\n%s", FILE_ERROR);	
		}
		if (!strcmp(buf, "6")) // 깜짝 그림 출력
		{
			system("clear");
			FILE *fp;
			fp = fopen("art.txt", "r");
			if (fp)
			{
				printf("\n");
				while (fgets(buf,BUFFSIZE, fp) != NULL)
				{
					printf("   %s", buf);
				}
				printf("\n");
				fclose(fp);
			}
			else
			{
				printf("atr file does not exist\n\n");
			}
			
		}
	}
	close(sockfd);
	return 0;
}

void login_check(int sockfd)
{
	char buf[BUFFSIZE] = { 0, };
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
			printf("===============Log in again==================\n");
		else
		{
			printf("****************FTP Start********************\n\n");
			return;
		}
	}
}

void creat_filelist()  // 파일리스트 만들기
{
	DIR *dp; // 폴더 관련
	struct dirent *dir; 
	FILE *fp; // 파일 관련
	fp = fopen("./file/file_list.txt", "w");
	
	char buf[BUFFSIZE] = { 0, };
	int file_count = 0; // 카운트 초기화 - 파일카운트 = 0이면 전송도 X하자

	if ((dp = opendir("./file")) == NULL) // .는 현재 경로를 의미
	{ 
		fprintf(stderr, "error : A file folder is required, plz make 'file' folder.\n");
		exit(-1);
	}

	memset(file_list, 0, sizeof(file_list)); // 파일 리스트 변수의 초기화 필수
	printf("================FILE LIST====================\n");
	printf("* Read only .txt file\n");
	printf("=============================================\n");
	while ((dir = readdir(dp)) != NULL) 
	{
		if (!dir->d_ino || !strstr(dir->d_name, ".txt") || !strcmp(dir->d_name, "file_list.txt") || !strcmp(dir->d_name, "server_filelist.txt")) continue; // 오직 txt 파일만 본다 (제외할 파일 선택)
		strcpy(file_list[file_count++], dir->d_name);
		sprintf(buf, "%d. %s\n", file_count, dir->d_name); 
		printf("%s", buf);
		fputs(buf, fp); // 파일 리스트를 만들기 위함.
	}
	printf("=============================================\n\n");

	fclose(fp); // 파일 종료
	closedir(dp); // 폴더 종료
}

void send_filelist(int sockfd, int flag) // 파일리스트 전달
{
	if(flag)
		send(sockfd, "1", 2, 0);
	
	creat_filelist(); // 현재 파일리스트 생성
	FTP_Transfer("file_list.txt", sockfd); // 리스트 파일 전달
	for (int i = 0; i < 5000; i++) // 너무 빨리보내면 server쪽에서 데이터를 받지 못하는 경우가 발생했다
		for (int i = 0; i < 5000; i++); // 실험 결과 4000도 안되고 5000부터 전송이 잘 되었다..
	send(sockfd, MY_IP, strlen(MY_IP) + 1, 0); // ip주소 전달
	send(sockfd, MY_PORT, strlen(MY_PORT) + 1, 0); // 포트번호 전달
}

int FTP_Transfer(char *file_name, int sockfd)
{
	char buf[BUFFSIZE] = { 0, }; // 메모리 초기화 필수

	char path[512] = "./file/";
	strcat(path, file_name); // 원하는 파일 경로를 설정해준다.

	FILE *fp;
	if (fp = fopen(path, "r"))
	{
		fread(buf, 1, sizeof(buf), fp);
		send(sockfd, buf, sizeof(buf), 0);
		fclose(fp);
		return 1;
	}
	else
		return 0;
}

int FTP_Receiver(char *file_name, int sockfd) // 파일을 자동으로 만들어주기에 return으로 에러체크안해도됨
{
	char buf[BUFFSIZE] = { 0, }; // 초기화 안해주면 이상한값 들어있었음. recv가 메모리 초기화 후 들어가는게 아닌것을 확인함
	char path[512] = "./file/";
	strcat(path, file_name); // 원하는 파일 경로를 설정해준다.
	if(!recv(sockfd, buf, sizeof(buf), 0)) return 0; // 서버 리스트 파일을 전송받음 연결 끊길 시 0값 리턴
	if (strcmp(buf, FILE_ERROR)) // 파일 에러가 아니라면
	{
		FILE *fp; // 파일 저장
		fp = fopen(path, "w");
		fwrite(buf, 1, strlen(buf), fp); // strlen으로 안해준다면 뭔가 이상하다 - 공백으로 공간이 채워져서 파일 포인터가 멀리 이동해버림 - 이후 a로 열어도 공백 이후에 써지는거 마찬가지 (이거 적기)
		fclose(fp);
		return 1;
	}
	return 0;
}

void make_server() // 서버쪽은 fork로 해결 - 딱히 스레드를 쓸 이유는 없음
{
	int sockfd, new_fd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	unsigned int sin_size;
	char buf[BUFFSIZE] = { 0, };
	int val = 1;

	pid_t pid;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Server-socket() error 1o1!");
		exit(1);
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(atoi(MY_PORT));
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), 0, 8);
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(val)) < 0) {
		perror("setsockopt");
		close(sockfd);
		return;
	}

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		perror("Server-bind() error 1o1!");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen() error 1o1!");
		exit(1);
	}
	else printf("Server creation success!!\n\n");

	while (1) {
		sin_size = sizeof(struct sockaddr_in);
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);	
		
		pid = fork();

		if (pid == 0) // 자식 프로세스 동작
		{
			recv(new_fd, buf, sizeof(buf), 0); // 파일 제목 받아오기		
			if (!FTP_Transfer(buf, new_fd))
			{
				send(sockfd, FILE_ERROR, sizeof(FILE_ERROR), 0); // 파일이 존재하지 않는다면 
			}
			close(new_fd);
			close(sockfd);
			exit(0); // 파일전송 완료 시 자식 프로세스를 닫아줘야함
		}
		else { // 부모프로세스 종작
			close(new_fd); // 부모 프로세스에는 연결된 새로운 소켓이 필요없기에 닫아준다.
		}
	}
	close(sockfd);
	exit(0);
}

int connect_server()
{
	int sockfd;
	struct sockaddr_in dest_addr;

	char buf[BUFFSIZE] = { 0, };

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("Client-socket() error 1o1!\n\n");
		return 0;
	}
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(target_port);
	dest_addr.sin_addr.s_addr = inet_addr(target_ip);

	memset(&(dest_addr.sin_zero), 0, 8);

	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == -1) {
		printf("Client-connect() error 1o1!\n\n");
		return 0;
	}
	else printf("Client-connect() is OK...\n");
	return sockfd;
}



