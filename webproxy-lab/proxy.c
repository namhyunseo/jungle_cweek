#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_KEY 512

#include "csapp.h"
/* You won't lose style points for including this long line in your code */
void doit(int fd);
void parse_uri(char *uri, char *hostname, char *path, int *port);
void read_requesthdrs(rio_t *rp, char *extrahdr);
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
  
void *thread(void *vargp);

typedef struct node_t{
  char key[MAX_KEY];
  char *data;
  size_t len;
  struct node_t *prev, *next;
} node_t;

typedef struct cache_t{
  size_t total;
  node_t *head, *tail;
} cache_t;

node_t* doyoucached(char *uri);
void caching(char *uri, char *buf, int total);
void dellru();
void updlru(node_t *node);

// 캐시 초기화
cache_t cache_list = { .total=0, .head=NULL, .tail=NULL};
/******************************************/
int main(int args, char **argv)
{
  int listenfd, *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  if(args != 2){
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  // 15214포트로 듣는 중
  listenfd = Open_listenfd(argv[1]);
  printf("listening... \n");

  while(1){
    clientlen = sizeof(struct sockaddr_storage);
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
    Pthread_create(&tid, NULL, thread, connfdp);
  }
}

void *thread(void *vargp)
{
  int connfd = *((int*)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);
  printf("server connected\n");
  doit(connfd);
  Close(connfd);
  return NULL;
}

// 입력 : 클라이언트와 연결된 소켓 fd
// 출력 : 웹 서버가 준 결과
void doit(int fd){ 
  // 요청 라인을 읽고
  // URI 파싱하고
  // TCP 연결하고
  // HTTP 메시지 만들고
  // 서버에게 보내고
  // 서버 응답 받아서 보낸다.
  
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char bufs[MAXLINE], host[MAXLINE], path[MAXLINE], bufc[MAXLINE], extrahdr[MAXLINE];
  rio_t rio;
  rio_t rios;
  int port;

  // read request line
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE); // buf에 요청 라인 있다!
  sscanf(buf, "%s %s %s", method, uri, version); // 받아왔으니깐 변수에 할당해줘야지
  if(strcasecmp(method, "GET")) {
    return; // Not Get, Get out
  }
  if(!strstr(uri, "http://")){ // strstr : 부분 문자열 검색
    printf("no original addr \n");
  }
  read_requesthdrs(&rio, extrahdr); //헤더 읽기
  printf("1\n");

  // 캐시에 있는지 확인 -> uri와 일치하는 key가 있는지 확인
  node_t *hit;
  if( (hit =  doyoucached(uri))){
    Rio_writen(fd, hit->data, hit->len);
    updlru(hit);
    return; //스레드 종료
  }

  parse_uri(uri, host, path, &port);
  printf("2\n");


  if(port == 80){
    sprintf(bufs,
      "GET %s HTTP/1.0\r\n"
      "Host: %s\r\n"
      "%s"
      "Connection: close\r\n"
      "Proxy-Connection: close\r\n"
      "\r\n",
      path, host, user_agent_hdr
    );
  }
  else{
    sprintf(bufs,
      "GET %s HTTP/1.0\r\n"
      "Host: %s:%d\r\n"
      "%s"
      "Connection: close\r\n"
      "Proxy-Connection: close\r\n"
      "\r\n",
      path, host, port, user_agent_hdr
    ); 
  }

  // 웹 서버 연결
  char portstr[16];
  sprintf(portstr, "%d", port);
  int svrfd = Open_clientfd(host, portstr);

  // 웹 서버에 데이터 전달
  Rio_readinitb(&rios, svrfd);
  // printf("to server \n");
  // printf("%s", bufs);
  Rio_writen(svrfd, bufs, strlen(bufs));
  // Rio_readlineb(&rios, bufc, MAXLINE);

  // 클라이언트에게 보내고 캐싱
  ssize_t n;
  size_t total = 0;
  int cacheable = 1;
  char *obj = Malloc(MAX_OBJECT_SIZE);
  while ((n = Rio_readnb(&rios, bufc, MAXLINE)) > 0) {
    // printf("%s",bufc);
    Rio_writen(fd, bufc, n);   // 클라이언트에게 전달

    if(cacheable){ // 캐시 객체에 들어갈 수 있는지 판단
      if(total + (size_t)n <= MAX_OBJECT_SIZE){
        memcpy(obj+total, bufc, (size_t)n);
        total += (size_t)n;
      }else{
        cacheable = 0;
      }
    }
  }
  if(!cacheable) Free(obj);

  // 캐시 용량이 가득 찼을 경우도 고려해서 LRU기반 캐시 용량 확보 후 다시 캐싱
  if(cache_list.total + total > MAX_CACHE_SIZE){
    dellru();
  }

  if(cacheable && (total > 0)){
    caching(uri, obj, total);
  }
  Free(obj);
  Close(svrfd);
}

