#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "proto.h"
#include "string.h"

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char nickname[LENGTH_NAME] = {};

char message[LENGTH_MSG] = {};

void catch_ctrl_c_and_exit()
{
    flag = 1;
}

void show_menu()
{
    printf("\n");
    printf("Dostepne akcje:\n");
    printf("1. Zobacz liste ogloszen\n");
    printf("2. Zobacz nowe ogloszenia\n");
    printf("3. Dodaj ogloszenie\n");
    printf("4. Usun ogloszenie\n");
    printf("\n");
}

void recv_msg_handler()
{
    while (1)
    {
        char receiveMessage[LENGTH_SEND] = {};
        int receive = recv(sockfd, receiveMessage, LENGTH_SEND, 0);
        if (receive > 0)
        {
            printf("\r%s\n", receiveMessage);
            str_overwrite_stdout();
        }
        else if (receive == 0)
        {
            break;
        }
    }
}

void send_msg_handler()
{
    show_menu();
    memset(message, 0, sizeof(message));
    while (1)
    {

        while (fgets(message, LENGTH_MSG, stdin) != NULL)
        {
            str_trim_lf(message, LENGTH_MSG);
            if (strlen(message) != 0)
            {
                break;
            }
        }
        int command = atoi(message);
        switch (command)
        {
        case 1:
            send(sockfd, "1", LENGTH_MSG, 0);
            break;
        case 2:
            send(sockfd, "2", LENGTH_MSG, 0);
            break;
        case 3:
            send(sockfd, "3", LENGTH_MSG, 0);
            char topic[30] = "";
            char desc[100] = "";
            char time[100] = "";

            printf("\nPodaj nazwe ogloszenia: ");
            while (fgets(topic, LENGTH_MSG, stdin) != NULL)
            {
                str_trim_lf(topic, LENGTH_MSG);
                if (strlen(topic) != 0)
                {
                    break;
                }
            }
            send(sockfd, topic, LENGTH_MSG, 0);

            printf("\nPodaj tresc ogloszenia: ");
            while (fgets(desc, LENGTH_MSG, stdin) != NULL)
            {
                str_trim_lf(desc, LENGTH_MSG);
                if (strlen(desc) != 0)
                {
                    break;
                }
            }
            send(sockfd, desc, LENGTH_MSG, 0);

            printf("\nPodaj czas wygasniecia (sekundy): ");
            while (fgets(time, LENGTH_MSG, stdin) != NULL)
            {
                str_trim_lf(time, LENGTH_MSG);
                if (strlen(time) != 0)
                {
                    break;
                }
            }
            send(sockfd, time, LENGTH_MSG, 0);

            break;
        case 4:
            send(sockfd, "4", LENGTH_MSG, 0);
            char answer[30] = "";
            usleep(200);
            printf("\nPodaj numer ogloszenia do usuniecia: ");
            while (fgets(answer, LENGTH_MSG, stdin) != NULL)
            {
                str_trim_lf(answer, LENGTH_MSG);
                if (strlen(answer) != 0)
                {
                    break;
                }
            }
            send(sockfd, answer, LENGTH_MSG, 0);
            break;
        default:
            break;
        }

        if (strcmp(message, "exit") == 0)
        {
            break;
        }
    }
    catch_ctrl_c_and_exit();
}

int main()
{
    signal(SIGINT, catch_ctrl_c_and_exit);
    printf("-- Witaj w Tablicy Ogłoszeń --\n\n");
    printf("Podaj swoje imie: ");
    if (fgets(nickname, LENGTH_NAME, stdin) != NULL)
    {
        str_trim_lf(nickname, LENGTH_NAME);
    }
    if (strlen(nickname) < 2)
    {
        printf("\nImie musi miec wiecej niz 2 znaki\n");
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("Fail to create a socket.");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_info, client_info;
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    memset(&server_info, 0, s_addrlen);
    memset(&client_info, 0, c_addrlen);
    server_info.sin_family = PF_INET;
    server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_info.sin_port = htons(8888);

    int err = connect(sockfd, (struct sockaddr *)&server_info, s_addrlen);
    if (err == -1)
    {
        printf("Connection to Server error!\n");
        exit(EXIT_FAILURE);
    }

    // obsluga bledu
    getsockname(sockfd, (struct sockaddr *)&client_info, (socklen_t *)&c_addrlen);
    getpeername(sockfd, (struct sockaddr *)&server_info, (socklen_t *)&s_addrlen);
    printf("Connect to Server: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));
    printf("You are: %s:%d\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));
    send(sockfd, nickname, LENGTH_NAME, 0);

    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0)
    {
        printf("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }

    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0)
    {
        printf("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }

    // obsluz beldy
    pthread_join(&send_msg_thread, NULL);
    pthread_join(&recv_msg_thread, NULL);

    while (1)
    {
        pthread_join(&send_msg_thread, NULL);
        pthread_join(&recv_msg_thread, NULL);

        if (flag)
        {
            printf("\nBye\n");
            break;
        }
    }

    close(sockfd);
    return 0;
}