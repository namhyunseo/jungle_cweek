/*
    argv[0] -> 실행 파일 이름
    argv[1] -> 서버가 열 포트 번호
*/



#include "../csapp.h"
#include <stdio.h>

// 서버가 연결을 정상적으로 받고, 클라이언트로 데이터를 보낸다.
void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;
    // 클라이언트가 보낸 입력을 받아오는 버퍼를 설정한다.
    Rio_readinitb(&rio, connfd);

    // 클라이언트가 보낸 데이터를 서버 버퍼에 저장한다.
    while((n = Rio_readlineb(&rio, buf, MAXLINE))!= 0){
        // 확인 메시지
        printf("server received %d bytes \n", (int)n);
        // 받은 그대로 다시 돌려준다.
        Rio_writen(connfd, buf, n);
    }
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientdaddr; //클라이언트의 주소를 담는다.
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if(argc != 2){ // 인자 중 한 개가 빠졌을 때
        fprintf(stderr, "usgae: %s <host> <port> \n", argv[0]); // 에러 출력
        exit(0);
    }

    /**
     * Open_listenfd(char *port)
     * 서버가 소켓을 생성하고, 소켓에 포트 번호로 바인딩 한다.
     * 서버는 생성된 소켓 주소로 데이터를 받을 준비가 완료되면 클라이언트로 부터 요청을 기다린다.
     * 반환값으로는 서버 소켓이 생성된다.
     */
    listenfd = Open_listenfd(argv[1]);

    /**
     * listen 상태에서 클라이언트가 read 요청을 보낼 때 까지 기다린다.
     * 요청이 들어오면 요청을 수락하고, 처리한다.
     */
    while(1){ 
        // 클라이언트 주소를 써줄 버퍼의 크기를 넉넉하게 준비한다.
        clientlen = sizeof(struct sockaddr_storage);
        printf("connecting ... \n");
        /**
         * 클라이언트가 connect()요청을 보내면, Accept가 동작해서 처리한다.
         * int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
         * 그냥... 요청을 받으면.. 새로운 소켓을 생성한다.. 그리고 accept는 backlog에 기록된 연결 요청들을 순차적으로 쳐낸다.
         * backlog는 listenfd에 backlog에 기록된다. backlog가 비어있을 경우 accept에서 block상태로 멈춰있는다.
         */
        connfd = Accept(listenfd, (SA *)&clientdaddr, &clientlen);
        // 바이너리 주소 -> 사람이 읽을 수 있는 주소로 변환
        Getnameinfo((SA *) &clientdaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Conntected to (%s, %s), %d \n", client_hostname, client_port, connfd);
        // 연결 하나에 대한 I/O를 처리한다.
        echo(connfd);
        Close(connfd);
    }
    exit(0);

}