/**
 * LRU -> 사용되는 순간 리스트의 가장 앞으로 이동
 * 용량 초과로 삭제할 때는 LRU tail 삭제
 */

void updlru(node_t *node){
  // 사용되면 가장 위로 옮긴다.
  if(cache_list.head == node) return;

  if(node->next != NULL){
    node->prev->next = node->next;
    node->next->prev = node->prev;
  }
  else if(node->next == NULL){
    cache_list.head->prev = node;
    node->next = cache_list.head;
    cache_list.tail = node->next;
    cache_list.head = node;
  }
}

void dellru(){
  // 가장 마지막에 있는 노드 삭제
  node_t *del = cache_list.tail;
  cache_list.tail = del->prev;
  Free(del);
}

node_t* doyoucached(char *uri){
  node_t *p = cache_list.head;
  for (; p; p = p->next) {
    if(strcmp(uri, p->key)==0){
      return p;
    };
  };
  return NULL;
}

void caching(char *uri, char *buf, int total){
  node_t *new = Malloc(sizeof(node_t));
  if(!new) return;
  // 새로운 노드 생성
  // Key, data 값 추가
  // 링크에 연결
  size_t ulen = strnlen(uri, MAXLINE-1); //uri 길이 측정
  memcpy(new->key, uri, ulen);
  new->key[ulen] = '\0'; //?

  new->data = malloc(total);
  if(!new->data){free(new); return;}
  memcpy(new->data, buf, total);
  new->len = total;
  
  // new->last_use = 
  new->prev = NULL;
  new->next = NULL;

  if(cache_list.head == NULL){
    cache_list.head = cache_list.tail = new;
  }else{
    new->next = cache_list.head;
    // cache_list.head->prev = new;
    cache_list.head->prev = new;
    cache_list.head = new;
  }

  cache_list.total += total;
}

void read_requesthdrs(rio_t *rp, char *extrahdr)
{
  char buf[MAXLINE];
  extrahdr[0] = '\0';
  while(1){
    if(Rio_readlineb(rp, buf, MAXLINE) <= 0) break;
    if(!strcmp(buf, "\r\n")) break;

    if (!strncasecmp(buf, "Host:", 5) ||
        !strncasecmp(buf, "User-Agent:", 11) ||
        !strncasecmp(buf, "Connection:", 11) ||
        !strncasecmp(buf, "Proxy-Connection:", 17)) {
      continue;
    }
    size_t elen = strlen(extrahdr), blen = strlen(buf);
    if(elen + blen < MAXLINE*8 - 1){
      memcpy(extrahdr + elen, buf, blen);
      extrahdr[elen + blen] = '\0';
    }
  }
  return;
}

/** 절대 경로가 들어올 경우 */
void parse_uri(char *uri, char *hostname, char *path, int *port)
{
  char *p = uri;
  if (!strncmp(p, "http://", 7)) p += 7;
  const char *slash = strchr(p, '/'); //strchr : 문자열에서 원하는 문자의 포인터 찾아내기
  if(slash){
    size_t len = (size_t)(slash-p);
    strncpy(hostname, p, len);
    hostname[len] = '\0';
    sprintf(path, "%s", slash);
  }else{
    sprintf(hostname, "%s", p);
    sprintf(path, "/");
  }

  // 호스트랑 포트랑 분리
  char *col = strchr(hostname, ':');
  if(col){
    *col = '\0';
    *port = atoi(col + 1);
    // printf("%s \n", *port);
  }else{
    *port = 80;
  }
}