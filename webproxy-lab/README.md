####################################################################
# CS:APP Proxy Lab
#
# Student Source Files
####################################################################

This directory contains the files you will need for the CS:APP Proxy
Lab.

proxy.c
csapp.h
csapp.c
    These are starter files.  csapp.c and csapp.h are described in
    your textbook. 

    You may make any changes you like to these files.  And you may
    create and handin any additional files you like.

    Please use `port-for-user.pl' or 'free-port.sh' to generate
    unique ports for your proxy or tiny server. 

Makefile
    This is the makefile that builds the proxy program.  Type "make"
    to build your solution, or "make clean" followed by "make" for a
    fresh build. 

    Type "make handin" to create the tarfile that you will be handing
    in. You can modify it any way you like. Your instructor will use your
    Makefile to build your proxy from source.

port-for-user.pl
    Generates a random port for a particular user
    usage: ./port-for-user.pl <userID>

free-port.sh
    Handy script that identifies an unused TCP port that you can use
    for your proxy or tiny. 
    usage: ./free-port.sh

driver.sh
    The autograder for Basic, Concurrency, and Cache.        
    usage: ./driver.sh

nop-server.py
     helper for the autograder.         

tiny
    Tiny Web server from the CS:APP text
-----

다음은 해당 GitHub 저장소의 README.md 파일 내용의 한글 번역입니다. 원문은 CS:APP(Computer Systems: A Programmer’s Perspective) 교재의 프록시 랩을 위한 소스 파일들을 설명하는 문서입니다.

### CS:APP 프록시 랩 – 학생 소스 파일

* 이 디렉터리에는 **CS:APP Proxy Lab**을 수행하는 데 필요한 파일들이 들어 있습니다.
* `proxy.c`, `csapp.h`, `csapp.c`는 시작용 소스 파일들로, `csapp.c`와 `csapp.h`는 교재에서 설명되어 있습니다.
* 필요에 따라 이 파일들을 자유롭게 수정할 수 있으며, 원하는 추가 파일을 생성해 제출할 수 있습니다.
* 프록시나 타이니 웹 서버를 실행할 고유 포트를 생성할 때는 `port-for-user.pl` 혹은 `free-port.sh` 스크립트를 사용하세요.

### 각 파일/스크립트 설명

| 파일/스크립트              | 설명                                                                                                                                                                                                         |
| -------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Makefile**         | 프록시 프로그램을 빌드하는 makefile입니다. 터미널에서 `make`를 실행하면 솔루션이 빌드되고, `make clean` 후 `make`를 실행하면 깨끗한 상태에서 다시 빌드합니다. `make handin`을 실행하면 제출용 tar 파일이 생성됩니다. 이 Makefile은 자유롭게 수정할 수 있으며, 강사가 이 파일을 이용해 소스에서 프록시를 빌드합니다. |
| **port-for-user.pl** | 특정 사용자에게 랜덤 포트를 생성해 줍니다. 사용법: `./port-for-user.pl <userID>`.                                                                                                                                               |
| **free-port.sh**     | 사용 가능한 TCP 포트를 찾아서 프록시나 타이니 서버에서 사용할 수 있게 해 주는 스크립트입니다. 사용법: `./free-port.sh`.                                                                                                                             |
| **driver.sh**        | 기본·동시성·캐시 기능을 채점하는 자동 채점기입니다. 사용법: `./driver.sh`.                                                                                                                                                          |
| **nop-server.py**    | 자동 채점기를 위한 보조 서버 스크립트입니다.                                                                                                                                                                                  |
| **tiny**             | CS:APP 교재에 나오는 간단한 웹 서버입니다.                                                                                                                                                                                |
