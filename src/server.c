#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "proto.h"
#include "server.h"

#define MAX_NUMBER_OF_ANNOUNCEMENTS 100

int leave_flag = 0;

int server_sockfd = 0, client_sockfd = 0;
ClientList *client_root, *client_now;

int currentNumberOfAnnouncements = 0;
struct Announcement announcements[MAX_NUMBER_OF_ANNOUNCEMENTS];

void catch_ctrl_c_and_exit(int sig)
{
    ClientList *tmp;
    while (client_root != NULL)
    {
        printf("\nClose socketfd: %d\n", client_root->data);
        close(client_root->data);
        tmp = client_root;
        client_root = client_root->link;
        free(tmp);
    }
    printf("Bye\n");
    exit(EXIT_SUCCESS);
}

void send_to_all_clients(ClientList *np, char tmp_buffer[])
{
    ClientList *tmp = client_root->link;
    while (tmp != NULL)
    {
        if (np->data != tmp->data)
        {

            printf("Send to sockfd %d: \"%s\" \n", tmp->data, tmp_buffer);
            printf("send\n");
            send(tmp->data, tmp_buffer, LENGTH_SEND, 0);
        }
        tmp = tmp->link;
    }
}

char *findUserName(sockfd)
{
    char name[20] = "nieznany";
    ClientList *tmp = client_root->link;
    while (tmp != NULL)
    {
        if (tmp->data == sockfd)
        {
            strncpy(name, tmp->name, 20);
        }
        tmp = tmp->link;
    }

    return name;
}

void time_handler()
{
    while (1)
    {
        if (leave_flag)
        {
            break;
        }
    }
}

void sendAnnouncement(struct Announcement helper, int sockfd, int counter)
{
    char label[20] = "";
    char formattedTopic[40] = "";
    char formattedDesc[120] = "";
    char formattedAuthor[40] = "";
    char formattedDate[100] = "";
    char formattedTime[100] = "";

    sprintf(label, "Ogloszenie nr %d", counter + 1);
    send(sockfd, label, LENGTH_SEND, 0);

    sprintf(formattedTopic, "• tytul: %s", helper.topic);
    send(sockfd, formattedTopic, LENGTH_SEND, 0);

    sprintf(formattedDesc, "• tresc: %s", helper.desc);
    send(sockfd, formattedDesc, LENGTH_SEND, 0);

    sprintf(formattedDate, "• dodane: %s", helper.date);
    send(sockfd, formattedDate, LENGTH_SEND, 0);

    sprintf(formattedAuthor, "• autor: %s", findUserName(helper.sockfd));
    send(sockfd, formattedAuthor, LENGTH_SEND, 0);

    sprintf(formattedTime, "• wygasnie za: %d s", helper.time);
    send(sockfd, formattedTime, LENGTH_SEND, 0);

    send(sockfd, "", LENGTH_SEND, 0);
}

