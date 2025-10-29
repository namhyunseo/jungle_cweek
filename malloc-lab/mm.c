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
// size, alloc을 현재 블록에 기록
#define PACK(size, alloc)  ((size) | (alloc))

#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))
// 현재 블럭의 주소에서 size, alloc 여부
#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)
// 현재 블럭의 헤더, 푸터 정보
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
// 다음, 이전 블록의 페이로드 주소
// #define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
// #define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/////// explicit 전용 메서드 ///////////
// 가용 리스트에서 해당 블록의 앞, 뒤에 연결된 요소
// LLP List Linked prev, LLN List Linked next
#define NEXT(bp)     (*((void **)(bp)))
#define PREV(bp)     (*((void **)((char *)(bp) + PTRSZ)))
#define PTRSZ       (sizeof(void*))

#define MIN     (DSIZE + 2*PTRSZ) //64bit-> 24B
// #define MIN ALIGN(2*WSIZE + 2*PTRSZ)


static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static char *first_fit(size_t size);
static void place(void *ptr, size_t size);
static void spliting(void *ptr, size_t size);
static char *next_fit(size_t size);
static char *heap_listp = NULL; //프롤로그의 payload 진입점
static void *exp_list_head = NULL; //list head
static void add_list(void *bp);
static void delete_list(void *bp);

//////////////////////////////////////////////////////////////////////////

int mm_init(void)
{
    // heap에서 16바이트 공간을 받아온다.
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) return -1;
    // 프롤로그, 에필로그 세팅 -> heap 초기화
    PUT(heap_listp, 0);
    PUT(heap_listp +(1*WSIZE), PACK(DSIZE, 1)); //header 프롤
    PUT(heap_listp +(2*WSIZE), PACK(DSIZE,1)); //footer 프롤
    PUT(heap_listp +(3*WSIZE), PACK(0, 1)); //epilogue header
    heap_listp += (2 * WSIZE); // 포인터 위치를 heap의 prol_Head 끝에 맞춤.
    // extend 시도 후 실패 시 -1 반환
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL)return -1;
    // exp_list_head = NEXT_BLKP(heap_listp);
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp; //payload 포지션 
    // 입력 words가 8의 배수인지 확인 -> 8의 배수로 맞춤
    size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    // 4096바이트 VM의 1page 크기로 block생성
    if((long)(bp = mem_sbrk(size)) == -1) return NULL;

    // 받아온 영역에 Block 초기 설정
    PUT(HDRP(bp), PACK(size, 0)); // header
    PUT(FTRP(bp), PACK(size, 0)); // footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); //new epilheader
    return coalesce(bp);
}
/////////////////////////////////////////////////////////////////////
static inline int is_linked(void *bp){
    return (bp == exp_list_head) || PREV(bp) || NEXT(bp);
}
void add_list(void *bp)
{
    //리스트에 이미 있는 요청이 들어올 경우
    // if (is_linked(bp)) delete_list(bp);
    //LIFO -> 새로운 블럭을 head에 연결
    //head에 연결된게 없을 때
    //head에 연결된게 있을 때

    if(bp == exp_list_head) return;
    PREV(bp) = NULL;
    NEXT(bp) = exp_list_head;
    
    if(exp_list_head){
        PREV(exp_list_head) = bp;
    }
    exp_list_head = bp;

}
// static void add_list(void *bp) {
//     // 이미 리스트에 있으면 먼저 빼기 (head에 자기 자신을 다시 넣는 사고 방지)
//     if (bp == exp_list_head || PREV(bp) != NULL || NEXT(bp) != NULL)
//         delete_list(bp);

//     PREV(bp) = NULL;
//     NEXT(bp) = exp_list_head;
//     if (exp_list_head) PREV(exp_list_head) = bp;
//     exp_list_head = bp;
// }

void delete_list(void *bp)
{
    // 리스트에 없는 요청이 들어올 경우
    // if(!is_linked(bp)){ PREV(bp) = NEXT(bp) = NULL; return; }
    void *prev = PREV(bp);
    void *next = NEXT(bp);

    if(prev){
        NEXT(prev) = next;
    } else exp_list_head = next;
    if(next) PREV(next) = prev;

    PREV(bp) = NULL;
    NEXT(bp) = NULL;
}

void *mm_malloc(size_t size)
{
    // printf("malloc 처리 : \n");
    size_t asize = ALIGN(size + DSIZE);
    // size_t asize;
    size_t extendsz;
    void *ptr;
    if(asize == 0) return NULL;
    // size 정렬 -> 최소 크기 16바이트 + 8의 배수
    if(asize < MIN){ // 24B 보다 작을 경우
        asize = 2 * MIN; // 24로 설정
    }
    // else {
    //     asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/DSIZE);
    // }
    // asize = MAX(ALIGN(size + 2*WSIZE), MIN);
    // free block탐색 -> first fit
    if((ptr = first_fit(asize)) != NULL){
        place(ptr, asize);
        return ptr;
    }
    // 가용 블럭이 없을 경우
    extendsz = MAX(asize, CHUNKSIZE);
    if((ptr = extend_heap(extendsz / WSIZE)) == NULL) return NULL;
    place(ptr, asize);
    return ptr;
}

