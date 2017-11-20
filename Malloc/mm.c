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

/* rounds down to the nearest multiple of mem_pagesize() */
#define ADDRESS_PAGE_START(p) ((void *)(((size_t)p) & ~(mem_pagesize()-1)))

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

/* get the max of two values */
#define MAX(a,b) (((a)>(b))?(a):(b))

static void mm_coalesce(void *pp);
static int ptr_is_mapped(void *p, size_t len);

list_node *free_list = NULL;
list_node *last_page = NULL;
char debugBool = 0;
int debugCounter = 0;

/* 
 * mm_init - initialize the malloc package with 8 pages.
 */
int mm_init(void)
{
  void *setupPointer = mem_map(8 * mem_pagesize());

  list_node *pageNode = (list_node *)setupPointer;              // Set up page pointer
  pageNode->next = NULL;
  pageNode->prev = NULL;
  last_page = pageNode;
  setupPointer += sizeof(list_node);

                                                              // Set up epiloge pointer
  void *epiloguePointer = setupPointer + ( 8 * mem_pagesize()) - sizeof(block_header) - sizeof(block_footer);
  GET_SIZE(epiloguePointer) = 0x1;
  GET_ALLOC(epiloguePointer) = 0x1;

                                                              // Set up initial chunk
  GET_SIZE(setupPointer) = (8 * mem_pagesize()) - sizeof(list_node) - sizeof(block_header);
  GET_ALLOC(setupPointer) = 0x0;
  void *footerPointer = setupPointer + GET_SIZE(setupPointer) - sizeof(block_footer);
  GET_SIZE(footerPointer) = mem_pagesize() - sizeof(list_node) - sizeof(block_header);

                                                              // Set up prologue chunk
  list_node *firstFreeNode = setupPointer + sizeof(block_header);
  firstFreeNode->next = NULL;
  firstFreeNode->prev = NULL;
  free_list = firstFreeNode;

  mm_malloc(0);
  
  return 0;
}

/* 
 * mm_malloc - Allocate a block by iterating through the free list,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  //printf("%d\n", debugCounter++);
  size_t needSize = MAX(size, sizeof(list_node));
  size_t newSize = ALIGN(needSize + OVERHEAD);
  size_t bestFit = -1;
  void *p;

  char foundFit = 0;
  list_node *bestNode = free_list;
  list_node *iterator = free_list;
  //printf("The best node is %zu, whereas the iterator is %zu\n", bestNode, iterator);
  do
  {
    size_t thisSize = GET_SIZE(HDRP(iterator));
    if (thisSize >= newSize && thisSize < bestFit)
    {
      bestFit = thisSize;
      bestNode = iterator;
      foundFit = 1;
    }
    iterator = iterator->next;
  } while(iterator != NULL);

  if (!foundFit)
  {
    newSize = MAX(PAGE_ALIGN(size), 8 * mem_pagesize());
    //printf("needed size is %zu vs newSize which is %zu\n", size, newSize);
    void *setupPointer = mem_map(newSize);

    list_node *pageNode = (list_node *)setupPointer;              // Set up page pointer
    pageNode->next = NULL;
    pageNode->prev = last_page;
    pageNode->prev->next = pageNode;
    last_page = pageNode;
    setupPointer += sizeof(list_node);

                                                              // Set up epiloge pointer
    void *epiloguePointer = setupPointer + ( 8 * mem_pagesize()) - sizeof(block_header) - sizeof(block_footer);
    GET_SIZE(epiloguePointer) = 0x1;
    GET_ALLOC(epiloguePointer) = 0x1;

                                                                  // Set up initial chunk
    GET_SIZE(setupPointer) = (8 * mem_pagesize()) - sizeof(list_node) - sizeof(block_header);
    GET_ALLOC(setupPointer) = 0x0;
    void *footerPointer = setupPointer + GET_SIZE(setupPointer) - sizeof(block_footer);
    GET_SIZE(footerPointer) = mem_pagesize() - sizeof(list_node) - sizeof(block_header);

                                                                  // Set up prologue chunk
    list_node *firstFreeNode = free_list;
    list_node *newFreeNode = (list_node *)setupPointer + sizeof(block_header);
    newFreeNode->next = firstFreeNode;
    newFreeNode->next->prev = newFreeNode;
    newFreeNode->prev = NULL;
    free_list = newFreeNode;
    bestNode = mm_malloc(0) + sizeof(block_footer);
  }

  GET_SIZE(HDRP(bestNode)) = newSize;                         // Set header information
  GET_ALLOC(HDRP(bestNode)) = 0x1; 
  p = bestNode;                                               // Set the payload pointer
  GET_SIZE(FTRP(p)) = newSize;                                // Set the footer pointer memory to footer                               

  if ((bestFit - newSize) >  (sizeof(list_node) + OVERHEAD))  // If there's leftover memory
  {

    // Set new header information
    block_header *new_header = (block_header *)(FTRP(p) + sizeof(block_footer));
    GET_SIZE(new_header) = bestFit - newSize;
    GET_ALLOC(new_header) = 0x0;
    void *n = new_header + 1;
    GET_SIZE(FTRP(n)) = bestFit - newSize;

    // replace old free list node with new node
    list_node *old_node  = (list_node *)p;
    list_node *free_node = (list_node *)n;
    if (old_node->next != NULL)
    {
      free_node->next = old_node->next;
      old_node->next = NULL;
    }

    if (old_node->prev != NULL)
    {
      free_node->prev = old_node->prev;
      old_node->prev = NULL;
    }
    else
    {
      free_list = free_node;
    }
  }
  else                                                        // If there's no leftover memory
  {
    // remove old node from the free list, but don't replace it
    list_node *old_node = (list_node *)p;
    if (old_node->next != NULL && old_node->prev != NULL)     // Node is in the middle of free list
    {
      old_node->prev->next = old_node->next;
      old_node->next->prev = old_node->prev;
      old_node->prev = NULL;
      old_node->next = NULL;
    }
    else if (old_node->next != NULL && old_node->prev == NULL)  // Node is at the beginning of free list
    {
      free_list = old_node->next;
      old_node->next->prev = NULL;
      old_node->prev = NULL;
      old_node->next = NULL;
    }
    else if (old_node->next == NULL && old_node->prev != NULL)  // Node is at the end of the free list
    {
      old_node->prev->next = NULL;
      old_node->prev = NULL;
      old_node->next = NULL;
    }
  }
  
  //TODO: Make sure payload pointer is 16-byte aligned
  //TODO: Avoiding footers in allocated blocks
  
  return p;
}

/*
 * mm_free 
 */
