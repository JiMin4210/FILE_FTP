#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INIT_MSG "=========================================\nHello! I'm P2P File Sharing Server...\nPlease, LOG-IN!\n=========================================\n"
#define LOGIN_MSG "============================\nNum : %d User Information\nID: %s, PW: %s\n============================\n"
#define PW_ERROR "Log-in fail: Incorrect password...\n"
#define ID_ERROR "Log-in fail: Incorrect ID...\n"
#define SUCCESS_MSG "Log-in success!! [%s] *^^*\n\n"

#define SERV_IP "220.149.128.100"
#define SERV_PORT 4460
#define BACKLOG 10
#define BUFFSIZE 512 // ���� ������ ����
#define MAX_CLIENT 10 // �ִ� Ŭ���̾�Ʈ ����
#define MAX_ID 2 // ������ �� �ִ� ID ����

void *clnt_connection(void * sock);
void login_check(int num, int client_sock);

typedef struct Client_Info { // Ŭ���̾�Ʈ���� ������ ������� ����ü
	char id[20];
	char pw[20];
	char ip[16];
	char port[5];
	int sock;
}Client_Info;

char *user_id[2] = { "user1", "user2" };
char *user_pw[2] = { "passwd1", "passwd2" };
int client_count;
Client_Info client_info [MAX_CLIENT]; // �ִ� Ŭ���̾�Ʈ ������ŭ ���� ���� ����.

pthread_mutex_t mutex; // �����带 �����̱⿡ �ڿ� ���� ����� �����������.

