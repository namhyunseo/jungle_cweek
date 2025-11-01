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

#define WSIZE       4
#define DSIZE       8
#define CHUNKSIZE  (1 << 8)
#define ALIGNMENT 8
#define PTRSZ sizeof(void *)
#define MIN (DSIZE + 2 * PTRSZ) // 24 bytes on 64bit

#define MAX(x, y) ((x) > (y)? (x) : (y))
#define PACK(size, alloc)  ((size) | (alloc))
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp)     ((char *)(bp) - WSIZE)
#define FTRP(bp)     ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))
#define NEXT(bp)     (*(void **)(bp))
#define PREV(bp)     (*(void **)((char *)(bp) + PTRSZ))
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)


static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *first_fit(size_t size);
static void place(void *ptr, size_t size);
static void *next_fit(size_t size);
static void add_list(void *bp);
static void delete_list(void *bp);
static void *heap_listp = NULL; //프롤로그의 payload 진입점
static void *exp_list_head = NULL; //list head

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

// 입력 : 필요한 크기 
// 출력 : 새로운 가용 블럭 주소 (unlinked)
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


void add_list(void *bp)
{
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

void delete_list(void *bp)
{
    void *prev = PREV(bp);
    void *next = NEXT(bp);

    // bp의 이전, 이후 block을 연결
    if(prev){
        NEXT(prev) = next;
    } else exp_list_head = next;
    if(next) PREV(next) = prev;

    // bp 초기화
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
        asize = MIN; // 24로 설정
    }
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


// void *first_fit(size_t asize) {
//     for (void *bp = exp_list_head; bp; bp = NEXT(bp)) {
//         if (GET_SIZE(HDRP(bp)) >= asize){
//             return bp;
//         }
//     }
//     return NULL;
// }

// void *first_fit(size_t asize){
//     const size_t CAP = 48;
//     void *best_cap = NULL;
//     size_t best_cap_rem = (size_t)-1;

//     void *best_any = NULL;
//     size_t best_any_rem = (size_t)-1;

//     for(void *bp = exp_list_head; bp; bp=NEXT(bp)){
//         size_t sz = GET_SIZE(HDRP(bp));
//         if(sz < asize) continue;
        
//         size_t rem = sz - asize;
//         size_t eff_rem = (rem >= MIN) ? rem : 0;

//         if(eff_rem <= CAP){
//             if (best_cap == NULL || eff_rem < best_cap_rem ||
//                 (eff_rem == best_cap_rem && sz < GET_SIZE(HDRP(best_cap)))) {
//                 best_cap = bp;
//                 best_cap_rem = eff_rem;
//             }
//             } else {
//             if (best_any == NULL || eff_rem < best_any_rem) {
//                 best_any = bp;
//                 best_any_rem = eff_rem;
//             }
//         }
//     }
//     return best_cap ? best_cap : best_any;

// }

void *first_fit(size_t asize) {
    const size_t CAP_WASTE = 128;               
    void *best_cap = NULL;  size_t best_cap_rem = (size_t)-1;
    void *best_any = NULL;  size_t best_any_rem = (size_t)-1;

    for (void *bp = exp_list_head; bp; bp = NEXT(bp)) {
        size_t sz = GET_SIZE(HDRP(bp));
        if (sz < asize) continue;

        size_t rem = sz - asize;
        size_t eff = (rem >= MIN) ? rem : 0;    

        if (eff <= CAP_WASTE) {
            if (!best_cap || eff < best_cap_rem ||
               (eff == best_cap_rem && sz < GET_SIZE(HDRP(best_cap)))) {
                best_cap = bp; best_cap_rem = eff;
                if (eff == 0) return bp;       
            }
        } else {
            if (!best_any || eff < best_any_rem) {
                best_any = bp; best_any_rem = eff;
            }
        }
    }
    return best_cap ? best_cap : best_any;
}


void place(void *ptr, size_t asize){
    size_t csize = GET_SIZE(HDRP(ptr));
    size_t rem = csize - asize;
    delete_list(ptr); //우선은 list에서 제거

    if(rem >= MIN){ //spliting
        PUT(HDRP(ptr), PACK(asize, 1));
        PUT(FTRP(ptr), PACK(asize, 1));
        void *next = NEXT_BLKP(ptr);
        PUT(HDRP(next), PACK(rem, 0));
        PUT(FTRP(next), PACK(rem, 0));
        // coalesce(next); // 분할된 부분은 다시 병합
        add_list(next);
    }else{
        PUT(HDRP(ptr), PACK(csize, 1));
        PUT(FTRP(ptr), PACK(csize, 1));
    }
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


// 입력 -> free block, but not linked
// 결과 -> linked block
// 호출부 -> place, free, place
void *coalesce(void *bp){
    size_t curr = GET_SIZE(HDRP(bp));
    size_t prev = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    // size_t prev = 0;
    // size_t next = 0;

    void *prevp = PREV_BLKP(bp);
    void *nextp = NEXT_BLKP(bp);

    
    if (!prev) delete_list(prevp); // 가용상태이면 리스트에서 삭제
    if (!next) delete_list(nextp); 

    if(!prev && !next){
        void *prevp = PREV_BLKP(bp);
        void *nextp = NEXT_BLKP(bp);
        curr += GET_SIZE(HDRP(prevp)) + GET_SIZE(HDRP(nextp));
        PUT(HDRP(prevp), PACK(curr, 0));
        PUT(FTRP(nextp), PACK(curr, 0));
        bp = prevp; // 새 병합 블록의 bp는 prev의 payload
    }else if (prev && !next)
    {
        curr += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(curr, 0));
        PUT(FTRP(bp), PACK(curr, 0));
    }else if (!prev && next){
        curr += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(curr, 0));
        PUT(FTRP(bp), PACK(curr, 0));
        bp = PREV_BLKP(bp);
    }
    add_list(bp); // 가용 리스트에 추가한다
    return bp;
    
}


