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
static void print_free_list();
static void print_mapped_pages();

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
  GET_SIZE(footerPointer) = (8 * mem_pagesize()) - sizeof(list_node) - sizeof(block_header);

                                                              // Set up prologue chunk
  list_node *firstFreeNode = setupPointer + sizeof(block_header);
  firstFreeNode->next = NULL;
  firstFreeNode->prev = NULL;
  free_list = firstFreeNode;
  
  void *prologue = mm_malloc(0);

  // mm_init sanity checker:
  //printf("pageNode is at %zu. prologue should be 32 above that: %zu. Free list should be 48 above that: %zu.", pageNode, prologue, free_list);
  //printf(" Epilogue should be 8*page_size - 16 after pageNode: %zu", epiloguePointer );
  
  return 0;
}

/* 
 * mm_malloc - Allocate a block by iterating through the free list,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  printf("allocating memory of size %zu\n", size);
  //printf("%d\n", debugCounter++);
  size_t needSize = MAX(size, sizeof(list_node));
  size_t newSize = ALIGN(needSize + OVERHEAD);
  size_t bestFit = -1;
  void *p;

  char foundFit = 0;
  list_node *bestNode = free_list;
  list_node *iterator = free_list;
  //printf("before: ");
  //print_free_list();
  while(iterator != NULL)
  {
    size_t thisSize = GET_SIZE(HDRP(iterator));
    if (thisSize >= newSize && thisSize < bestFit)
    {
      bestFit = thisSize;
      bestNode = iterator;
      foundFit = 1;
    }
    iterator = iterator->next;
  }
  //printf("the best node is: %zu\n", bestNode);

  if (!foundFit)
  {
    printf("no fit found. allocate more memory\n");
    newSize = MAX(PAGE_ALIGN(newSize), 8 * mem_pagesize());
    void *setupPointer = mem_map(newSize);
    //printf("setupPointer starts at %zu and goes to %zu\n", setupPointer, setupPointer + newSize);
    //printf("before: "); print_mapped_pages();

    list_node *pageNode = (list_node *)setupPointer;              // Set up page pointer
    pageNode->prev = NULL;
    pageNode->next = last_page;
    pageNode->next->prev = pageNode;
    last_page = pageNode;
    setupPointer += sizeof(list_node);
    //printf("after: "); print_mapped_pages();
    //printf("is this pointer mapped? %d\n", ptr_is_mapped(setupPointer, newSize - OVERHEAD));
                                                              // Set up epiloge pointer
    size_t epiAddress = setupPointer + newSize - OVERHEAD;
    //printf("i'm currently trying to set epiPointer to %zu\n", epiAddress);
    block_header *epiloguePointer = (block_header *)epiAddress;
    //printf("we make it here");
    GET_SIZE(epiloguePointer) = 0x1;
    GET_ALLOC(epiloguePointer) = 0x1;
    
                                                                  // Set up initial chunk
    GET_SIZE(setupPointer) = newSize - sizeof(list_node) - sizeof(block_header);
    GET_ALLOC(setupPointer) = 0x0;
    size_t footAddress = setupPointer + GET_SIZE(setupPointer) - sizeof(block_footer);
    block_footer *footerPointer = (block_footer *)footAddress;
    GET_SIZE(footerPointer) = newSize - sizeof(list_node) - sizeof(block_header);
    
    
    // Set up prologue chunk
    if (free_list != NULL)
    {
      list_node *firstFreeNode = free_list;
      list_node *newFreeNode = (list_node *)setupPointer + sizeof(block_header);
      newFreeNode->next = firstFreeNode;
      newFreeNode->next->prev = newFreeNode;
      newFreeNode->prev = NULL;
      free_list = newFreeNode;
    }
    else
    {
      list_node *newFreeNode = (list_node *)setupPointer + sizeof(block_header);
      newFreeNode->next = NULL;
      newFreeNode->prev = NULL;
      free_list = newFreeNode;
    }    

    
    void *prologue = mm_malloc(0);
    //printf("we also make it here");
    bestNode = prologue + ALIGNMENT + OVERHEAD;
    bestFit = GET_SIZE(HDRP(bestNode));

    //Extend sanity checker
    //printf("pageNode is at %zu. prologue should be 32 above that: %zu. Free list should be 48 above that: %zu.", pageNode, prologue, free_list);
    //printf("Epilogue should be 8*page_size - 16 after pageNode: %zu", epiloguePointer );
  }

  GET_SIZE(HDRP(bestNode)) = newSize;                         // Set header information for the newly allocated block
  GET_ALLOC(HDRP(bestNode)) = 0x1;                            // Set the allocated status
  p = bestNode;                                               // Set the payload pointer
  GET_SIZE(FTRP(p)) = newSize;                                // Set the footer pointer memory to footer
  //printf("best_node's new size is: %zu\n", GET_SIZE(HDRP(bestNode)));
  
  if ((bestFit - newSize) >=  (sizeof(list_node) + OVERHEAD))  // If there's leftover memory
  {

    // Set new header information
    block_header *new_header = (block_header *)(FTRP(p) + sizeof(block_footer));
    GET_SIZE(new_header) = bestFit - newSize;
    GET_ALLOC(new_header) = 0x0;
    void *n = new_header + 1;
    GET_SIZE(FTRP(n)) = bestFit - newSize;
    //printf("new_header is at %zu and new_free_node is at %zu and new_footer is at %zu\n",new_header,n,FTRP(n));
    //printf("new_header has a size of %zu\n",GET_SIZE(new_header));

    // Update the free list
    list_node *allocated_chunk  = (list_node *)p;
    list_node *replacement_chunk = (list_node *)n;
    
    if (allocated_chunk->next != NULL)
    {
      allocated_chunk->next->prev = replacement_chunk;
      replacement_chunk->next = allocated_chunk->next;
      allocated_chunk->next = NULL;
    }
    else
    {
      replacement_chunk->next = NULL;
    }

    if (allocated_chunk->prev != NULL)
    {
      allocated_chunk->prev->next = replacement_chunk;
      replacement_chunk->prev = allocated_chunk->prev;
      allocated_chunk->prev = NULL;
    }
    else
    {
      replacement_chunk->prev = NULL;
      free_list = replacement_chunk;
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
      old_node->next->prev = NULL;
      free_list = old_node->next;
      old_node->next = NULL;
    }
    else if (old_node->next == NULL && old_node->prev != NULL)  // Node is at the end of the free list
    {
      old_node->prev->next = NULL;
      old_node->prev = NULL;
    }
    else if (old_node->next == NULL && old_node->prev == NULL)
    {
      
    }
  }
  //printf("after: ");
  //print_free_list();
  
  //TODO: Make sure payload pointer is 16-byte aligned
  //TODO: Avoiding footers in allocated blocks
  
  return p;
}

/*
 * mm_free 
 */