/* first-fit for implicit
char *first_fit(size_t asize)
{
    void *bp = NEXT_BLKP(heap_listp);
    int lim = 16;
    for (; GET_SIZE(HDRP(bp)) != 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && asize <= GET_SIZE(HDRP(bp))) {
            return bp;
        }
    }
    return NULL;
}
*/

/* next-fit 
char *next_fit(size_t size)
{
    return;
}
*/

// for explicit 
char *first_fit(size_t asize)
{
    if (asize == 0) return NULL;
    void *bp = exp_list_head; //head에 연결된 주소

    for(; bp != NULL; bp = NEXT(bp)){
        if(asize <= GET_SIZE(HDRP(bp))){
            return bp;
        }
    }
    return NULL;
}

void place(void *ptr, volatile size_t asize){
    size_t csize = GET_SIZE(HDRP(ptr));
    size_t rem = csize - asize;

    if(rem >= MIN) spliting(ptr, asize);
    else {
        PUT(HDRP(ptr), PACK(csize, 1));
        PUT(FTRP(ptr), PACK(csize, 1));
        delete_list(ptr);
    }
}
/* for implicit
void spliting(void *ptr, size_t asize)
{
    // 분할 할 block과 넣을 블럭의 사이즈를 가져온다.
    size_t csize = GET_SIZE(HDRP(ptr));
    size_t rem = csize - asize;

    PUT(HDRP(ptr), PACK(asize, 1));
    PUT(FTRP(ptr), PACK(asize, 1));

    void *next = NEXT_BLKP(ptr);
    PUT(HDRP(next),PACK(rem, 0));
    PUT(FTRP(next),PACK(rem, 0));
}
*/

// for explicit
void spliting(void *ptr, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(ptr));
    // printf("csize : %zu \n", csize);
    // printf("asize : %zu \n", asize);
    size_t rem = csize - asize;
    
    delete_list(ptr); //아예 분리 해버리고 자른부분만 맨앞으로 연결
    PUT(HDRP(ptr), PACK(asize, 1));
    PUT(FTRP(ptr), PACK(asize, 1));

    void *next = NEXT_BLKP(ptr); 
    PUT(HDRP(next),PACK(rem, 0));
    PUT(FTRP(next),PACK(rem, 0));
    add_list(next); // 잘랐던 부분 다시 넣어줌
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


/* for implicit
static void *coalesce(void *ptr)
{
    size_t prev = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));

    if(prev && next){
        return ptr;
    }
    else if(prev && !next){ //next만 할당 가능
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0)); //헤더에 정보 기록
        PUT(FTRP(ptr), PACK(size, 0)); //푸터에 정보 기록
        return ptr;
    }else if(!prev && next){  // prev가용
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        // 현재 블럭의 footer에 병합 후 사이즈 기록
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        return PREV_BLKP(ptr);
    }else{ //둘 다 가용
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        return PREV_BLKP(ptr);
    }
    return ptr;
}
*/

void *coalesce(void *bp){
    volatile size_t curr = GET_SIZE(HDRP(bp));
    volatile size_t prev = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    volatile size_t next = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    // size_t prev = 0;
    // size_t next = 0;

    if(prev && next){
        add_list(bp);
        return bp;
    }else if (prev && !next)
    {
        //제거하고, 병합하고, list에 삽입
        delete_list(NEXT_BLKP(bp));
        curr += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(curr, 0));
        PUT(FTRP(bp), PACK(curr, 0));
    }else if (!prev && next){
        delete_list(PREV_BLKP(bp));
        curr += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(curr, 0));
        PUT(FTRP(bp), PACK(curr, 0));
        bp = PREV_BLKP(bp);
    }else{ // prev, next 모두 가용
        void *prevp = PREV_BLKP(bp);
        void *nextp = NEXT_BLKP(bp);
    // 리스트에서 '두 블록'을 빼야 함
        delete_list(prevp);
        delete_list(nextp);
        curr += GET_SIZE(HDRP(prevp)) + GET_SIZE(HDRP(nextp));
    // 헤더는 prev에, 푸터는 next의 위치에 써야 블록 경계가 맞음
        PUT(HDRP(prevp), PACK(curr, 0));
        PUT(FTRP(nextp), PACK(curr, 0));
        bp = prevp; // 새 병합 블록의 bp는 prev의 payload
    }
    
    add_list(bp);
    return bp;
    
}

void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    void *newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;

    size_t old_size = GET_SIZE(HDRP(ptr)) - DSIZE ;
    size_t copy_size = (size < old_size) ? size : old_size;
    memcpy(newptr, ptr, copy_size);
    mm_free(ptr);
    return newptr;
}