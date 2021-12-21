#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INIT_MSG "=============================================\nHello! I'm P2P File Sharing Server...\nPlease, LOG-IN!\n=============================================\n"
#define LOGIN_MSG "============================\nNum : %d User Information\nID: %s, PW: %s\n============================\n"
#define PW_ERROR "Log-in fail: Incorrect password...\n"
#define ID_ERROR "Log-in fail: Incorrect ID...\n"
#define ID_DUPERROR "Log-in fail: %s is already connecting to the server...\n"
#define SUCCESS_MSG "Log-in success!! [%s] *^^*\n\n"
#define NO_FILE "No files on all clients\n"

#define SERV_IP "220.149.128.100"
#define SERV_PORT 4460
#define BACKLOG 10
#define BUFFSIZE 512 // 버퍼 사이즈 설정
#define MAX_CLIENT 10 // 최대 클라이언트 개수
#define MAX_ID 4 // 생성할 수 있는 ID 개수

void *clnt_connection(void * sock);
void login_check(int num, int client_sock);
void filelist_remove();

typedef struct Client_Info { // 클라이언트들의 정보를 담기위한 구조체
	char id[20];
	char pw[20];
	char ip[16];
	char port[5];
	int sock;
}Client_Info;

char *user_id[4] = { "user1", "user2", "user3", "user4" };
char *user_pw[4] = { "passwd1", "passwd2", "passwd3", "passwd4" };
int client_count; // 현재 몇번째 클라이언트까지 접근했었는지
Client_Info client_info [MAX_CLIENT]; // 최대 클라이언트 개수만큼 정보 저장 가능.
int alive_clnt[MAX_CLIENT]; // 현재 연결되어있는 클라이언트를 의미한다. (1 = 연결 중 , 0 = 연결 안됨)

pthread_mutex_t mutex; // 스레드를 쓸것이기에 자원 동시 사용을 방지해줘야함.

int main(void)
{
	system("clear"); // 켜질 때 화면 깨끗하게
	int sockfd, new_fd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	unsigned int sin_size;
	int val = 1;
	
	pthread_t thread; // 스레드 사용을 위한 선언 - 스레드의 장점 단점 확실히 적기 (단점 = 오류시 찾기 힘듬)
	pthread_mutex_init(&mutex, NULL);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Server-socket() error 1o1!");
		exit(1);
	}
	else printf("Server-socket() sockfd is OK...\n");

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(SERV_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), 0, 8);
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(val)) < 0) {
		perror("setsockopt");
		close(sockfd);
		return -1;
	}
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		perror("Server-bind() error 1o1!");
		exit(1);
	}
	else printf("Server-bind() is OK...\n");
	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen() error 1o1!");
		exit(1);
	}
	else printf("listen() is OK...\n\n");

	filelist_remove(); // 파일 리스트를 받아서 하는것이기에 예전에 썻던 파일리스트가 존재 한다면 겹치기에
					   // 파일을 한번 비워줘야한다. (서버 파일을 바로끄게 되면 기존 파일이 안지워지고 꺼지기 때문에 그런것)
					   // 만약 클라이언트부터 다 꺼준다면 파일이 다 지워지기에 사실상 필요없지만 혹시모를 상황 대비

	while (1) {
		sin_size = sizeof(struct sockaddr_in);
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

		pthread_mutex_lock(&mutex); // client_count 변수의 자원 동시사용을 방지해줘야한다.(동시 접속시 에러 방지)
		client_info[client_count].sock = new_fd;
		pthread_create(&thread, NULL, clnt_connection, (void *)client_count++);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}

