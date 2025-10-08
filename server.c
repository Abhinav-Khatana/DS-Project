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
/* JSON builders */
char* json_escape(char *dst, const char *src) {
    char *p=dst;
    while(*src) {
        if(*src=='"'||*src=='\\') *p++='\\';
        *p++=*src++;
    }
    *p=0;
    return dst;
}

char* build_json(const char *query) {
    static char buf[BUFSIZE], esc[1024];
    char *p=buf;
    
    if(!query) {  /* History list */
        p+=sprintf(p,"[");
        for(Node *n=head; n; n=n->next) {
            if(n!=head) *p++=',';
            p+=sprintf(p,"{\"url\":\"%s\",\"time\":\"%s\",\"current\":%s}",
                json_escape(esc,n->url), n->time, n==current?"true":"false");
        }
        p+=sprintf(p,"]");
    } else {  /* Search results */
        p+=sprintf(p,"{\"query\":\"%s\",\"results\":[", json_escape(esc,query));
        int first=1;
        for(Node *n=head; n; n=n->next) {
            if(strstr(n->url,query)) {
                if(!first) *p++=','; first=0;
                p+=sprintf(p,"{\"url\":\"%s\",\"time\":\"%s\",\"count\":%d}",
                    json_escape(esc,n->url), n->time, get_count(n->url));
            }
        }
        p+=sprintf(p,"]}");
    }
    return buf;
}

/* URL decode */
void urldecode(char *dst, const char *src) {
    while(*src) {
        if(*src=='%' && src[1] && src[2]) {
            char h[3]={src[1],src[2],0};
            *dst++=strtol(h,NULL,16); src+=3;
        } else if(*src=='+') {*dst++=' '; src++;}
        else *dst++=*src++;
    }
    *dst=0;
}

/* Static file server */
void serve_file(SOCKET c, const char *path) {
    const char *files[][2] = {
        {"index.html", "text/html"},
        {"app.js", "application/javascript"},
        {"styles.css", "text/css"}
    };
    
    if(!strcmp(path,"/")) path="/index.html";
    
    for(int i=0; i<3; i++) {
        if(!strcmp(path+1, files[i][0])) {
            FILE *f=fopen(files[i][0],"rb");
            if(f) {
                fseek(f,0,SEEK_END); long sz=ftell(f); rewind(f);
                char hdr[256];
                sprintf(hdr,"HTTP/1.1 200 OK\r\nContent-Type:%s\r\nContent-Length:%ld\r\n\r\n",
                    files[i][1],sz);
                send(c,hdr,strlen(hdr),0);
                char buf[4096]; size_t r;
                while((r=fread(buf,1,sizeof(buf),f))>0) send(c,buf,r,0);
                fclose(f);
                return;
            }
        }
    }
    send(c,"HTTP/1.1 404\r\nContent-Length:9\r\n\r\nNot Found",45,0);
}

/* API handler */
void handle_api(SOCKET c, const char *method, const char *path, const char *body) {
    char resp[BUFSIZE], hdr[256];
    
    if(!strcmp(method,"GET") && !strcmp(path,"/api/history")) {
        strcpy(resp, build_json(NULL));
    }
    else if(!strcmp(method,"GET") && !strncmp(path,"/api/search?q=",14)) {
        char query[256]={0};
        urldecode(query, path+14);
        strcpy(resp, build_json(query));
    }
    else if(!strcmp(method,"POST")) {
        if(!strcmp(path,"/api/visit") && body) {
            char url[512]={0};
            if(strstr(body,"url=")) {
                urldecode(url, strstr(body,"url=")+4);
                add_history(url);
                strcpy(resp,"{\"status\":\"ok\"}");
            } else strcpy(resp,"{\"error\":\"no url\"}");
        }
        else if(!strcmp(path,"/api/back")) {
            go_back(); strcpy(resp,"{\"status\":\"ok\"}");
        }
        else if(!strcmp(path,"/api/forward")) {
            go_forward(); strcpy(resp,"{\"status\":\"ok\"}");
        }
        else if(!strcmp(path,"/api/clear")) {
            clear_all(); strcpy(resp,"{\"status\":\"cleared\"}");
        }
        else strcpy(resp,"{\"error\":\"unknown\"}");
    }
    else strcpy(resp,"{\"error\":\"method\"}");
    
    sprintf(hdr,"HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nContent-Length:%d\r\n\r\n",
        strlen(resp));
    send(c,hdr,strlen(hdr),0);
    send(c,resp,strlen(resp),0);
}
/* Main server */
int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
    
    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    int opt=1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    
    struct sockaddr_in addr={0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if(bind(server,(struct sockaddr*)&addr,sizeof(addr))<0 || listen(server,10)<0) {
        printf("Failed to start server\n");
        return 1;
    }
    
    printf("Server running at http://localhost:%d\n", PORT);
    
    while(1) {
        SOCKET client = accept(server, NULL, NULL);
        if(client == INVALID_SOCKET) continue;
        
        char buf[BUFSIZE];
        int len = recv(client, buf, sizeof(buf)-1, 0);
        if(len<=0) {closesocket(client); continue;}
        buf[len]=0;
        
        /* Parse request */
        char method[16], path[256];
        sscanf(buf, "%15s %255s", method, path);
        
        /* Handle OPTIONS (CORS) */
        if(!strcmp(method,"OPTIONS")) {
            send(client,"HTTP/1.1 200 OK\r\nContent-Length:0\r\n\r\n",39,0);
        }
        /* API calls */
        else if(!strncmp(path,"/api",4)) {
            char *body = strstr(buf,"\r\n\r\n");
            if(body) body+=4;
            handle_api(client, method, path, body);
        }
        /* Static files */
        else serve_file(client, path);
        
        closesocket(client);
    }
    
    closesocket(server);
    WSACleanup();
    return 0;
}