void mm_free(void *ptr)
{
  //printf("attempting to free...\n");
  //GET_ALLOC(HDRP(ptr)) = 0x0;
  //mm_coalesce(ptr);
}

/*
 * coalesce - If a block-to-be-freed neighbors free blocks, coalesce them.
  */
static void mm_coalesce(void *pp)
{
  //printf("pp's head is at %zu and its footer is at %zu\n", HDRP(pp), FTRP(pp));
	list_node *back_neighbor = NULL;
	list_node *fwrd_neighbor = NULL;
  
	if (ptr_is_mapped(HDRP(PREV_BLKP(pp)), OVERHEAD) && !GET_ALLOC(HDRP(PREV_BLKP(pp))))
	{
		back_neighbor = (list_node *)PREV_BLKP(pp);
	}
	if (ptr_is_mapped(HDRP(NEXT_BLKP(pp)), OVERHEAD) && !GET_ALLOC(HDRP(NEXT_BLKP(pp))))
	{
		fwrd_neighbor = (list_node *)NEXT_BLKP(pp);
	}
	//printf("back_neighbor is %zu and fwrd_neighbor is %zu for %zu\n", back_neighbor, fwrd_neighbor, pp);
	
	if (fwrd_neighbor != NULL)
	{
		size_t old_size = GET_SIZE(HDRP(pp));
		//printf("pp's size is %zu. fwrd_neighbor's size is %zu. ", old_size, GET_SIZE(HDRP(fwrd_neighbor)));
		GET_SIZE(HDRP(pp)) += GET_SIZE(HDRP(fwrd_neighbor));
		GET_SIZE(FTRP(pp)) += old_size;
		//printf("And now pp's size is %zu.\n", GET_SIZE(HDRP(pp)));
	}
	if (back_neighbor != NULL)
	{
		size_t old_size = GET_SIZE(HDRP(back_neighbor));
		//printf("pp's size is %zu. back_neighbor's size is %zu. ", old_size, GET_SIZE(HDRP(back_neighbor)));
		GET_SIZE(HDRP(back_neighbor)) += GET_SIZE(HDRP(pp));
		GET_SIZE(FTRP(back_neighbor)) += old_size;
		//printf("And now back_neighbor's size is %zu.\n", GET_SIZE(HDRP(back_neighbor)));
	}
	
	if (back_neighbor == NULL)
	{
	  //print_free_list();
	  //printf("adding pp %zu to the head of the free list",pp);
	        list_node *free_chunk = (list_node *)pp;
		list_node *fl_head = free_list;
		
		free_chunk->next = fl_head;
		fl_head->prev = free_chunk;
		free_chunk->prev = NULL;
		free_list = free_chunk;
		//print_free_list();
	}
	if (fwrd_neighbor != NULL)
	{
		if (fwrd_neighbor->prev != NULL)
		{
			fwrd_neighbor->prev->next = fwrd_neighbor->next;
		}
		if (fwrd_neighbor->next != NULL)
		{
			fwrd_neighbor->next->prev = fwrd_neighbor->prev;
			if (fwrd_neighbor->prev == NULL)
			{
				free_list = fwrd_neighbor->next;
			}
		}
		fwrd_neighbor->prev = NULL;
		fwrd_neighbor->next = NULL;
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
  //printf("doing a check..\n");
  list_node *page_iterator = last_page;
  do
  {
    block_header *chunk_iterator = (block_header *)last_page + 2;

    // check prologue information
    if (GET_SIZE(HDRP(chunk_iterator)) != 0x30 || GET_ALLOC(HDRP(chunk_iterator)) != 0x1 || GET_SIZE(FTRP(chunk_iterator)) != 0x30)
    {
      //printf("prologue is broke\n");
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
        if ((fl_node->next == NULL && fl_node->prev == NULL) && free_list != fl_node)
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

static void print_free_list()
{
  list_node *iterator = free_list;
  printf("free_list --> ");
  while (iterator != NULL)
    {
      printf("%zu --> ",iterator);
      iterator = iterator->next;
    }
  printf("END\n");
}

static void print_mapped_pages()
{
  list_node *iterator = last_page;
  printf("last_page --> ");
  while (iterator != NULL)
    {
      printf("%zu --> ", iterator);
      iterator = iterator->next;
    }
  printf("END\n");
}
