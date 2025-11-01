/* client 동작
1. 서버와 TCP 연결
2. 루프 시작
    1. 표준 입력에서 한 줄 읽기
    2. 읽은 내용을 서버로 전송
    3. 서버의 응답 읽기
    4. 받은 내용을 출력하기
3. 루프 종료
4. 연결 닫기
*/

#include "../csapp.h"
#include <stdio.h>


/*
    shell ->  ./echoclient localhost 12345
    
    argc = 3
    argv[0] = "./echoclient"
    argv[1] = "localhost"
    argv[2] = "12345"

*/


int main(int argc, char **argv) // argc : 인자의 개수, argv : 인자의 문자열 배열
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio; 

    if(argc != 3){ // 인자 중 한 개가 빠졌을 때
        fprintf(stderr, "usgae: %s <host> <port> \n", argv[0]); // 에러 출력
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    // 커널 -> socket -> connect / return : socket fd
    clientfd = Open_clientfd(host, port); // host, port로 IP주소로 만든다.

    /** rio -> 서버에서 전달받은 데이터를 담는다.
     * 서버 write() → TCP → [커널 recv buffer] → read(fd) → [rio_buf] → 네 프로그램의 buf
     * readinitb는 recv buf에 저장된 데이터를 rio버퍼로 읽어 오도록 설정
     * rio_t 타입의 구조체가 선언된된다.
     * 이 구조체는 사용자 영역에 rio 버퍼를 만들고, 이 버퍼가 socket fd에서 데이터를 읽어오도록 설정한다.
     */
    Rio_readinitb(&rio, clientfd);

    
    /**
     * char *fgets(char *s, int n, FILE *stream)
     * 한 줄 단위 입력 함수
     * 개행을 만나거나, 버퍼 한계 도달, EOF를 만날 때 까지 읽는다.
     * s 버퍼에 입력 스트림(stdin, fopen)에서 읽은 데이터를 n 이하 만큼 저장한다.
     */
    while(Fgets(buf, MAXLINE, stdin) != NULL){
        /**
         * 서버로 user buf에 있는 데이터를 clientfd에게 전달해서 write 하도록 한다.
         * 그러면 내부적으로 소켓 객체로 write 요청이 갈거고, 커널이 TCP 계층에 따라서 데이터를 전달한다.
         */
        Rio_writen(clientfd, buf, strlen(buf)); 
        Rio_readlineb(&rio, buf, MAXLINE); // rio 버퍼로 서버에서 보낸 응답을 받아온다.
        /**
         * int fputs(const char *s, FILE *stream);
         * buf에 저장된 값들을 출력한다.
         */
        Fputs(buf, stdout); // 서버 응답으로 받은 값을 표준 출력에 출력.
    }
    Close(clientfd);
    exit(0);
}
