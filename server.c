#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INIT_MSG "=====================================================\nHello! I'm P2P File Sharing Server...\nPlease, LOG-IN!\n=====================================================\n"

#define LOGIN_MSG "====================================\nNum : %d User Information\nID: %s, PW: %s\n====================================\n" // �α��� �޼���
#define PW_ERROR "Log-in fail: Incorrect password...\n"
#define ID_ERROR "Log-in fail: Incorrect ID...\n"
#define ID_DUPERROR "Log-in fail: %s is already connecting to the server..\n"
#define SUCCESS_MSG "Log-in success!! [%s] *^^*\n"

#define SIGNUP_MSG "====================================\nNum : %d User try to Sign up!\nID: %s, PW: %s\n====================================\n" // ȸ������ �޼���
#define SIGNUP_ERROR "Sign-up fail: %s is already exists on the server...\n"
#define MAXID_ERROR "Sign-up fail: full ID on the surver, plz delet ID....\n"
#define SIGNUP_SUCCESS "Sign-up success!! [%s] *^^*\n"

#define NO_FILE "No files on all clients\n"
#define NO_ACCOUNT "That number ID doesn't exist\n" // ���� �������� �޼���
#define SUCCESS_ACCOUNT "ID : %s has been successfully deleted\n" // ���� ���� ���� �޼���
#define ALIVE_ACCOUNT "ID : %s is already connecting to the server, cannot be deleted!\n" // ���� �������� �޼���
#define ERROR_ACCOUNT "ID : %s doesn't exist\n" // ���� �������� �޼���

#define SERV_IP "220.149.128.100"
#define SERV_PORT 4460
#define BACKLOG 10
#define BUFFSIZE 512 // ���� ������ ����
#define MAX_CLIENT 15 // �ִ� Ŭ���̾�Ʈ ����
#define MAX_ID 15 // ������ �� �ִ� ID ����

void *clnt_connection(void * sock);
int login_check(int num, int client_sock);
void sign_up(int num, int client_sock);
void update_id();
void filelist_remove();

typedef struct Client_Info { // Ŭ���̾�Ʈ���� ������ ������� ����ü
	char id[20];
	char pw[20];
	char ip[16];
	char port[5];
	int sock;
}Client_Info;