void client_handler(void *p_client)
{

    char nickname[LENGTH_NAME] = {};
    char recv_buffer[LENGTH_MSG] = {};
    char send_buffer[LENGTH_SEND] = {};
    ClientList *np = (ClientList *)p_client;
    int counter;

    if (recv(np->data, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME - 1)
    {
        printf("%s didn't input name.\n", np->ip);
        leave_flag = 1;
    }
    else
    {
        strncpy(np->name, nickname, LENGTH_NAME);
        printf("%s(%s)(%d) join the notice board.\n", np->name, np->ip, np->data);
        sprintf(send_buffer, "❕%s join the notice board.\n", np->name);
        send_to_all_clients(np, send_buffer);
    }

    while (1)
    {
        if (leave_flag)
        {
            break;
        }
        int receive = recv(np->data, recv_buffer, LENGTH_MSG, 0);
        if (receive > 0)
        {
            if (strlen(recv_buffer) == 0)
            {
                continue;
            }
            int command = atoi(recv_buffer);
            switch (command)
            {
            case 1: // lista ogloszen

                send(np->data, "", LENGTH_SEND, 0);
                if (currentNumberOfAnnouncements > 0)
                {
                    for (counter = 0; counter < currentNumberOfAnnouncements; counter++)
                    {
                        struct Announcement helper = announcements[counter];

                        sendAnnouncement(helper, np->data, counter);
                        np->seen[counter] = true;
                    }
                }
                else
                {
                    send(np->data, "Brak ogloszen\n", LENGTH_SEND, 0);
                }

                break;
            case 2:
                sprintf(send_buffer, "zobacz nowe ogloszenia");

                send(np->data, "", LENGTH_SEND, 0);
                if (currentNumberOfAnnouncements > 0)
                {
                    int helper_counter = 0;
                    for (counter = 0; counter < currentNumberOfAnnouncements; counter++)
                    {
                        if (!np->seen[counter])
                        {
                            helper_counter++;
                            struct Announcement helper = announcements[counter];
                            sendAnnouncement(helper, np->data, counter);

                            np->seen[counter] = true;
                        }
                    }
                    if (helper_counter == 0)
                    {
                        send(np->data, "Brak nowych ogloszen\n", LENGTH_SEND, 0);
                    }
                }
                else
                {
                    send(np->data, "Brak ogloszen\n", LENGTH_SEND, 0);
                }
                break;
            case 3: // dodaj ogloszenie
                announcements[currentNumberOfAnnouncements].sockfd = np->data;

                recv(np->data, recv_buffer, LENGTH_MSG, 0);
                strcpy(announcements[currentNumberOfAnnouncements].topic, recv_buffer);

                recv(np->data, recv_buffer, LENGTH_MSG, 0);
                strcpy(announcements[currentNumberOfAnnouncements].desc, recv_buffer);

                recv(np->data, recv_buffer, LENGTH_MSG, 0);
                int currentTime = atoi(recv_buffer);
                announcements[currentNumberOfAnnouncements].time = currentTime;

                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                char date[100];

                sprintf(date, "%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                strcpy(announcements[currentNumberOfAnnouncements].date, date);
                currentNumberOfAnnouncements++;

                sprintf(send_buffer, "\n✅ Dodano ogloszenie\n");
                send(np->data, send_buffer, LENGTH_SEND, 0);
                break;
            case 4:
                send(np->data, "", LENGTH_SEND, 0);
                if (currentNumberOfAnnouncements > 0)
                {
                    int helper_counter = 0;
                    for (counter = 0; counter < currentNumberOfAnnouncements; counter++)
                    {

                        struct Announcement helper = announcements[counter];

                        if (np->data == helper.sockfd)
                        {
                            sendAnnouncement(helper, np->data, counter);
                            helper_counter++;
                        }
                    }

                    if (helper_counter == 0)
                    {
                        send(np->data, "Brak ogloszen do usuniecia\n", LENGTH_SEND, 0);
                    }
                    else
                    {
                        recv(np->data, recv_buffer, LENGTH_MSG, 0);

                        int announcement_number = atoi(recv_buffer) - 1;

                        if (announcement_number < 0 || announcement_number > currentNumberOfAnnouncements)
                        {
                            send(np->data, "\n❌ Podana zla wartosc\n", LENGTH_SEND, 0);
                        }
                        else
                        {
                            if (np->data == announcements[announcement_number].sockfd)
                            {
                                for (counter = 0; counter < currentNumberOfAnnouncements - 1; counter++)
                                {
                                    if (counter >= announcement_number)
                                    {
                                        announcements[counter] = announcements[counter + 1];
                                    }
                                }
                                currentNumberOfAnnouncements--;

                                send(np->data, "\n✅ Usunieto ogloszenie\n\n", LENGTH_SEND, 0);
                            }
                            else
                            {
                                send(np->data, "\n❌ Podano nieprawidlowa wartosc\n\n", LENGTH_SEND, 0);
                            }
                        }
                    }
                }
                else
                {
                    send(np->data, "Brak ogloszen\n", LENGTH_SEND, 0);
                }
                break;
            default:
                sprintf(send_buffer, "nieznana komenda");
                break;
            }
            // sprintf(send_buffer, "%s：%s from %s", np->name, recv_buffer, np->ip);
        }
        else if (receive == 0 || strcmp(recv_buffer, "exit") == 0)
        {
            printf("%s(%s)(%d) leave the chatroom.\n", np->name, np->ip, np->data);
            sprintf(send_buffer, "%s(%s) leave the chatroom.", np->name, np->ip);
            leave_flag = 1;
        }
        else
        {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }
    }

    close(np->data);
    if (np == client_now)
    {
        client_now = np->prev;
        client_now->link = NULL;
    }
    else
    {
        np->prev->link = np->link;
        np->link->prev = np->prev;
    }
    free(np);
}

int main()
{
    signal(SIGINT, catch_ctrl_c_and_exit);

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd == -1)
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
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(8888);

    bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen);
    listen(server_sockfd, 5);

    printf("• getsockname\n\n");
    getsockname(server_sockfd, (struct sockaddr *)&server_info, (socklen_t *)&s_addrlen);
    printf("Start Server on: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));

    printf("• newClient\n\n");
    client_root = newClient(server_sockfd, inet_ntoa(server_info.sin_addr));
    client_now = client_root;

    printf("• while\n\n");
    while (1)
    {
        printf("• accept\n\n");
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_info, (socklen_t *)&c_addrlen);

        printf("Client %s:%d come in.\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

        ClientList *new_client = newClient(client_sockfd, inet_ntoa(client_info.sin_addr));
        new_client->prev = client_now;
        client_now->link = new_client;
        client_now = new_client;

        pthread_t client_handler_thread;
        if (pthread_create(&client_handler_thread, NULL, (void *)client_handler, (void *)new_client) != 0)
        {
            perror("Create client_handler_thread error!\n");
            exit(EXIT_FAILURE);
        }

        pthread_t time_handler_thread;
        if (pthread_create(&time_handler_thread, NULL, (void *)time_handler, NULL) != 0)
        {
            perror("Create time_handler_thread error!\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}