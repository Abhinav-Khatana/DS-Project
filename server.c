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
void add_history(const char *url) {
    /* Handle navigation */
    if(current) {
        push(&back_stack, current);
        /* Clear forward history if not at end */
        if(current->next) {
            Node *n=current->next;
            while(n) {Node *t=n->next; free(n); n=t;}
            current->next=NULL; tail=current;
        }
        while(forward_stack) pop(&forward_stack);
    }
    
    /* Create new node */
    Node *n=calloc(1,sizeof(Node));
    strcpy(n->url,url);
    time_t t=time(0);
    strftime(n->time,32,"%Y-%m-%d %H:%M:%S",localtime(&t));
    
    /* Add to list */
    if(!head) head=tail=n;
    else {tail->next=n; n->prev=tail; tail=n;}
    current=n;
    inc_visit(url);
}

void go_back() {
    Node *n=pop(&back_stack);
    if(n) {push(&forward_stack,current); current=n;}
}

void go_forward() {
    Node *n=pop(&forward_stack);
    if(n) {push(&back_stack,current); current=n;}
}

void clear_all() {
    while(head) {Node *t=head->next; free(head); head=t;}
    while(back_stack) pop(&back_stack);
    while(forward_stack) pop(&forward_stack);
    for(int i=0;i<101;i++) {
        Visit *v=visits[i];
        while(v) {Visit *t=v->next; free(v); v=t;}
        visits[i]=NULL;
    }
    head=tail=current=NULL;
}
