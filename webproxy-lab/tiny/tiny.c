/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // main routine
  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept₩
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}
/**
 * todo
 * [v] doit
 * [v] Http transaction 처리
 * [v] 오류 응답 생성
 * [v] 요청 헤더 읽기
 * [v] URI 분석기
 * [v] 정적 콘텐츠 생성기
 * [v] 동적 콘텐츠 생성기
 * 
 * 우선은 따라 적기만, 이후에 주석달면서 분석
 * 따라 적으면서 흐름 잡기
 */

void doit(int fd)
{
  /**
   * 1.	요청 라인 읽기 (rio_readlineb)
   * 2.	요청 메서드 검사 (GET만 지원)
   * 3.	요청 헤더 읽고 무시
   * 4.	URI 파싱 (정적 or 동적 콘텐츠 판별)
   * 5.	파일 유효성 검사
   * 6.	정적 요청이면 serve_static() 호출
   * 7.	동적 요청이면 serve_dynamic() 호출
   */
  
  // 변수 설정
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;
  
  // Rio 요청 라인 확인
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers :\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); //입력이 이렇게 들어오나보네
  if(strcasecmp(method, "GET")){
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  // parse URI
  is_static = parse_uri(uri, filename, cgiargs); //dyn : 0, static : 1
  // URI에서 파일 가져와서 유효성 검증
  if(stat(filename, &sbuf) < 0){
    clienterror(fd, filename, "404", "NOT found", "Tiny couldn't find this file");
    return;
  }

  if(is_static){
    // 파일에 접근 가능한지
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read this file");
      return;
    }
    // static 처리 실행
    serve_static(fd, filename, sbuf.st_size);
  }
  else {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){ //IXUSR 실행 가능여부
          clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
          return;
    }
    serve_dynamic(fd, filename, sbuf.st_size);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,char *longmsg)
{
  char buf[MAXLINE], body[MAXLINE];

  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff""> \r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}


void read_requesthdrs(rio_t *rp){
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")){
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}


int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;
  if(!strstr(uri, "cgi-bin")){
    // strcpy(char *dest, char *src)
    // strcpy -> src를 dest에 저장
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri)-1] == '/'){
      // stract -> dest + src 문자열 이어 붙히기
      strcat(filename, "home.html");
    }
    return 1;
  }
  else {
    ptr = index(uri, '?'); // uri에서 ?가 처음 나오는 부분을 찾아서 위치를 포인터에 담는다.
    if(ptr){
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri); //"uri" -> uri 수정
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  srcfd = Open(filename, O_RDONLY, 0); //파일을 오픈
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
      strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
      strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
      strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
      strcpy(filetype, "image/jpeg");
  else
      strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny web server \r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(Fork() == 0){
    setenv("QUERY_STRIN", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
}