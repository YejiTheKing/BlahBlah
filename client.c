#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

#define BUF_SIZE 100
#define NORMAL_SIZE 20
#define MAX_SIZE 511

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);

void menu();
void changeName();
void menuOptions();
void check_arg(int argc, char *argv[]);
void chat_socket(char *argv[]);
int ftp_socket(char *servip, unsigned short port);
void print_time();
void chat();

void fileoption();
void get();
void put();
void pwd();
void ls();
void cd();

char name[NORMAL_SIZE] = "[DEFALT]"; // name
char msg_form[NORMAL_SIZE];          // msg form
char server_time[NORMAL_SIZE];       // server time
char msg[BUF_SIZE];                  // msg
char server_port[NORMAL_SIZE];       // server port number
char clnt_ip[NORMAL_SIZE];           // client ip address
char buf[100], command[5], filename[MAX_SIZE], temp[20], *f;
int filehandle, k, size, status;

int chat_sock;
int ftp_sock;
struct sockaddr_in serv_addr;
pthread_t snd_thread, rcv_thread;
void *thread_return;

int main(int argc, char *argv[])
{
    check_arg(argc, argv);
    print_time();
    chat_socket(argv);
    ftp_sock = ftp_socket(argv[1], atoi(argv[2]));
    if (ftp_sock == -1)
    {
        printf("ftp connection fail\n");
        exit(1);
    }
    menu();
    chat();
    return 0;
}

void check_arg(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf(" Usage : %s <ip> <port> <name>\n", argv[0]);
        exit(1);
    }
}

void chat()
{
    pthread_create(&snd_thread, NULL, send_msg, (void *)&chat_sock);
    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&chat_sock);
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);
    close(chat_sock);
}