char user_id[MAX_ID][20]; // ID ����� ����
char user_pw[MAX_ID][20];
int client_count; // ���� ���° Ŭ���̾�Ʈ���� �����߾�����
Client_Info client_info [MAX_CLIENT]; // �ִ� Ŭ���̾�Ʈ ������ŭ ���� ���� ����.
int alive_clnt[MAX_CLIENT]; // ���� ����Ǿ��ִ� Ŭ���̾�Ʈ�� �ǹ��Ѵ�. (1 = ���� �� , 0 = ���� �ȵ�)

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

	filelist_remove(); // ���� ����Ʈ�� �޾Ƽ� �ϴ°��̱⿡ ������ ���� ���ϸ���Ʈ�� ���� �Ѵٸ� ��ġ�⿡
					   // ������ �ѹ� �������Ѵ�. (���� ������ �ٷβ��� �Ǹ� ���� ������ ���������� ������ ������ �׷���)
					   // ���� Ŭ���̾�Ʈ���� �� ���شٸ� ������ �� �������⿡ ��ǻ� �ʿ������ Ȥ�ø� ��Ȳ ���

	update_id(); // ���� ID, PW�� ���Ͽ��� �޾ƿ´�. (���Ϸ� �����Ұ��̱� ����)

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
	char buf[BUFFSIZE] = { 0, };

	printf("Num : %d User Accept() is OK...\n\n", num);
	send(client_sock, INIT_MSG, strlen(INIT_MSG) + 1, 0); // �����޼��� ����

	alive_clnt[num] = 1; //Ŭ���̾�Ʈ�� connect�����ΰ��� �ǹ�

	while (1) // Log_In Menu ����
	{
		if (!recv(client_sock, buf, sizeof(buf), 0)) break;

		if (!strcmp(buf, "1")) // �α��� ����
		{
			if (login_check(num, client_sock)) 
				break; // �α��� �Լ� (���� �� while ��������)
		}
		else if (!strcmp(buf, "3")) // ȸ������ ����
			sign_up(num, client_sock); 

		else if (!strcmp(buf, "2") || !strcmp(buf, "4")) // 2 = �������� ���� Ȯ��, 4 = ���� ����
		{
			printf("====================================\n");
			printf("* Costom requested Account-list. \n");
			printf("====================================\n\n");
			int num = atoi(buf); // 2������ 4������ �˱� ����
			char alive[MAX_ID][15] = { 0, };
			char user_info[BUFFSIZE] = { 0, };
			char id_buf[MAX_ID][20] = { 0, }; // ���� ID ������ �ٷ� �̿��ص� ������ ���� ����Ʈ�� ���� �� �ٸ� �����忡���� �ǽð����� ���� ����Ʈ�� ���� �Ǿ ���� ID�� �Ǻ��ϱ� ���� ���� �������ش�.
			memset(buf, 0, BUFFSIZE); // ���� �ʱ�ȭ �����ְ� strcat�� ���� ���� ����Ʈ�� ���� ���̴�.
			for (int i = 0; i < MAX_ID; i++)
			{
				if (!user_id[i][0]) break;
				strcpy(id_buf[i], user_id[i]);
				strcpy(alive[i], "disconnected");
				for (int x = 0; x < MAX_CLIENT; x++) // �ش� ID�� ���� ������ ����Ǿ��ִ���, ���� �Ǿ��ִٸ� ǥ�����ش�. (�������� ����)
				{
					if (alive_clnt[x])
					{
						if (!strcmp(client_info[x].id, id_buf[i]))
						{
							strcpy(alive[i], "connecting");
							break;
						}
					}
				}
				sprintf(user_info, "%d.\t%s\t%s\n", i + 1, id_buf[i],alive[i]);
				strcat(buf, user_info); // ��������Ʈ ������ ���� ���ۿ� ���� ���� ����
			}	
			send(client_sock, buf, sizeof(buf), 0); // ���� ���� ����Ʈ�� ������

			if (num == 4) // 2���� �� �������� �ص� ��������� 4���� ���⼭ ��ȣ�� �ް� ���� �� �� �־����
			{
				char delete_id[20] = { 0, };
				int delete_num = 0;
				if (!recv(client_sock, buf, sizeof(buf), 0)) break;
				delete_num = atoi(buf) - 1; // ������ ID ��ȣ�̴�.
				if (delete_num >= 0 && delete_num < MAX_ID)
				{
					if (!id_buf[delete_num][0])
					{
						send(client_sock, NO_ACCOUNT, strlen(NO_ACCOUNT) + 1, 0);
						continue;
					}

					strcpy(delete_id, id_buf[delete_num]); // ���� ��ȣ�� ���� �� �ٸ� �����忡�� ���������� ���� ID ��ȣ�� �ٲ�����ٸ� ���� ������ user_id�� �ٲ�����Ű� �ε��� ��ȣ�� ������ �ٵ�
															  // �ӽ� ID ���ۿ� ��������⶧���� �ε��� ��ȣ�� �������� ID������ �� �̷������ �ȴ�.

					printf("====================================\n");
					printf("* Account '%s' deletion request. \n", delete_id);
					printf("====================================\n");

					for (int i = 0; i < MAX_CLIENT; i++) // ���ʿ��� scanf,recv�� ��ȣ �޴� �ð����� �ٸ� �����忡 ���� �ش� ID�� ������ ���� �� �� ������ ���õ� ID�� ����Ǿ����� �ٽ� Ȯ�����ش� (�ֽ�ȭ)
					{
						if (alive_clnt[i])
						{
							if (!strcmp(client_info[i].id, delete_id))
							{
								strcpy(alive[delete_num], "connecting");
								break;
							}
						}
					}

					if (!strcmp(alive[delete_num], "connecting")) // ���� ������ �������� ID�� ������ �Ұ����ϴٴ� ���� �˸�
					{
						sprintf(buf, ALIVE_ACCOUNT, delete_id);
						send(client_sock, buf, BUFFSIZE, 0);
						printf("%s\n", buf);
						continue;
					}

					pthread_mutex_lock(&mutex); // ���� ������ �̷�����⿡ ���ؽ� �ʼ�

					system("cp User_info.txt buffer.txt"); // �̷� �ý��� ��ɾ �ᵵ ���������� �𸣰ڴ�. (���� ������ ���� �ӽ����� ����)
					FILE *fp;
					FILE *fp_real;
					fp = fopen("buffer.txt", "r"); // �ӽ������� �̿��� ���� ������ �����Ѵ�.
					fp_real = fopen("User_info.txt", "w"); // ���� ���� ���� ������ ������ �������ش� (���õ� ID, PW�� �����ش�)

					int success_flag = 0;
					while (fgets(buf, BUFFSIZE, fp) != NULL)
					{
						char *point = buf; // ID�� ã�����̴�.
						char search_id[20] = { 0, };
						int count_word = 0;

						while (*point != '\t') // ID ã�� ����
							search_id[count_word++] = *point++;

						if (!strcmp(search_id, id_buf[delete_num])) // strstr�� �Ⱦ� ������ user1, user ���̵� ���� �� user�� ����� �ϸ� user1�� ������ ������ �ֱ� ����
						{
							char msg[BUFFSIZE] = { 0, };
							sprintf(msg, SUCCESS_ACCOUNT, id_buf[delete_num]); // ���� ���� �޼��� ����
							printf("%s\n", msg);
							send(client_sock, msg, sizeof(msg), 0);
							success_flag = 1;
							continue; // ��������ϴ� ID�� �����ϰ� �������� �� ���ش�.
						}
						fputs(buf, fp_real);
					}
					if (!success_flag)
					{
						send(client_sock, NO_ACCOUNT, strlen(NO_ACCOUNT) + 1, 0); // �ش� ID�� �����ٴ°��� �˸�.
						printf(ERROR_ACCOUNT"\n", id_buf[delete_num]);
					}
					fclose(fp);
					fclose(fp_real);
					remove("buffer.txt"); // �ӽ� ���� ����
					update_id(); // id, pw���� �ֽ�ȭ

					pthread_mutex_unlock(&mutex);
				}
				else
				{
					send(client_sock, NO_ACCOUNT, strlen(NO_ACCOUNT) + 1, 0);
				}
			}
		}
		
	}

	while (1) // Main Menu ����
	{ 
		if(!recv(client_sock, buf, sizeof(buf),0)) break;
		//printf("number = %s\n", buf);
		
		if (!strcmp(buf, "1"))
		{		
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
			recv(client_sock, buf, sizeof(buf), 0); // ��Ʈ ��ȣ ����
			strcpy(client_info[num].port, buf);

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
			printf("====================================\n");
			printf("* %s updated file-list. \n", client_info[num].id);
			printf("====================================\n\n");
		}

		else if (!strcmp(buf, "2")) // ���� ���ϸ���Ʈ�� �����ϰ� ���� �� ���������� Ȯ�� -> �������� ���� ���ľ� ���� �ֽ�ȭ�� ���� ��� ����(����)
		{
			printf("====================================\n");
			printf("* %s requested file-list. \n", client_info[num].id);
			printf("====================================\n\n");
			char merge[512];
			int count = 0;
			memset(buf, 0, sizeof(buf)); // ���� �ʱ�ȭ
			FILE *fp;
			FILE *fp_all;

			pthread_mutex_lock(&mutex); // file_list ������ �ڿ� ���û���� ����������Ѵ�.(���� ���ӽ� ���� ����) - �д� ���� �����Ǹ� �ȵ�

			for (int i = 0; i < MAX_CLIENT; i++) // ����� ������ �����ǰ� �ϱ� - clear // for�� ���� - ����ִ� �����常 - clear
			{
				if (alive_clnt[i]) // Ŭ���̾�Ʈ�� ������ �Ǿ��ִٸ� ������ ���� �������ش�. - �������� ���� = �ڿ������� ���⼭ ������
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
			} // ������� ���� ��ġ�� ����

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
			if(!strlen(buf))
				send(client_sock, NO_FILE, strlen(NO_FILE) + 1, 0); // ��� Ŭ���̾�Ʈ�� ������ ���°�� ���ٴ� �޼��� ����
			else
				send(client_sock, buf, strlen(buf), 0); // ���� ���ϸ���Ʈ ���� -> �뷮���� �ִµ� 1024�ϴϱ� ���� �� �ȵ��� - 512�� �ִ��ε��ϴ�.(�뷫 14�� ���� = 540ũ��)		
		}		
	}

	if (client_info[num].id[0] == NULL) // �����ϴ� ���� - �̸��� �����ٸ� custom���� �������ش�.
		strcpy(client_info[num].id, "custom");
	printf("====================================\n");
	printf("            %s is out\n", client_info[num].id);
	printf("====================================\n\n");

	alive_clnt[num] = 0; // Ŭ���̾�Ʈ ������ �ȵȰ��� �ǹ��Ѵ�. (���� ���� �� �ֽ�ȭ) - �ڱ�͸� ���⿡ ���� ���ؽ� ���ʿ� ����

	sprintf(buf, "./list/file_list%d.txt", num); // �����ϱ� ������ �ش� Ŭ���̾�Ʈ���� ���� ����Ʈ ������ ����
	remove(buf);

	close(client_sock);
	pthread_exit(0);
	return;
}

