#include <stdbool.h>

#ifndef LIST
#define LIST

typedef struct ClientNode
{
    int data;
    struct ClientNode *prev;
    struct ClientNode *link;
    char ip[16];
    char name[31];
    bool seen[100];
} ClientList;

ClientList *newClient(int sockfd, char *ip)
{
    ClientList *newList = (ClientList *)malloc(sizeof(ClientList));
    newList->data = sockfd;
    newList->prev = NULL;
    newList->link = NULL;
    strncpy(newList->ip, ip, 16);
    strncpy(newList->name, "NULL", 5);

    int i;
    for (i = 0; i < 1000; i++)
    {
        newList->seen[i] = false;
    }

    return newList;
}

struct Announcement
{
    int sockfd;
    char topic[30];
    char desc[100];
    char date[100];
    int time;
};

#endif