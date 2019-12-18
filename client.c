#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<sys/sendfile.h>
#include<pthread.h>
#include<time.h>
#include<signal.h>


#define BUF_SIZE 100
#define NORMAL_SIZE 20
#define MAXLINE  511

void* send_msg(void* arg);
void* recv_msg(void* arg);
void error_handling(char* msg);
void menu();
void changeName();
void menuOptions();
void check_arg(int argc, char *argv[]);
void create_socket_connection(char *argv[]);
void print_time();
void chat();
int tcp_connect(int af, char *servip, unsigned short port);
void ftpsetting(char *argv[]);

char name[NORMAL_SIZE]="[DEFALT]";     // name
char msg_form[NORMAL_SIZE];            // msg form
char server_time[NORMAL_SIZE];        // server time
char msg[BUF_SIZE];                    // msg
char server_port[NORMAL_SIZE];        // server port number
char clnt_ip[NORMAL_SIZE];
char bufmsg[MAXLINE];
char buf[100], command[5], filename[MAXLINE],temp[20], *f;

int sock,ftpsock;
int filehandle;
int size, status;
struct sockaddr_in serv_addr;
struct sockaddr_in server;
struct stat obj;
pthread_t snd_thread, rcv_thread, ftp_thread;
void* thread_return;

int main(int argc, char *argv[])
{
    signal(SIGINT, SIG_IGN);
    check_arg(argc,argv);
    print_time();
    create_socket_connection(argv);
    ftpsetting(argv);
    menu();
    chat();
    return 0;
}

void ftpsetting(char *argv[])
{
    ftpsock = tcp_connect(AF_INET, argv[1], atoi(argv[2])+1);
	if (ftpsock == -1) {
		printf("tcp_connect fail");
		exit(1);
	}
}
void check_arg(int argc, char *argv[])
{
    if (argc!=4)
    {
        printf(" Usage : %s <ip> <port> <name>\n", argv[0]);
        exit(1);
    }
}

void chat()
{
    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);
    send(ftpsock, buf, 100, 0);
	recv(ftpsock, &status, 100, 0);
    close(sock);
}

void create_socket_connection(char *argv[])
{
    sprintf(name, "[%s]", argv[3]);
    sprintf(clnt_ip, "%s", argv[1]);
    sprintf(server_port, "%s", argv[2]);
    sock=socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
        error_handling(" conncet() error");
}

