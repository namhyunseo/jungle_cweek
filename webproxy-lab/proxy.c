#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#include "csapp.h"
/* You won't lose style points for including this long line in your code */
void doit(int fd);
void parse_uri(char *uri, char *hostname, char *path, int *port);
void read_requesthdrs(rio_t *rp, char *extrahdr);
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
  
void *thread(void *vargp);

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
  listenfd = Open_listenfd(argv[1]);

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
  int *port;

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

  parse_uri(uri, host, path, &port);

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
  ssize_t n;
  while ((n = Rio_readnb(&rios, bufc, MAXLINE)) > 0) {
    // printf("%s",bufc);
    Rio_writen(fd, bufc, n);   // 클라이언트에게 전달
  }
  Close(svrfd);
}




void read_requesthdrs(rio_t *rp, char *extrahdr)
{
  char buf[MAXLINE];
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