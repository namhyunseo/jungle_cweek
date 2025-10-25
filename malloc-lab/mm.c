/*
 * mm-naive.c - 가장 빠르지만 메모리 효율은 최악인 malloc 패키지.
 *
 * 이 순진한(naive) 방식에서는, 블록을 할당할 때 단지 brk 포인터(힙의 끝 지점)를
 * 증가시키기만 한다. 블록은 순수 페이로드로만 이루어지며, 헤더나 푸터가 없다.
 * 블록들은 결합(coalesce)되거나 재사용되지 않는다. realloc은
 * mm_malloc과 mm_free를 그대로 이용해 직접 구현되어 있다.
 *
 * 학생들을 위한 노트: 아래의 헤더 주석을 여러분의 해법을
 * 상위 수준에서 설명하는 여러분만의 헤더 주석으로 교체하라.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.   
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE       4              
#define DSIZE       8              
#define CHUNKSIZE  (1 << 12)       
#define MAX(x, y) ((x) > (y)? (x) : (y))

#define PACK(size, alloc)  ((size) | (alloc))

#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


static char *heap_listp = NULL; //프롤로그의 payload 진입점
static void *coalesce(void *ptr);
static void *extend_heap(size_t words);
static char *find_fit(size_t size);
static void place(void *ptr, size_t size);
static void spliting(void *ptr, size_t size);
int g_search_steps=0, g_split_cnt=0, g_coal_cnt=0, g_extend_cnt=0;

int mm_init(void)
{
    // heap에서 16바이트 공간을 받아온다.
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) return -1;
    // 프롤로그, 에필로그 세팅 -> heap 초기화
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); //header
    PUT(heap_listp +(2*WSIZE), PACK(DSIZE,1)); //footer
    PUT(heap_listp +(3*WSIZE), PACK(0, 1)); //epilogue header
    heap_listp += (2 * WSIZE); // 포인터 위치를 heap의 prol_Head 끝에 맞춤.

    // extend 시도 후 실패 시 -1 반환
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL)return -1;
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp; //payload 포지션 
    size_t size;

    // 입력 words가 8의 배수인지 확인 -> 8의 배수로 맞춤
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    // 4096바이트 VM의 1page 크기로 block생성
    if((long)(bp = mem_sbrk(size)) == -1) return NULL;

    // 받아온 영역에 Block 초기 설정
    PUT(HDRP(bp), PACK(size, 0)); // header
    PUT(FTRP(bp), PACK(size, 0)); // footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); //new epilheader

    return coalesce(bp);
}


void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsz;
    char *ptr;


    if(size == 0) return NULL;
    // size 정렬 -> 최소 크기 16바이트 + 8의 배수
    if(size <= DSIZE){
        asize = 2 * DSIZE; //16B
    }else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/DSIZE);
    }

    // free block탐색 -> first fit
    if((ptr = find_fit(asize)) != NULL){
        place(ptr, asize);
        return ptr;
    }

    // 가용 블럭이 없을 경우
    extendsz = MAX(asize, CHUNKSIZE);
    if((ptr = extend_heap(extendsz / WSIZE)) == NULL) return NULL;
    place(ptr, asize);
    return ptr;

}

static char *find_fit(size_t size)
{
    char *ptr = NEXT_BLKP(heap_listp);

    /*의사코드
    while(block의 사이즈가 0이면 -> 에필로그가 아닐 때 까지){
        if(size > GET_size(ptr) && GET_ALLOC(ptr)){
            return ptr;
        }else ptr = 다음 블럭 header로
    }
    if(GET_SIZE(ptr) == 0){ 에필이라면
        if(extend_heap(size/WSIZE)==NULL) return NULL;
        return ptr;
    }
    */
    while(GET_SIZE(ptr) != 0){
        if(size <= GET_SIZE(ptr) && GET_ALLOC(ptr) == 0){
            return ptr;
        }else ptr = HDRP(NEXT_BLKP(ptr));
    }
    return NULL;
}

void place(void *ptr, size_t asize){
    size_t csize = GET_SIZE(HDRP(ptr));
    size_t rem = csize - asize;

    if(rem >= 2*DSIZE) spliting(ptr, asize);
    else {
        PUT(HDRP(ptr), PACK(csize, 1));
        PUT(HDRP(ptr), PACK(csize, 1));
    }
}

void spliting(void *ptr, size_t asize)
{
    /*
    분할 할 block과 넣을 블럭의 사이즈를 가져온다.
    */
    size_t csize = GET_SIZE(HDRP(ptr));
    size_t rem = csize - asize;

    PUT(HDRP(ptr), PACK(asize, 1));
    PUT(FTRP(ptr), PACK(asize, 1));

    void *next = NEXT_BLKP(ptr);
    PUT(HDRP(next),PACK(rem, 1));
    PUT(FTRP(next),PACK(rem, 1));
}

// ptr -> payload 시작 주소 
void mm_free(void *ptr)
{
    if(ptr == NULL) return;

    size_t size = GET_SIZE(HDRP(ptr)); //현재 블록 크기
    PUT(HDRP(ptr), PACK(size, 0)); // header -> free로 표시
    PUT(FTRP(ptr), PACK(size, 0)); // putter 가용 표시
    // 병합 처리
    coalesce(ptr);
}



static void *coalesce(void *ptr)
{
    size_t prev = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));

    if(prev && next){
        return ptr;
    }
    else if(prev && !next){
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0)); //헤더에 정보 기록
        PUT(FTRP(ptr), PACK(size, 0)); //푸터에 정보 기록
        return ptr;
    }else if(!prev && next){
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        // 현재 블럭의 footer에 병합 후 사이즈 기록
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        return PREV_BLKP(ptr);
    }else{
        size += GET_SIZE(HDRP((PREV_BLKP(ptr)))) + GET_SIZE(FTRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        return PREV_BLKP(ptr);
    }
    return ptr;
}

void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}