void print_time()
{
    struct tm *t;
    time_t timer = time(NULL);
    t=localtime(&timer);
    sprintf(server_time, "%d-%d-%d %d:%d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour,t->tm_min);
}

void* ftp(void *arg)
{
    while (1) 
    {
		printf("\033[1;33mcommand : get, put, pwd, ls, cd, quit\n");
		printf("\033[1;32mclient> ");
		fgets(bufmsg, MAXLINE, stdin); 
		fprintf(stderr, "\033[97m");
		if (!strcmp(bufmsg, "get\n")) {
			printf("filename to download : ");
			scanf("%s", filename);       
			fgets(temp, MAXLINE, stdin); 
			strcpy(buf, "get ");
			strcat(buf, filename);
			send(ftpsock, buf, 100, 0);        
			recv(ftpsock, &size, sizeof(int), 0);
			if (!size) {
				printf("no file named %s\n", filename);
				continue;
			}
			f = malloc(size);
			recv(ftpsock, f, size, 0);
			while (1) {
				filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0777);
				if (filehandle == -1)
					sprintf(filename + strlen(filename), "_1");
				else break;
			}
			write(filehandle, f, size);
			close(filehandle);
			printf("Download finished\n");
		}
		else if (!strcmp(bufmsg, "put\n")) {
			printf("filename to upload : ");
			scanf("%s", filename);       
			fgets(temp, MAXLINE, stdin); 
			filehandle = open(filename, O_RDONLY);
			if (filehandle == -1) {
				printf("no file named %s\n", filename);
				continue;
			}
			strcpy(buf, "put ");
			strcat(buf, filename);
			send(ftpsock, buf, 100, 0);
			stat(filename, &obj);
			size = obj.st_size;
			send(ftpsock, &size, sizeof(int), 0);
			sendfile(ftpsock, filehandle, NULL, size);
			recv(ftpsock, &status, sizeof(int), 0);
			if (status)
				printf("upload success\n");
			else
				printf("upload fail\n");
		}
		else if (!strcmp(bufmsg, "pwd\n")) {
			strcpy(buf, "pwd");
			send(ftpsock, buf, 100, 0);
			recv(ftpsock, buf, 100, 0);
			printf("--The path of the Remote Directory--\n%s", buf);
		}
		else if (!strcmp(bufmsg, "ls\n")) {
			strcpy(buf, "ls");
			send(ftpsock, buf, 100, 0);
			recv(ftpsock, &size, sizeof(int), 0);
			f = malloc(size);
			recv(ftpsock, f, size, 0);
			filehandle = creat("temp.txt", 0777);
			write(filehandle, f, size);
			close(filehandle);
			printf("--The Remote Directory List--\n");
			system("cat temp.txt");
		}
		else if (!strcmp(bufmsg, "cd\n")) {
			strcpy(buf, "cd ");
			printf("path to move : ");
			scanf("%s", buf + 3);        
			fgets(temp, MAXLINE, stdin); 
			send(ftpsock, buf, 100, 0);  
			recv(ftpsock, &status, sizeof(int), 0);
			if (status)
				printf("success to change directory\n");
			else
				printf("fail to change directory\n");
		}
		else if (!strcmp(bufmsg, "quit\n")) {
			strcpy(buf, "quit");
            printf("Exit ftp service bye~\n");
            /*
            send(ftpsock, buf, 100, 0);
			recv(ftpsock, &status, 100, 0);
			if (status) {
				printf("Exit ftp service bye~\n");
				break;
			}*/
            break;
		}
	}
}
void* send_msg(void* arg)
{
    int sock=*((int*)arg);
    char name_msg[NORMAL_SIZE+BUF_SIZE];
    char myInfo[BUF_SIZE];
    char* who = NULL;
    char temp[BUF_SIZE];

    printf(" >> join the chat !! \n");
    sprintf(myInfo, "%s's join. IP_%s\n",name , clnt_ip);
    write(sock, myInfo, strlen(myInfo));

    while(1)
    {
        fgets(msg, BUF_SIZE, stdin);

        if (!strcmp(msg, "!menu\n"))
        {
            menuOptions();
            continue;
        }

        else if(!strcmp(msg, "!file\n"))
        {
            pthread_create(&ftp_thread, NULL, ftp, (void*)&ftpsock);
            pthread_join(ftp_thread, &thread_return);
            pthread_detach(ftp_thread);
            continue;
        }

        else if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
        {
            printf(" >> Bye Bye !! \n");
            sprintf(myInfo, "%s's exit. IP_%s\n",name , clnt_ip);
            write(sock, myInfo, strlen(myInfo));
            close(sock);
            exit(0);
        }

        sprintf(name_msg, "%s %s", name,msg);
        write(sock, name_msg, strlen(name_msg));
    }
    return NULL;
}

void* recv_msg(void* arg)
{
    int sock=*((int*)arg);
    char name_msg[NORMAL_SIZE+BUF_SIZE];
    int str_len;

    while(1)
    {
        str_len=read(sock, name_msg, NORMAL_SIZE+BUF_SIZE-1);
        if (str_len==-1)
            return (void*)-1;
        name_msg[str_len]=0;
        fputs(name_msg, stdout);
    }
    return NULL;
}

void menuOptions() 
{
    int select;
    printf("\n\t**** menu mode ****\n");
    printf("\t1. change name\n");
    printf("\t2. clear/update\n\n");
    printf("\tother key will be cancel");
    printf("\n\t*******************");
    printf("\n\t>> ");
    scanf("%d", &select);
    getchar();
    switch(select)
    {
        // change user name
        case 1:
        changeName();
        break;

        case 2:
        menu();
        break;

        default:
        printf("\tcancel.");
        break;
    }
}

void changeName()
{
    char nameTemp[100];
    printf("\n\tInput new name -> ");
    scanf("%s", nameTemp);
    sprintf(name, "[%s]", nameTemp);
    printf("\n\tComplete.\n\n");
}

void menu()
{
    system("clear");
    printf(" **** Blah Blah ****\n");
    printf(" server port : %s \n", server_port);
    printf(" client IP   : %s \n", clnt_ip);
    printf(" chat name   : %s \n", name);
    printf(" server time : %s \n", server_time);
    printf(" ************* menu ***************\n");
    printf(" if you want to select menu -> !menu\n");
    printf(" 1. change name\n");
    printf(" 2. clear/update\n");
     printf(" if you want to do ftp -> !file\n");
    printf(" **********************************\n");
    printf(" Exit -> q & Q\n\n");
}    

void error_handling(char* msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
} 

int tcp_connect(int af, char *servip, unsigned short port)
 {
	struct sockaddr_in servaddr;
	int  s;
	
	if ((s = socket(af, SOCK_STREAM, 0)) < 0)
		return -1;
	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = af;
	inet_pton(AF_INET, servip, &servaddr.sin_addr);
	servaddr.sin_port = htons(port);

	if (connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr))
		< 0)
		return -1;
	return s;
}