void *clnt_connection(void * count) {
	int num = (int)count; // 현재 몇번째 클라이언트인지
	int len; // 상대와의 연결이 끊길 때 에러를 방지
	int client_sock = client_info[num].sock;
	char buf[BUFFSIZE] = { 0, };

	printf("Num : %d User Accept() is OK...\n\n", num);
	send(client_sock, INIT_MSG, strlen(INIT_MSG) + 1, 0); // 성공메세지 전송

	alive_clnt[num] = 1; //클라이언트가 연결상태인것을 의미

	login_check(num, client_sock); // 로그인 함수
	while (1) {
		if(!recv(client_sock, buf, sizeof(buf),0)) break;
		//printf("number = %s\n", buf);
		
		if (!strcmp(buf, "1"))
		{
			
			recv(client_sock, buf, sizeof(buf),0); // file_list받음
			//printf("\n%s\n", buf); // 받은 파일리스트 출력
			FILE *fp; // 임시 파일용
			FILE *fp_all; // 파일리스트 작성용

			pthread_mutex_lock(&mutex); // file_list_test 파일의 자원 동시사용을 방지해줘야한다.(동시 접속시 에러 방지)
			fp = fopen("./list/file_list_test.txt", "w");
			fwrite(buf, 1, strlen(buf), fp); // 임시파일리스트 작성
			fclose(fp);

			recv(client_sock, buf, sizeof(buf), 0); // IP주소 받음 (파일 전송 완료 후)
			strcpy(client_info[num].ip, buf);
			recv(client_sock, buf, sizeof(buf), 0); // 포트 번호 받음
			strcpy(client_info[num].port, buf);

			fp = fopen("./list/file_list_test.txt", "r"); // 읽기모드로 임시파일 열기
			char file_name[50];
			sprintf(file_name, "./list/file_list%d.txt", num); // 전송받은 리스트 파일 이름 정함
			fp_all = fopen(file_name, "w"); // 쓰기모드로 파일리스트 열기
			while (fgets(buf, sizeof(buf), fp) != NULL)
			{
				char info[50];
				buf[strlen(buf)-1] = '\t'; // \n없애주기
				sprintf(info, "%s\t%s\t%s\n", client_info[num].id, client_info[num].ip, client_info[num].port); // ip, port주소 붙여주기
				strcat(buf, info);
				fputs(buf, fp_all);
			}

			fclose(fp); // 임시파일 닫고
			remove("./list/file_list_test.txt"); // 임시파일 삭제
			fclose(fp_all);			
			pthread_mutex_unlock(&mutex);
			printf("============================\n");
			printf("* %s updated file-list. \n", client_info[num].id);
			printf("============================\n\n");
		}

		if (!strcmp(buf, "2")) // 종합 파일리스트를 전송하고 파일 을 받을것인지 확인 -> 받으려는 순간 합쳐야 제일 최신화된 정보 얻기 가능(적기)
		{
			printf("============================\n");
			printf("* %s requested file-list. \n", client_info[num].id);
			printf("============================\n\n");
			char merge[512];
			int count = 0;
			memset(buf, 0, sizeof(buf)); // 버퍼 초기화
			FILE *fp;
			FILE *fp_all;

			pthread_mutex_lock(&mutex); // file_list 파일의 자원 동시사용을 방지해줘야한다.(동시 접속시 에러 방지) - 읽는 도중 수정되면 안됨

			for (int i = 0; i < MAX_CLIENT; i++) // 종료시 파일이 삭제되게 하기 - clear // for문 쓰자 - 살아있는 스레드만 - clear
			{
				if (alive_clnt[i]) // 클라이언트가 연결이 되어있다면 파일을 열고 병합해준다. - 스레드의 장점 = 자원공유가 여기서 유용함
				{
					char path[512] = { 0, };
					sprintf(path, "./list/file_list%d.txt", i);
					fp = fopen(path, "r");
					if (fp)
					{
						memset(merge, 0, sizeof(merge));
						fread(merge, 1, 512, fp);
						strcat(buf, merge);
						fclose(fp);
					}
				}
			} // 여기까지 파일 합치는 과정

			fp = fopen("./list/file_list_test.txt", "w"); // 임시파일 생성
			fwrite(buf, 1, strlen(buf), fp);
			fclose(fp);

			fp = fopen("./list/file_list_test.txt", "r");
			fp_all = fopen("./list/file_list.txt", "w");
			while (fgets(buf, sizeof(buf), fp) != NULL)
			{	
				if (++count > 9) // 번호가 10이 넘어 갈 때
				{
					buf[0] = count / 10 + '0'; // 아스키코드 전환 필수
					buf[1] = count % 10 + '0';
					buf[2] = '.';
				}
				else
					buf[0] = count + '0'; // 아스키코드 전환 필수
				
				fputs(buf, fp_all);
			}
			fclose(fp);
			fclose(fp_all); // 여기까지 파일 번호 바꾸는 과정
			remove("./list/file_list_test.txt"); // 임시파일 삭제
			pthread_mutex_unlock(&mutex); // file_list 수정이 끝날 때 뮤텍스 언락해주기

			fp_all = fopen("./list/file_list.txt", "r"); // 이쪽은 나중에 FTP 전용 함수로 바꿔주기 - 굳이 함수 안이용해도 될듯 서버라서 한번밖에 안씀
			memset(buf, 0, sizeof(buf)); // 초기화 해줘야 깔끔하게 값이 들어간다. - fread시 초기화의 중요성 적기 (실험결과 다 없애고하는게 아닌 덮어씌우는거라 위험)
			fread(buf, 1, BUFFSIZE, fp_all);
			fclose(fp_all); // 파일 내용 출력 과정
			//printf("\nfile = %s\n", buf); // 최종 서버 리스트 파일 출력
			//printf("buf len = %d\n", strlen(buf)); // 버퍼 크기 혹시몰라서 실험 - 문제없음
			//for (int i = 0; i < 5000; i++);
			if(!strlen(buf))
				send(client_sock, NO_FILE, strlen(NO_FILE) + 1, 0); // 모든 클라이언트에 파일이 없는경우 없다는 메세지 전송
			else
				send(client_sock, buf, strlen(buf), 0); // 서버 파일리스트 전송 -> 용량제한 있는듯 1024하니까 전송 잘 안됐음 - 512가 최대인듯하다.(대략 14개 파일 = 540크기)		
			//printf("send len = %d\n",ll); //위에 딜레이 없어도 이상하게 이쪽에 printf 한번만써줘도 전송 오류가 없어졌다 이유를 모르겠다.
		}		
	}

	if (client_info[num].id[0] == NULL) // 종료하는 과정 - 이름이 없었다면 custom으로 지정해준다.
		strcpy(client_info[num].id, "custom");
	printf("============================\n");
	printf("        %s is out\n", client_info[num].id);
	printf("============================\n\n");

	alive_clnt[num] = 0; // 클라이언트 연결이 안된것을 의미한다. (접속 끊길 때 최신화) - 자기것만 쓰기에 굳이 뮤텍스 쓸필요 없음

	sprintf(buf, "./list/file_list%d.txt", num); // 종료하기 때문에 해당 클라이언트에서 받은 리스트 파일은 삭제
	remove(buf);

	close(client_sock);
	pthread_exit(0);
	return;
}

