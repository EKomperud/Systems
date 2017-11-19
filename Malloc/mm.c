/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * The heap check and free check always succeeds, because the
 * allocator doesn't depend on any of the old data.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

/* the size of additional blocks of memory that must be allocated with each payload */
#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))

/* get the header from a payload pointer */
#define HDRP(pp) ((char *)(pp) - sizeof(block_header))

/* get the footer from a payload pointer */
#define FTRP(pp) ((char *)(pp)+GET_SIZE(HDRP(pp))-OVERHEAD)

/* get a size_t value from a block header/footer pointer */
#define GET(pp) (*(size_t *) (pp))

/* get the size of a block */
//#define GET_SIZE(p) (GET(p) & ~0xF)
#define GET_SIZE(p) ((block_header *)(p))->size

/* get the allocation status of a block */
//#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_ALLOC(p) ((block_header *)(p))->allocated

/* install a value to a block pointer's size_t value */
#define PUT(p, val) (*(size_t *) (p) = (val))

/* pack a size value and allocation value into one size_t value */
#define PACK(size, alloc) ((size) | (alloc))

/* get the next block's payload (pointer) */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))

/* get the previous block's payload (pointer) */
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-OVERHEAD))

/* get the next value of a free list node */
#define GET_NEXT_FREE_POINTER(p) ((list_node *)(p))->next

#define GET_NEXT_FREE_VALUE(p) *(((list_node *)(p))->next)

/* get the prev value of a free list node */
#define GET_PREV_FREE_POINTER(p) ((list_node *)(p))->prev

#define GET_PREV_FREE_VALUE(p) *(((list_node *)(p))->prev)

/* get the max of two values */
#define MAX(a,b) (((a)>(b))?(a):(b))

static void mm_coalesce(void *pp);

list_node *free_list = NULL;
void *current_avail = NULL;
int current_avail_size = 0;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  current_avail = mem_map(mem_pagesize());
  current_avail_size = mem_pagesize();

  mm_malloc(0);
  PUT(current_avail + current_avail_size - sizeof(block_footer), PACK(ALIGNMENT, 0x1));
  free_list = current_avail + (3 * ALIGNMENT);
  
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  int needSize = MAX(size, sizeof(list_node));
  int newsize = ALIGN(needSize + OVERHEAD);
  size_t bestFit = -1;
  void *p;

  char foundFit = 0;
  list_node *bestNode = free_list;
  list_node *iterator = free_list;
  while (iterator->next != NULL)
  {
    size_t thisSize = GET_SIZE(HDRP(iterator));
    if (thisSize >= newsize && thisSize < bestFit)
    {
      bestFit = thisSize;
      bestNode = iterator;
      foundFit = 1;
    }
    iterator = iterator->next;
  }

  if (!foundFit)
  {
    //TODO: map new memory and link it to old memory
  }

  GET_SIZE(bestNode) = newsize;                        // Set header information
  GET_ALLOC(bestNode) = 0x1;
  //PUT((block_header *)current_avail, PACK(newsize, 0x1));   
  p = bestNode + sizeof(block_header);                 // Set the payload pointer
  GET_SIZE(FTRP(p)) = newsize;                              // Set the footer pointer memory to footer
  //PUT(FTRP(p), newsize);                                    

  PUT((block_header *)current_avail, current_avail_size);   // Set the header of the unallocated memory
  free_list = current_avail + sizeof(block_header);
  
  
  //TODO: Make sure payload pointer is 16-byte aligned
  //TODO: Avoiding footers in allocated blocks
  
  return p;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  //PUT(HDRP(ptr), GET_SIZE(ptr));
  GET_ALLOC(HDRP(ptr)) = 0x1;
  mm_coalesce(ptr);
}

/*
 * coalesce - If a block-to-be-freed neighbors free blocks, coalesce them.
  */
static void mm_coalesce(void *pp)
{
	size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(pp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(pp)));
	size_t size = GET_SIZE(HDRP(pp));

	if (prev_alloc && next_alloc)
	{
    // DO NOT coalesce with neighboring blocks

		// DO update the free list
    list_node *free_node = (list_node *)pp;
    // install pointer from you, to the current head of the free list
    free_node->next = free_list;
    free_list->prev = free_node;

    // update free list pointer to point to you
    free_list = free_node;
	}
	else if (prev_alloc && !next_alloc)
	{
    // DO coalesce with the forward neighbor
		size += GET_SIZE(HDRP(NEXT_BLKP(pp)));
		GET_SIZE(HDRP(pp)) = size;
		//PUT(HDRP(pp), PACK(size,prev_alloc));
		GET_SIZE(FTRP(pp)) = size;
		//PUT(FTRP(pp), size);

    // DO replace the forward neighbor in the free list
    list_node *free_node = (list_node *)pp;
    list_node *forward_neighbor = (list_node *)NEXT_BLKP(pp);

    if (forward_neighbor->prev == NULL && forward_neighbor->next != NULL)           // forward neighbor is the start of free list
    {
      free_node->next = forward_neighbor->next;
      free_node->next->prev = free_node;

      free_node->prev = NULL;
      free_list = free_node;
    }
    else if (forward_neighbor->prev != NULL && forward_neighbor->next != NULL)      // forward neighbor is in the middle of free list
    {
      free_node->next = forward_neighbor->next;
      free_node->next->prev = free_node;

      free_node->prev = forward_neighbor->prev;
      free_node->prev->next = free_node;
    }
    else if (forward_neighbor->prev != NULL && forward_neighbor->next == NULL)      // forward neighbor is at the end of free list
    {
      free_node->next = NULL;

      free_node->prev = forward_neighbor->prev;
      free_node->prev->next = free_node;
    }
	}
	else if (!prev_alloc && next_alloc)
	{
    // DO coalesce with the back neighbor
		size += GET_SIZE(HDRP(PREV_BLKP(pp)));
		GET_SIZE(FTRP(pp)) = size;
		//PUT(FTRP(pp), size);
		GET_SIZE(HDRP(PREV_BLKP(pp))) = size;
		//PUT(HDRP(PREV_BLKP(pp)), PACK(size, prev_alloc));
		pp = PREV_BLKP(pp);

    // DO NOT replace the back neighbor in the free list
	}
	else if (!prev_alloc && !next_alloc)
	{
    // DO coalesce with both neighbors
		size += (GET_SIZE(HDRP(NEXT_BLKP(pp))) + GET_SIZE(HDRP(PREV_BLKP(pp))));
		GET_SIZE(FTRP(NEXT_BLKP(pp))) = size;
		//PUT(FTRP(NEXT_BLKP(pp)), size);
		GET_SIZE(HDRP(PREV_BLKP(pp))) = size;
		//PUT(HDRP(PREV_BLKP(pp)), PACK(size, prev_alloc));
		pp = PREV_BLKP(pp);
	}
}

/*
 * mm_check - Check whether the heap is ok, so that mm_malloc()
 *            and proper mm_free() calls won't crash.
 */
int mm_check()
{
  return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{
  return 1;
}
