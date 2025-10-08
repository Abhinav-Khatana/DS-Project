#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFSIZE 65536

/* History node */
typedef struct Node {
    char url[512], time[32];
    struct Node *prev, *next;
} Node;

/* Stack for back/forward */
typedef struct Stack {
    Node *page;
    struct Stack *next;
} Stack;

/* Visit counter */
typedef struct Visit {
    char url[512];
    int count;
    struct Visit *next;
} Visit;

/* Globals */
Node *head=NULL, *tail=NULL, *current=NULL;
Stack *back_stack=NULL, *forward_stack=NULL;
Visit *visits[101]={0};