void login_check(int num, int client_sock)
{
	char buf[BUFFSIZE] = { 0, };
	char input_id[20]; // 임시 아이디 저쟝용
	char input_pw[20];
	while (1)
	{
		if (!recv(client_sock, buf, sizeof(buf),0)) break; // 아이디 입력 받기 - 중간 종료시 에러 방지 break
		strcpy(input_id, buf);
		if (!recv(client_sock, buf, sizeof(buf),0)) break; // 비밀번호 입력 받기
		strcpy(input_pw, buf);
		sprintf(buf, LOGIN_MSG, num, input_id, input_pw);
		printf("%s", buf); // 로그인 정보 출력				

		for (int i = 0; i < MAX_ID; i++) // id max개 이후 수정해야할 부분임
		{
			if (!strcmp(user_id[i], input_id))
			{
				if (!strcmp(user_pw[i], input_pw))
				{
					int flag = 1; // 중복 로그인 체크용 플레그 비트;
					for (int i = 0; i < MAX_CLIENT; i++) // 현재 로그인중인 사람들중에 id가 중복되는지 체크 (동시 접속 타이밍이 완전 절묘하게 맞으면 동시로그인이 가능하긴함)
					{
						if (alive_clnt[i])
						{
							if (!strcmp(client_info[i].id, input_id)) // 중복된 ID가 접속해있을 때
							{
								sprintf(buf, ID_DUPERROR, input_id);
								printf("%s\n", buf);
								send(client_sock, buf, sizeof(buf), 0); // 아이디 중복 에러 메세지 전송
								flag = 0;
								break; // 계속 for문을 돌 필요가 없기에 break
							}
						}
					}
					if (flag)
					{
						strcpy(client_info[num].id, input_id);// 로그인 성공 시 구조체에 로그인한 id 넣기
						strcpy(client_info[num].pw, input_pw);
						sprintf(buf, SUCCESS_MSG, client_info[num].id);
						send(client_sock, buf, sizeof(buf), 0); // 성공 메세지 전송
						printf("%s", buf);
						return;
					}
					else
						break;
				}
				else // 패스워드 틀렸을 때
				{
					printf(PW_ERROR"\n");
					send(client_sock, PW_ERROR, strlen(PW_ERROR) + 1, 0); // 패스워드 실패 메세지 전송
					break; // 계속 for문을 돌 필요가 없기에 break
				}
			}
			else if (i == MAX_ID - 1) // 아이디가 전부 다 틀렸을 때
			{
				printf(ID_ERROR"\n");
				send(client_sock, ID_ERROR, strlen(ID_ERROR) + 1, 0); // 아이디 실패 메세지 전송
			}
		}
	}
}

void filelist_remove()
{
	char buf[BUFFSIZE] = { 0, };
	for (int i = 0; i < MAX_CLIENT; i++)
	{
		sprintf(buf, "./list/file_list%d.txt", i);
		remove(buf);
	}
}