int login_check(int num, int client_sock) // �α��� ������ 1 ���� + ���� ���� �� 1 ����
{
	char buf[BUFFSIZE] = { 0, };
	char input_id[20]; // �ӽ� ���̵� ������
	char input_pw[20];
	
	if (!recv(client_sock, buf, sizeof(buf), 0)) return 1; // ���̵� �Է� �ޱ� - �߰� ����� ���� ���� break
	strcpy(input_id, buf);
	if (!recv(client_sock, buf, sizeof(buf), 0)) return 1; // ��й�ȣ �Է� �ޱ�
	strcpy(input_pw, buf);
	sprintf(buf, LOGIN_MSG, num, input_id, input_pw);
	printf("%s", buf); // �α��� ���� ���				

	for (int i = 0; i < MAX_ID; i++) // id max�� ���� �����ؾ��� �κ���
	{

		if (!strcmp(user_id[i], input_id))
		{
			if (!strcmp(user_pw[i], input_pw))
			{
				int flag = 1; // �ߺ� �α��� üũ�� �÷��� ��Ʈ;
				for (int i = 0; i < MAX_CLIENT; i++) // ���� �α������� ������߿� id�� �ߺ��Ǵ��� üũ (���� ���� Ÿ�̹��� ���� �����ϰ� ������ ���÷α����� �����ϱ��� - �ٵ� ���� �ߺ�üũ �� �Ǵ°� Ȯ��)
				{
					if (alive_clnt[i])
					{
						if (!strcmp(client_info[i].id, input_id)) // �ߺ��� ID�� ���������� ��
						{
							sprintf(buf, ID_DUPERROR, input_id);
							printf("%s\n", buf);
							send(client_sock, buf, sizeof(buf), 0); // ���̵� �ߺ� ���� �޼��� ����
							flag = 0;
							break; // ��� for���� �� �ʿ䰡 ���⿡ break
						}
					}
				}
				if (flag)
				{
					strcpy(client_info[num].id, input_id);// �α��� ���� �� ����ü�� �α����� id �ֱ�
					strcpy(client_info[num].pw, input_pw);
					sprintf(buf, SUCCESS_MSG, client_info[num].id);
					send(client_sock, buf, sizeof(buf), 0); // ���� �޼��� ����
					printf("%s\n", buf);
					return 1; // ù ȭ�� ����
				}
				else
					return 0;
			}
			else // �н����� Ʋ���� ��
			{
				printf(PW_ERROR"\n");
				send(client_sock, PW_ERROR, strlen(PW_ERROR) + 1, 0); // �н����� ���� �޼��� ����
				return 0;
			}
		}
		else if (!strcmp(user_id[i], "") || i == MAX_ID - 1) // ���̵� ���� �� Ʋ���� ��
		{
			printf(ID_ERROR"\n");
			send(client_sock, ID_ERROR, strlen(ID_ERROR) + 1, 0); // ���̵� ���� �޼��� ����
			return 0;
		}
	}

}