void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) return mm_malloc(size);
    if (size == 0) { mm_free(ptr); return NULL; }

    // 원하는 블록 크기 정규화
    size_t asize = ALIGN(size + DSIZE);
    if (asize < MIN) asize = MIN;

    size_t csize = GET_SIZE(HDRP(ptr));

    // 1) 축소: 남는 공간이 충분하면 꼬리 분할하여 free
    if (asize <= csize) {
        size_t rem = csize - asize;
        if (rem >= MIN) {
            // 현재 블록을 asize로 줄이고
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));

            // 뒤쪽을 새 free 블록으로 만들기
            void *nbp = (char *)ptr + asize;
            PUT(HDRP(nbp), PACK(rem, 0));
            PUT(FTRP(nbp), PACK(rem, 0));
            coalesce(nbp); // 분할 조각 병합/삽입
        }
        return ptr; // as-is
    }

    // 2) 확장: 오른쪽 이웃이 free면 병합 시도
    void *nextp = NEXT_BLKP(ptr);
    size_t next_alloc = GET_ALLOC(HDRP(nextp));
    size_t next_size  = GET_SIZE(HDRP(nextp));

    // 2-1) 에필로그 바로 앞이면 힙을 늘려서 next를 free로 만든 뒤 다시 계산
    if (next_size == 0 && next_alloc == 1) {
        size_t need = asize - csize;
        size_t words = (need + WSIZE - 1) / WSIZE;         // byte→word 올림
        if (extend_heap(words) == NULL) return NULL;       // 힙 확장 실패시 포기
        nextp = NEXT_BLKP(ptr);
        next_alloc = GET_ALLOC(HDRP(nextp));
        next_size  = GET_SIZE(HDRP(nextp));
    }

    // 2-2) 오른쪽 이웃이 free라면 잡아먹기
    if (next_alloc == 0) {
        size_t total = csize + next_size;
        if (total >= asize) {
            // 이웃 free 블록은 리스트에서 먼저 제거
            delete_list(nextp);

            size_t rem = total - asize;
            if (rem >= MIN) {
                // 앞은 할당 asize, 뒤는 새 free(rem)
                PUT(HDRP(ptr), PACK(asize, 1));
                PUT(FTRP(ptr), PACK(asize, 1));
                void *nbp = (char *)ptr + asize;
                PUT(HDRP(nbp), PACK(rem, 0));
                PUT(FTRP(nbp), PACK(rem, 0));
                coalesce(nbp);
            } else {
                // 잔여가 너무 작으면 전부 할당으로 흡수
                PUT(HDRP(ptr), PACK(total, 1));
                PUT(FTRP(ptr), PACK(total, 1));
            }
            return ptr; // in-place 확장 성공
        }
    }

    // 3) in-place 실패 → 새로 할당하여 복사
    void *newptr = mm_malloc(size);
    if (newptr == NULL) return NULL;

    size_t copy_size = csize - DSIZE;        // payload 크기
    if (size < copy_size) copy_size = size;  // 요청한 크기가 더 작으면 그만큼만
    memcpy(newptr, ptr, copy_size);
    mm_free(ptr);
    return newptr;
}