int main(void)
{
	system("clear"); // ���� �� ȭ�� �����ϰ�
	int sockfd, new_fd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	unsigned int sin_size;
	int val = 1;
	
	pthread_t thread; // ������ ����� ���� ���� - �������� ���� ���� Ȯ���� ���� (���� = ������ ã�� ����)
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

	while (1) {
		sin_size = sizeof(struct sockaddr_in);
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

		pthread_mutex_lock(&mutex); // client_count ������ �ڿ� ���û���� ����������Ѵ�.(���� ���ӽ� ���� ����)
		client_info[client_count].sock = new_fd;
		pthread_create(&thread, NULL, clnt_connection, (void *)client_count++);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}

void *clnt_connection(void * count) {
	int num = (int)count; // ���� ���° Ŭ���̾�Ʈ����
	int len; // ������ ������ ���� �� ������ ����
	int client_sock = client_info[num].sock;
	char buf[BUFFSIZE];

	printf("Num : %d User Accept() is OK...\n\n", num);
	send(client_sock, INIT_MSG, strlen(INIT_MSG) + 1, 0); // �����޼��� ����

	login_check(num, client_sock); // �α��� �Լ�
	while (1) {
		if(!recv(client_sock, buf, sizeof(buf),0)) break;
		//printf("number = %s\n", buf);
		
		if (!strcmp(buf, "1"))
		{
			printf("============================\n");
			printf("* %s updated file-list. \n", client_info[num].id);
			printf("============================\n\n");
			recv(client_sock, buf, sizeof(buf),0); // file_list����
			//printf("\n%s\n", buf); // ���� ���ϸ���Ʈ ���
			FILE *fp; // �ӽ� ���Ͽ�
			FILE *fp_all; // ���ϸ���Ʈ �ۼ���

			pthread_mutex_lock(&mutex); // file_list_test ������ �ڿ� ���û���� ����������Ѵ�.(���� ���ӽ� ���� ����)
			fp = fopen("./list/file_list_test.txt", "w");
			fwrite(buf, 1, strlen(buf), fp); // �ӽ����ϸ���Ʈ �ۼ�
			fclose(fp);

			recv(client_sock, buf, sizeof(buf), 0); // IP�ּ� ���� (���� ���� �Ϸ� ��)
			strcpy(client_info[num].ip, buf);
			for (int i = 0; i < 5000; i++);
			printf("ip = %s\n", buf);
			recv(client_sock, buf, sizeof(buf), 0); // ��Ʈ ��ȣ ����
			strcpy(client_info[num].port, buf);
			printf("port = %s\n", buf);

			fp = fopen("./list/file_list_test.txt", "r"); // �б���� �ӽ����� ����
			char file_name[50];
			sprintf(file_name, "./list/file_list%d.txt", num); // ���۹��� ����Ʈ ���� �̸� ����
			fp_all = fopen(file_name, "w"); // ������� ���ϸ���Ʈ ����
			while (fgets(buf, sizeof(buf), fp) != NULL)
			{
				char info[50];
				buf[strlen(buf)-1] = '\t'; // \n�����ֱ�
				sprintf(info, "%s\t%s\t%s\n", client_info[num].id, client_info[num].ip, client_info[num].port); // ip, port�ּ� �ٿ��ֱ�
				strcat(buf, info);
				fputs(buf, fp_all);
			}

			fclose(fp); // �ӽ����� �ݰ�
			remove("./list/file_list_test.txt"); // �ӽ����� ����
			fclose(fp_all);			
			pthread_mutex_unlock(&mutex);
		}

		if (!strcmp(buf, "3")) // ���� ���ϸ���Ʈ�� �����ϰ� ���� �� ���������� Ȯ�� -> �������� ���� ���ľ� ���� �ֽ�ȭ�� ���� ��� ����(����)
		{
			printf("============================\n");
			printf("* %s requested file-list. \n", client_info[num].id);
			printf("============================\n\n");
			char merge[512];
			int count = 0;
			memset(buf, 0, sizeof(buf)); // ���� �ʱ�ȭ
			FILE *fp;
			FILE *fp_all;

			pthread_mutex_lock(&mutex); // file_list ������ �ڿ� ���û���� ����������Ѵ�.(���� ���ӽ� ���� ����) - �д� ���� �����Ǹ� �ȵ�
			fp = fopen("./list/file_list0.txt", "r"); // 0, 1 ������ �ƴ� ���������� �����ϰ� �ٲ��ֱ� + ����� ������ �����ǰ� �ϱ�
			if (fp)
			{
				memset(merge, 0, sizeof(merge));
				fread(merge, 1, 512, fp);
				strcat(buf, merge);
				fclose(fp);
			}
			
			fp = fopen("./list/file_list1.txt", "r"); // for�� ���� - ����ִ� �����常
			if (fp)
			{
				memset(merge, 0, sizeof(merge));
				fread(merge, 1, 512, fp);
				strcat(buf, merge);
				fclose(fp); // ������� ���� ��ġ�� ����
			}

			fp = fopen("./list/file_list_test.txt", "w"); // �ӽ����� ����
			fwrite(buf, 1, strlen(buf), fp);
			fclose(fp);

			fp = fopen("./list/file_list_test.txt", "r");
			fp_all = fopen("./list/file_list.txt", "w");
			while (fgets(buf, sizeof(buf), fp) != NULL)
			{	
				if (++count > 9) // ��ȣ�� 10�� �Ѿ� �� ��
				{
					buf[0] = count / 10 + '0'; // �ƽ�Ű�ڵ� ��ȯ �ʼ�
					buf[1] = count % 10 + '0';
					buf[2] = '.';
				}
				else
					buf[0] = count + '0'; // �ƽ�Ű�ڵ� ��ȯ �ʼ�
				
				fputs(buf, fp_all);
			}
			fclose(fp);
			fclose(fp_all); // ������� ���� ��ȣ �ٲٴ� ����
			remove("./list/file_list_test.txt"); // �ӽ����� ����
			pthread_mutex_unlock(&mutex); // file_list ������ ���� �� ���ؽ� ������ֱ�

			fp_all = fopen("./list/file_list.txt", "r"); // ������ ���߿� FTP ���� �Լ��� �ٲ��ֱ� - ���� �Լ� ���̿��ص� �ɵ� ������ �ѹ��ۿ� �Ⱦ�
			memset(buf, 0, sizeof(buf)); // �ʱ�ȭ ����� ����ϰ� ���� ����. - fread�� �ʱ�ȭ�� �߿伺 ���� (������ �� ���ְ��ϴ°� �ƴ� �����°Ŷ� ����)
			fread(buf, 1, BUFFSIZE, fp_all);
			fclose(fp_all); // ���� ���� ��� ����
			//printf("\n%s\n", buf); // ���� ���� ����Ʈ ���� ���
			//printf("buf len = %d\n", strlen(buf)); // ���� ũ�� Ȥ�ø��� ���� - ��������
			//for (int i = 0; i < 5000; i++);
			send(client_sock, buf, strlen(buf), 0); // ���� ���ϸ���Ʈ ���� -> �뷮���� �ִµ� 1024�ϴϱ� ���� �� �ȵ��� - 512�� �ִ��ε��ϴ�.(�뷫 14�� ���� = 540ũ��)		
			//printf("send len = %d\n",ll); //���� ������ ��� �̻��ϰ� ���ʿ� printf �ѹ������൵ ���� ������ �������� ������ �𸣰ڴ�.
		}		
	}

	if (client_info[num].id[0] == NULL) // �����ϴ� ����
		strcpy(client_info[num].id, "custom");
	printf("============================\n");
	printf("        %s is out\n", client_info[num].id);
	printf("============================\n\n");
	close(client_sock);
	pthread_exit(0);
	return;
}

void login_check(int num, int client_sock)
{
	char buf[BUFFSIZE];
	while (1)
	{
		if (!recv(client_sock, buf, sizeof(buf),0)) break; // ���̵� �Է� �ޱ� - �߰� ����� ���� ���� break
		strcpy(client_info[num].id, buf);
		if (!recv(client_sock, buf, sizeof(buf),0)) break; // ��й�ȣ �Է� �ޱ�
		strcpy(client_info[num].pw, buf);
		sprintf(buf, LOGIN_MSG, num, client_info[num].id, client_info[num].pw);
		printf("%s", buf); // �α��� ���� ���				

		for (int i = 0; i < MAX_ID; i++) // id max�� ���� �����ؾ��� �κ���
		{
			if (!strcmp(user_id[i], client_info[num].id))
			{
				if (!strcmp(user_pw[i], client_info[num].pw))
				{
					sprintf(buf, SUCCESS_MSG, client_info[num].id);
					send(client_sock, buf, sizeof(buf), 0); // ���� �޼��� ����
					printf("%s", buf);
					return;
				}
				else // �н����� Ʋ���� ��
				{
					printf(PW_ERROR"\n");
					send(client_sock, PW_ERROR, strlen(PW_ERROR) + 1, 0); // �н����� ���� �޼��� ����
					break; // ��� for���� �� �ʿ䰡 ���⿡ break
				}
			}
			else if (i == MAX_ID - 1) // ���̵� ���� �� Ʋ���� ��
			{
				printf(ID_ERROR"\n");
				send(client_sock, ID_ERROR, strlen(ID_ERROR) + 1, 0); // ���̵� ���� �޼��� ����
			}
		}
	}
}

