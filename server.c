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
/* Helper functions */
unsigned hash(const char *s) {
    unsigned h=5381;
    while(*s) h=h*33+*s++;
    return h%101;
}

void push(Stack **s, Node *n) {
    Stack *new=malloc(sizeof(Stack));
    new->page=n; new->next=*s; *s=new;
}

Node* pop(Stack **s) {
    if(!*s) return NULL;
    Stack *t=*s; Node *n=t->page;
    *s=t->next; free(t);
    return n;
}

void inc_visit(const char *url) {
    unsigned h=hash(url);
    Visit *v=visits[h];
    while(v) {
        if(!strcmp(v->url,url)) {v->count++; return;}
        v=v->next;
    }
    v=malloc(sizeof(Visit));
    strcpy(v->url,url); v->count=1;
    v->next=visits[h]; visits[h]=v;
}

int get_count(const char *url) {
    Visit *v=visits[hash(url)];
    while(v) {
        if(!strcmp(v->url,url)) return v->count;
        v=v->next;
    }
    return 0;
}