void sign_up(int num, int client_sock) // ȸ������ ���� �� �� - ���Ϸ� �������ν� ���� ������ ������
{
	char buf[BUFFSIZE] = { 0, };
	char input_id[20]; // �ӽ� ���̵� ������
	char input_pw[20];
	
	if (!recv(client_sock, buf, sizeof(buf), 0)) return; // ���̵� �Է� �ޱ� - �߰� ����� ���� ���� break
	strcpy(input_id, buf);
	if (!recv(client_sock, buf, sizeof(buf), 0)) return; // ��й�ȣ �Է� �ޱ�
	strcpy(input_pw, buf);
	sprintf(buf, SIGNUP_MSG, num, input_id, input_pw);
	printf("%s", buf); // ȸ������ ���� ���	

	//update_id(); // id, pw �ֽ�ȭ ��Ű�� �񱳰� �Ǵٰ� �����Ҽ� �ִµ�, ��¼�� �� ���渶�� �ڵ� �ֽ�ȭ�̱⿡ ���� �ֽ�ȭ�� ���⼭ �� �ʿ�� ����.

	pthread_mutex_lock(&mutex); // user_id, user_pw�� ���� + User_info.txt�� ������ ���̱⿡ ���ؽ��� ���û���� �������ش�.
	for (int i = 0; i < MAX_ID; i++) // id �ִ� ������ŭ for��
	{
		if (!strcmp(user_id[i], input_id))
		{
			sprintf(buf, SIGNUP_ERROR, input_id);
			printf("%s\n", buf);
			send(client_sock, buf, sizeof(buf), 0); // ���̵� �ߺ� ���� �޼��� ����
			break;
		}
		else if (!strcmp(user_id[i], "")) // ���̵� ĭ�� ����ִٸ�
		{
			sprintf(buf, "%s\t%s\n", input_id, input_pw); // ���Ͽ� id, pw �־��� �غ�
			FILE *fp;
			fp = fopen("User_info.txt", "a");
			fputs(buf, fp); // ���Ͽ� id, pw�־��ְ� �ݾ��ش�. - ���� ���� ���⼭ �߻� (�ܼ� fp�Ƚ��� ��������)
			fclose(fp);
			update_id(); // id, pw ���� �ֽ�ȭ

			sprintf(buf, SIGNUP_SUCCESS, input_id);
			send(client_sock, buf, sizeof(buf), 0); // ���� �޼��� ����
			printf("%s\n", buf);
			break;
		}
		else if (i == MAX_ID - 1)
		{
			printf(MAXID_ERROR"\n");
			send(client_sock, MAXID_ERROR, strlen(MAXID_ERROR) + 1, 0); // ���̵� ���� ������ ���ٴ� �����޼��� ����
		}
	}	
	pthread_mutex_unlock(&mutex);
}