void mm_free(void *ptr)
{
  GET_ALLOC(HDRP(ptr)) = 0x1;
  mm_coalesce(ptr);
}

/*
 * coalesce - If a block-to-be-freed neighbors free blocks, coalesce them.
  */
static void mm_coalesce(void *pp)
{
  char prev_alloc = 0;
  char next_alloc = 0;
  if (ptr_is_mapped(HDRP(PREV_BLKP(pp)), GET_SIZE(PREV_BLKP(pp))))
  {
    prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(pp)));
  }
  if (ptr_is_mapped(HDRP(NEXT_BLKP(pp)), GET_SIZE(PREV_BLKP(pp))))
  {
    next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(pp)));
  }
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

      forward_neighbor->next = NULL;
      forward_neighbor->prev = NULL;

      free_node->prev = NULL;
      free_list = free_node;
    }
    else if (forward_neighbor->prev != NULL && forward_neighbor->next != NULL)      // forward neighbor is in the middle of free list
    {
      free_node->next = forward_neighbor->next;
      free_node->next->prev = free_node;

      free_node->prev = forward_neighbor->prev;
      free_node->prev->next = free_node;

      forward_neighbor->next = NULL;
      forward_neighbor->prev = NULL;
    }
    else if (forward_neighbor->prev != NULL && forward_neighbor->next == NULL)      // forward neighbor is at the end of free list
    {
      free_node->next = NULL;

      free_node->prev = forward_neighbor->prev;
      free_node->prev->next = free_node;

      forward_neighbor->prev = NULL;
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

static int ptr_is_mapped(void *p, size_t len) {
  void *s = ADDRESS_PAGE_START(p);
  return mem_is_mapped(s, PAGE_ALIGN((p + len) - s));
}

/*
 * mm_check - Check whether the heap is ok, so that mm_malloc()
 *            and proper mm_free() calls won't crash.
 */
int mm_check()
{
  list_node *page_iterator = last_page;
  do
  {
    block_header *chunk_iterator = (block_header *)last_page + 2;

    // check prologue information
    if (GET_SIZE(HDRP(chunk_iterator)) != 0x30 || GET_ALLOC(HDRP(chunk_iterator)) != 0x1 || GET_SIZE(FTRP(chunk_iterator)) != 0x30)
    {
      printf("prologue is broke\n");
      return 0;
    }

    char prev_alloc = 1;

    do
    {
      chunk_iterator += GET_SIZE(HDRP(chunk_iterator)) / ALIGNMENT;
      size_t block_size = GET_SIZE(HDRP(chunk_iterator));
      char block_alloc  = GET_ALLOC(HDRP(chunk_iterator));
      if (!block_alloc && !prev_alloc)
      {
        printf("failed coalesce\n");
        return 0;
      }
      prev_alloc = block_alloc;
      if (GET_SIZE(FTRP(chunk_iterator)) != block_size)
      {
        printf("inconsistent header/footers\n");
        return 0;
      }
      if (!block_alloc)
      {
        list_node *fl_node = (list_node *)chunk_iterator;
        if (fl_node->next == NULL && fl_node->prev == NULL)
        {
          printf("free block isn't in the free list\n");
          return 0;
        }
      }

    } while (ptr_is_mapped(NEXT_BLKP(chunk_iterator), ALIGNMENT));

    page_iterator = last_page->prev;
  } while (page_iterator != NULL);
  return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{
  return GET_ALLOC(HDRP(p));
}