void chat_socket(char *argv[])
{
    sprintf(name, "[%s]", argv[3]);
    sprintf(clnt_ip, "%s", argv[1]);
    sprintf(server_port, "%s", argv[2]);
    chat_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(chat_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling(" conncet() error");
}

int ftp_socket(char *servip, unsigned short port)
{
    struct sockaddr_in servaddr;
    int s;
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, servip, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);

    if (connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        return -1;
    return s;
}
void print_time()
{
    struct tm *t;
    time_t timer = time(NULL);
    t = localtime(&timer);
    sprintf(server_time, "%d-%d-%d %d:%d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
}

void *send_msg(void *arg)
{
    int sock = *((int *)arg);
    char name_msg[NORMAL_SIZE + BUF_SIZE];
    char myInfo[BUF_SIZE];
    char *who = NULL;
    char temp[BUF_SIZE];

    printf(" >> join the chat !! \n");
    sprintf(myInfo, "%s's join. IP_%s\n", name, clnt_ip);
    write(sock, myInfo, strlen(myInfo));

    while (1)
    {
        fgets(msg, BUF_SIZE, stdin);

        if (!strcmp(msg, "!menu\n"))
        {
            menuOptions();
            continue;
        }

        else if (!strcmp(msg, "!file\n"))
        {
            fileoption();
            continue;
        }

        else if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
        {
            close(sock);
            exit(0);
        }

        // send message
        sprintf(name_msg, "%s %s", name, msg);
        write(sock, name_msg, strlen(name_msg));
    }
    return NULL;
}

void *recv_msg(void *arg)
{
    int sock = *((int *)arg);
    char name_msg[NORMAL_SIZE + BUF_SIZE];
    int str_len;

    while (1)
    {
        str_len = read(sock, name_msg, NORMAL_SIZE + BUF_SIZE - 1);
        if (str_len == -1)
            return (void *)-1;
        name_msg[str_len] = 0;
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
    switch (select)
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
    printf(" **** Chat Client ****\n");
    printf(" server port : %s \n", server_port);
    printf(" client IP   : %s \n", clnt_ip);
    printf(" chat name   : %s \n", name);
    printf(" server time : %s \n", server_time);
    printf(" ************* menu ***************\n");
    printf(" if you want to select menu -> !menu\n");
    printf(" 1. change name\n");
    printf(" 2. clear/update\n");
    printf(" if you want to send file -> !file\n");
    printf(" **********************************\n");
    printf(" Exit -> q & Q\n\n");
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void fileoption()
{
    while (1)
    {
        char bufmsg[MAX_SIZE];
        printf("\n\t**** ftp command ****\n");
        printf("\tget : download file\n");
        printf("\tput : upload file\n");
        printf("\tpwd : current path\n");
        printf("\tls : list files in current path\n");
        printf("\tcd : change directory\n");
        printf("\tq : exit ftp\n\n");
        printf("\tother key will be ignored");
        printf("\n\t*******************");
        printf("\n\t>> ");
        fgets(bufmsg, MAX_SIZE, stdin);

        if (!strcmp(bufmsg, "get\n"))
            get();
        else if (!strcmp(bufmsg, "put\n"))
            put();
        else if (!strcmp(bufmsg, "pwd\n"))
            pwd();
        else if (!strcmp(bufmsg, "ls\n"))
            ls();
        else if (!strcmp(bufmsg, "cd\n"))
            cd();
        else if (!strcmp(bufmsg, "q\n") || !strcmp(bufmsg, "Q\n"))
        {
            strcpy(buf, "quit");
            send(ftp_sock, buf, 100, 0);
            recv(ftp_sock, &status, 100, 0);
            if (status)
            {
                printf("exit ftp service..\n");
                exit(0);
            }
            printf("fail to exit ftp service\n");
        }
    }
}

void get()
{
    printf("filename to download : ");
    scanf("%s", filename);
    fgets(temp, MAX_SIZE, stdin);
    strcpy(buf, "get ");
    strcat(buf, filename);
    send(ftp_sock, buf, 100, 0); //명령어 전송
    recv(ftp_sock, &size, sizeof(int), 0);
    if (!size)
    {
        printf("there is no file\n");
        return;
    }
    f = malloc(size);
    recv(ftp_sock, f, size, 0);
    while (1)
    {
        filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
        if (filehandle == -1) //같은 이름이 있다면 이름 끝에 _1 추가
            sprintf(filename + strlen(filename), "_1");
        else
            return;
    }
    write(filehandle, f, size);
    close(filehandle);
    printf("download finished\n");
}

void put()
{
    struct stat obj;
    printf("filename to upload : ");
    scanf("%s", filename);
    fgets(temp, MAX_SIZE, stdin);
    filehandle = open(filename, O_RDONLY);
    if (filehandle == -1)
    {
        printf("there is no file name %s\n", filename);
        return;
    }
    strcpy(buf, "put ");
    strcat(buf, filename);
    send(ftp_sock, buf, 100, 0);
    stat(filename, &obj);
    size = obj.st_size;
    send(ftp_sock, &size, sizeof(int), 0);
    sendfile(ftp_sock, filehandle, NULL, size);
    recv(ftp_sock, &status, sizeof(int), 0);
    if (status)
        printf("upload success\n");
    else
        printf("upload fail\n");
}

void pwd()
{
    strcpy(buf, "pwd");
    send(ftp_sock, buf, 100, 0);
    recv(ftp_sock, buf, 100, 0);
    printf("--The path of the Directory--\n%s", buf);
}

void ls()
{
    strcpy(buf, "ls");
    send(ftp_sock, buf, 100, 0);
    recv(ftp_sock, &size, sizeof(int), 0);
    f = malloc(size);
    recv(ftp_sock, f, size, 0);
    filehandle = creat("temp.txt", O_WRONLY);
    write(filehandle, f, size);
    close(filehandle);
    printf("--The Remote Directory List--\n");
    system("cat temp.txt");
}

void cd()
{
    strcpy(buf, "cd ");
    printf("directory path to move : ");
    scanf("%s", buf + 3);
    fgets(temp, MAX_SIZE, stdin);
    send(ftp_sock, buf, 100, 0);
    recv(ftp_sock, &status, sizeof(int), 0);
    if (status)
        printf("changed directory\n");
    else
        printf("faile to change directory\n");
}