void update_id()
{
	char buf[BUFFSIZE] = { 0, };
	int num = 0; // ���° ID����
	int topic_count = 0; // �ܾ� ������ ����
	int count = 0; // ���� ����뵵
	char id_pw[2][20] = { 0, };

	FILE *fp;
	fp = fopen("User_info.txt", "r");
	if (!fp) // ������ ���ٸ� ����� �����ֱ�
	{
		system("touch User_info.txt");
		fp = fopen("User_info.txt", "r");
	}
	memset(user_id, 0, sizeof(user_id)); // id, pw �ʱ�ȭ�ϰ� �ٽ� �ִ��۾� ����
	memset(user_pw, 0, sizeof(user_pw));
	while (fgets(buf, BUFFSIZE, fp) != NULL)
	{
		char *point = buf; // �����͸� �̿��� �ܾ ������ ���̴�.
		while (*point != '\n') // '\n'�� ������ �����Ѵ�.
		{
			id_pw[topic_count][count++] = *point;
			point++;
			if (*point == '\t')
			{
				point++;
				topic_count++;
				count = 0;
			}
		}
		strcpy(user_id[num], id_pw[0]); // id, pw �ֽ�ȭ �����ش�.
		strcpy(user_pw[num++], id_pw[1]);

		memset(id_pw, 0, sizeof(id_pw)); // ��� ���� �ʱ�ȭ = ���� ������ �о���ϱ� ����
		topic_count = 0;
		count = 0;
	}
	fclose(fp);
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