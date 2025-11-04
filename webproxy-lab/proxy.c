#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#include "csapp.h"
/* You won't lose style points for including this long line in your code */
void doit(int fd);
void parse_uri(char *uri, char *hostname, char *path, int *port);
void read_requesthdrs(rio_t *rp);
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int args, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  if(args != 2){
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  listenfd = Open_listenfd(argv[1]);

  while(1){
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);    
    printf("Accepted connection from %s, %s\n", hostname, port);
    doit(connfd); // 클라이언트의 요청을 받고, 서버에게 전달
    Close(connfd);
  }
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
  char bufs[MAXLINE], host[MAXLINE], path[MAXLINE], bufc[MAXLINE];
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
  read_requesthdrs(&rio); //헤더 읽기

  printf("parsing.... \n");
  // 파싱
  parse_uri(uri, host, path, &port);
  printf("parsing done.... \n");
  // HTTP message

  sprintf(bufs,
    "GET %s HTTP/1.0\r\n"
    "Host: %s\r\n"
    "%s"
    "Connection: close\r\n"
    "Proxy-Connection: close\r\n"
    "\r\n",
    path, host, user_agent_hdr
  );
  char portstr[16];
  sprintf(portstr, "%d", port);
  printf("%c %s", portstr, host);
  int svrfd = Open_clientfd(host, portstr);
  Rio_readinitb(&rios, svrfd);
  printf("server connect \r\n");
  Rio_writen(svrfd, bufs, strlen(bufs));
  printf("server connected \r\n");
  Rio_readlineb(&rios, bufc, MAXLINE);
  Rio_writen(fd, bufc, strlen(bufc));
  Close(svrfd);
}




void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

/** 절대 경로가 들어올 경우 */
void parse_uri(char *uri, char *hostname, char *path, int *port)
{
  // port 없으면 80 처리
  // uri 해체
  // http:// 제외하고
  // 그 다음 slash 만날 때 까지 = hostname
  // 그 다음 전체를 pat
  // 만약 :가 있으면 port 저장
  // *port = 80;
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