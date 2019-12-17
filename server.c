#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<netinet/in.h>
#include<pthread.h>
#include<time.h>
 
#define BUF_SIZE 100
#define MAX_CLNT 100
#define MAX_IP 30
 
void * handle_clnt(void *arg);
void send_msg(char *msg, int len);
void error_handling(char *msg);
char* serverState(int count);
void menu(char port[]);
void check_arg(int argc, char *argv[]);
void socket_setting(char *argv[]);
void *ftpserv();
void *chatserv();
int tcp_listen(int host, int port, int backlog);


void* thread_return;
char buf[100], command[5], filename[20];
int clnt_cnt=0, len, filehandle, size, i, k, c;
int clnt_socks[MAX_CLNT];
int serv_sock, clnt_sock;
int sock1, sock2;
int clnt_adr_sz;
struct sockaddr_in serv_adr, clnt_adr, server, client;
struct stat obj;
struct tm *t;
time_t timer;
pthread_t t_id;
pthread_t chat, ftp;
pthread_mutex_t mutx;


int main(int argc, char *argv[])
{
    
    timer = time(NULL);
    t=localtime(&timer);
    check_arg(argc,argv);
    menu(argv[1]);
    pthread_mutex_init(&mutx, NULL);
    socket_setting(argv);
    
    while(1)
    {
        pthread_create(&chat, NULL, chatserv, NULL);
        pthread_create(&ftp, NULL, ftpserv, NULL);
        pthread_join(ftp, &thread_return);
        pthread_join(chat, &thread_return);
    }

    close(serv_sock);
    return 0;
}

void *chatserv()
{
   while(1)
    {
        t=localtime(&timer);
        clnt_adr_sz=sizeof(clnt_adr);
        clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
 
        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++]=clnt_sock;
        pthread_mutex_unlock(&mutx);
 
        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
        printf(" Connceted client IP : %s ", inet_ntoa(clnt_adr.sin_addr));
        printf("(%d-%d-%d %d:%d)\n", t->tm_year+1900, t->tm_mon+1, t->tm_mday,
        t->tm_hour, t->tm_min);
        printf(" chatter (%d/100)\n", clnt_cnt);
    }
}

void *ftpserv()
{
    while (1) {
		recv(sock2, buf, 100, 0);
		sscanf(buf, "%s", command);
		if (!strcmp(command, "ls")) {
			system("ls >temps.txt");
			stat("temps.txt", &obj);
			size = obj.st_size;
			send(sock2, &size, sizeof(int), 0);
			filehandle = open("temps.txt", O_RDONLY);
			sendfile(sock2, filehandle, NULL, size);
		}
		else if (!strcmp(command, "get")) {
			sscanf(buf, "%s%s", filename, filename);
			stat(filename, &obj);
			filehandle = open(filename, O_RDONLY);
			size = obj.st_size;
			if (filehandle == -1)
				size = 0;
			send(sock2, &size, sizeof(int), 0);
			if (size)
				sendfile(sock2, filehandle, NULL, size);

		}
		else if (!strcmp(command, "put")) {
			int c = 0, len;
			char *f;
			sscanf(buf + strlen(command), "%s", filename);
			recv(sock2, &size, sizeof(int), 0);

			while (1) {
				filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
				if (filehandle == -1)
					sprintf(filename + strlen(filename), "_1");
				else break;
			}
			f = malloc(size);
			recv(sock2, f, size, 0);
			c = write(filehandle, f, size);
			close(filehandle);
			send(sock2, &c, sizeof(int), 0);
		}
		else if (!strcmp(command, "pwd")) 
        {
			system("pwd>temp.txt");
			i = 0;
			FILE*f = fopen("temp.txt", "r");
			while (!feof(f)) buf[i++] = fgetc(f);
			buf[i - 1] = '\0';
			fclose(f);
			send(sock2, buf, 100, 0);
		}
		else if (!strcmp(command, "cd")) {
			if (chdir(buf + 3) == 0) c = 1;
			else c = 0;	
			send(sock2, &c, sizeof(int), 0);
		}
		else if (!strcmp(command, "quit")) 
        {
			i = 1;
            remove("temp.txt");
            remove("temps.txt");
			send(sock2, &i, sizeof(int), 0);
			break;
		}
	}
}
void socket_setting(char *argv[])
{
    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));
 
    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");
    if (listen(serv_sock, 5)==-1)
        error_handling("listen() error");

    sock1 = tcp_listen(INADDR_ANY, atoi(argv[1]), 5);

	len = sizeof(client);
	sock2 = accept(sock1, (struct sockaddr*)&client, &len);
}

void check_arg(int argc, char *argv[])
{
 if (argc != 2)
    {
        printf(" Usage : %s <port>\n", argv[0]);
        exit(1);
    }
 
}
void *handle_clnt(void *arg)
{
    int clnt_sock=*((int*)arg);
    int str_len=0, i;
    char msg[BUF_SIZE];
 
    while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)
        send_msg(msg, str_len);
 
    pthread_mutex_lock(&mutx);
    for (i=0; i<clnt_cnt; i++)
    {
        if (clnt_sock==clnt_socks[i])
        {
            while(i++<clnt_cnt-1)
                clnt_socks[i]=clnt_socks[i+1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}
 
void send_msg(char* msg, int len)
{
    int i;
    pthread_mutex_lock(&mutx);
    for (i=0; i<clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutx);
}
 
void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
 
char* serverState(int count)
{
    char* stateMsg = malloc(sizeof(char) * 20);
    strcpy(stateMsg ,"None");
    
    if (count < 5)
        strcpy(stateMsg, "Good");
    else
        strcpy(stateMsg, "Bad");
    
    return stateMsg;
}        
 
void menu(char port[])
{
    system("clear");
    printf(" **** moon/sun chat server ****\n");
    printf(" server port    : %s\n", port);
    printf(" server state   : %s\n", serverState(clnt_cnt));
    printf(" max connection : %d\n", MAX_CLNT);
    printf(" ****          Log         ****\n\n");
}

int tcp_listen(int host, int port, int backlog) {
	int sd;
	struct sockaddr_in servaddr;
    port++;
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket fail");
		exit(1);
	}
	
	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(host);
	servaddr.sin_port = htons(port);
	if (bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind fail");  exit(1);
	}
	
	listen(sd, backlog);
	return sd;
}
