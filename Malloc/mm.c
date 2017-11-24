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
size_t lastAllocSize = 0;
char debugBool = 0;
int debugCounter = 0;
int debugCounter2 = 0;

/* 
 * mm_init - initialize the malloc package with 8 pages.
 */
int mm_init(void)
{
  debugCounter = 0;
  debugCounter2 = 0;
  size_t setupAddr = mem_map(8 * mem_pagesize());

  list_node *pageNode = (list_node *)setupAddr;              // Set up page pointer
  pageNode->next = NULL;
  pageNode->prev = NULL;
  last_page = pageNode;
  setupAddr += sizeof(list_node);
  
  // Set up prologue
  block_header *prologueHeader = (block_header *)setupAddr;
  GET_SIZE(prologueHeader) = 0x20;
  GET_ALLOC(prologueHeader) = 0x1;
  setupAddr += sizeof(block_header);
  block_footer *prologueFooter = (block_footer *)setupAddr;
  GET_SIZE(prologueFooter) = 0x20;
  setupAddr += sizeof(block_footer);
  
  // Set up initial chunk
  block_header *initialHeader = (block_header *)setupAddr;
  GET_SIZE(initialHeader) = ((8 * mem_pagesize()) - (4 * ALIGNMENT));
  GET_ALLOC(initialHeader) = 0x0;
  setupAddr += sizeof(block_header);
  list_node *fl_node = (list_node *)setupAddr;
  fl_node->next = NULL;
  fl_node->prev = NULL;
  free_list = fl_node;
  setupAddr = setupAddr + GET_SIZE(initialHeader) - sizeof(block_footer) - sizeof(list_node);
  block_footer *initialFooter = (block_footer *)setupAddr;
  GET_SIZE(initialFooter) = GET_SIZE(initialHeader);
  setupAddr += sizeof(block_footer);
  
  // Set up epiloge pointer
  block_header *epiloguePointer = (block_header *)setupAddr;
  GET_SIZE(epiloguePointer) = 0x0;
  GET_ALLOC(epiloguePointer) = 0x1;

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
  //print_free_list();
  //printf("malloc count: %d\n", debugCounter++);
  size_t needSize = MAX(size, sizeof(list_node));
  size_t newSize = ALIGN(needSize + OVERHEAD);
  size_t bestFit = -1;
  void *p;
  //printf("allocating memory of size %zu\n", newSize);

  char foundFit = 0;
  list_node *bestNode = free_list;
  list_node *iterator = free_list;
  //printf("before: ");
  //print_free_list();
  int freeCounter = 0;
  while(iterator != NULL)
  {
    //printf("free counter = %d\n", freeCounter++);
    //printf("%zu <-- prev <-- %zu --> next --> %zu\n", iterator->prev, iterator, iterator->next);
    size_t thisSize = GET_SIZE(HDRP(iterator));
    if (thisSize >= newSize)// && thisSize < bestFit)
    {
      bestFit = thisSize;
      bestNode = iterator;
      foundFit = 1;
      //if (thisSize < (newSize * 2))
      break;
    }
    iterator = iterator->next;
  }

  if (!foundFit)
  {
    //printf("no fit found. allocate more memory\n");
    size_t actualNeededSize = PAGE_ALIGN(newSize + (4 * ALIGNMENT));
    size_t allocSize = MAX(PAGE_ALIGN(actualNeededSize), (8 * mem_pagesize()));
    allocSize = MAX(allocSize, lastAllocSize);
    lastAllocSize = allocSize;
    size_t setupAddr = (size_t)mem_map(allocSize);
    //printf("setupAddr starts at %zu and goes to %zu\n",setupAddr, setupAddr + allocSize);
    
    // Set up page pointer
    //printf("pageNode is at %zu\n", setupAddr);
    list_node *pageNode = (list_node *)setupAddr;
    pageNode->prev = NULL;
    pageNode->next = last_page;
    pageNode->next->prev = pageNode;
    last_page = pageNode;
    setupAddr += sizeof(list_node);

    // Set up prologue
    //printf("prologueHeader is at %zu\n", setupAddr);
    block_header *prologueHeader = (block_header *)setupAddr;
    GET_SIZE(prologueHeader) = 0x20;
    GET_ALLOC(prologueHeader) = 0x1;
    setupAddr += sizeof(block_header);
    //printf("prologueFooter is at %zu\n", setupAddr);
    block_footer *prologueFooter = (block_footer *)setupAddr;
    GET_SIZE(prologueFooter) = 0x20;
    setupAddr += sizeof(block_footer);

    // Set up initial chunk
    //printf("initialHeader is at %zu\n", setupAddr);
    block_header *initialHeader = (block_header *)setupAddr;
    GET_SIZE(initialHeader) = allocSize - (4 * ALIGNMENT);
    GET_ALLOC(initialHeader) = 0x0;
    setupAddr += sizeof(block_header);
    list_node *fl_node = (list_node *)setupAddr;
    if (free_list != NULL)
    {
      list_node *fl_head = free_list;
      fl_node->next = fl_head;
      fl_node->next->prev = fl_node;
      fl_node->prev = NULL;
      free_list = fl_node;
    }
    else
    {
      fl_node->next = NULL;
      fl_node->prev = NULL;
      free_list = fl_node;
    }
    setupAddr += GET_SIZE(initialHeader) - sizeof(block_footer) - sizeof(list_node);
    block_footer *initialFooter = (block_footer *)setupAddr;
    GET_SIZE(initialFooter) = allocSize - (5 * ALIGNMENT);
    setupAddr += sizeof(block_footer);
    
    // Set up epiloge pointer
    block_header *epiloguePointer = (block_header *)setupAddr;
    GET_SIZE(epiloguePointer) = 0x0;
    GET_ALLOC(epiloguePointer) = 0x1;
    
    bestNode = (void *)fl_node;
    bestFit = GET_SIZE(HDRP(bestNode));

    //Extend sanity checker
    //printf("pageNode is at %zu. prologue should be 32 above that: %zu. Free list should be 48 above that: %zu.", pageNode, prologue, free_list);
    //printf("Epilogue should be 8*page_size - 16 after pageNode: %zu", epiloguePointer );
  }

  p = bestNode;
  GET_SIZE(HDRP(p)) = newSize;                         // Set header information for the newly allocated block
  GET_ALLOC(HDRP(p)) = 0x1;                            // Set the allocated status
  GET_SIZE(FTRP(p)) = newSize;                         // Set the footer pointer memory to footer
  //printf("bestFit: %zu. newSize: %zu\n",bestFit, newSize);
  
  if ((bestFit - newSize) >=  (sizeof(list_node) + OVERHEAD))  // If there's leftover memory
  {
    
    // Set new header information
    size_t setupAddr = FTRP(p) + sizeof(block_footer);
    block_header *new_header = (block_header *)setupAddr;
    GET_SIZE(new_header) = bestFit - newSize;
    GET_ALLOC(new_header) = 0x0;
    setupAddr += sizeof(block_header);
    void *n = (void *)setupAddr;
    setupAddr += GET_SIZE(new_header) - OVERHEAD;
    block_footer *new_footer = (block_footer *)setupAddr;
    GET_SIZE(new_footer) = bestFit - newSize;

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
      //printf("update ->next\n");
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
      //printf("update ->prev\n");
      replacement_chunk->prev = NULL;
      free_list = replacement_chunk;
    }
  }
  
  else                                                        // If there's no leftover memory
  {
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
    else if (old_node->next == NULL && old_node->prev == NULL)  // Node IS the free list
    {
	  //size_t actualNeededSize = PAGE_ALIGN(newSize + (4 * ALIGNMENT));
      //size_t allocSize = MAX(PAGE_ALIGN(actualNeededSize), (8 * mem_pagesize()));
      size_t allocSize = MAX((8*mem_pagesize()), lastAllocSize);
      lastAllocSize = allocSize;
      size_t setupAddr = (size_t)mem_map(allocSize);
      //printf("setupAddr starts at %zu and goes to %zu\n",setupAddr, setupAddr + allocSize);
    
      // Set up page pointer
      //printf("pageNode is at %zu\n", setupAddr);
      list_node *pageNode = (list_node *)setupAddr;
      pageNode->prev = NULL;
      pageNode->next = last_page;
      pageNode->next->prev = pageNode;
      last_page = pageNode;
      setupAddr += sizeof(list_node);

      // Set up prologue
      //printf("prologueHeader is at %zu\n", setupAddr);
      block_header *prologueHeader = (block_header *)setupAddr;
      GET_SIZE(prologueHeader) = 0x20;
      GET_ALLOC(prologueHeader) = 0x1;
      setupAddr += sizeof(block_header);
      //printf("prologueFooter is at %zu\n", setupAddr);
      block_footer *prologueFooter = (block_footer *)setupAddr;
      GET_SIZE(prologueFooter) = 0x20;
      setupAddr += sizeof(block_footer);

      // Set up initial chunk
      //printf("initialHeader is at %zu\n", setupAddr);
      block_header *initialHeader = (block_header *)setupAddr;
      GET_SIZE(initialHeader) = allocSize - (4 * ALIGNMENT);
      GET_ALLOC(initialHeader) = 0x0;
      setupAddr += sizeof(block_header);
      list_node *fl_node = (list_node *)setupAddr;
      fl_node->next = NULL;
      fl_node->prev = NULL;
      free_list = fl_node;
      setupAddr += GET_SIZE(initialHeader) - sizeof(block_footer) - sizeof(list_node);
      block_footer *initialFooter = (block_footer *)setupAddr;
      GET_SIZE(initialFooter) = allocSize - (5 * ALIGNMENT);
      setupAddr += sizeof(block_footer);
    
      // Set up epiloge pointer
      block_header *epiloguePointer = (block_header *)setupAddr;
      GET_SIZE(epiloguePointer) = 0x0;
      GET_ALLOC(epiloguePointer) = 0x1;
    }
  }
  
  return p;
}

/*
 * mm_free 
 */
void mm_free(void *ptr)
{
  GET_ALLOC(HDRP(ptr)) = 0x0;
  mm_coalesce(ptr);
}

/*
 * coalesce - If a block-to-be-freed neighbors free blocks, coalesce them.
  */
static void mm_coalesce(void *pp)
{
  //printf("free count: %d\n", debugCounter2++);
	list_node *back_neighbor = NULL;
	size_t backAddr = HDRP(pp) - sizeof(block_footer);
	//backAddr = backAddr - GET_SIZE(backAddr) + sizeof(block_footer);
	list_node *fwrd_neighbor = NULL;
	size_t fwrdAddr = FTRP(pp) + sizeof(block_footer);
  
	if (ptr_is_mapped(backAddr, ALIGNMENT))
	{
	  backAddr = backAddr - GET_SIZE(backAddr) + sizeof(block_footer);
	  if (GET_ALLOC(backAddr) == 0 && GET_SIZE(backAddr) != 0x20)
	  {
	      backAddr += sizeof(block_header);
	      back_neighbor = (list_node *)backAddr;
	  }
	}
	if (ptr_is_mapped(fwrdAddr, OVERHEAD))
	{
	  block_header *fwrdHeader = (block_header *)fwrdAddr;
	  if (ptr_is_mapped(fwrdHeader, GET_SIZE(fwrdHeader)) && GET_ALLOC(fwrdHeader) == 0 && GET_SIZE(fwrdHeader) != 0)
	  {
	    fwrdAddr += sizeof(block_header);
	    fwrd_neighbor = (list_node *)fwrdAddr;
	    if ((!ptr_is_mapped(fwrd_neighbor->next, ALIGNMENT) && !ptr_is_mapped(fwrd_neighbor->prev, ALIGNMENT)) && free_list != fwrd_neighbor)
	      fwrd_neighbor = NULL;
	  }
	}
	
	if (fwrd_neighbor != NULL)
	{
		size_t old_size = GET_SIZE(HDRP(pp));
		GET_SIZE(HDRP(pp)) += GET_SIZE(HDRP(fwrd_neighbor));
		GET_SIZE(FTRP(pp)) += old_size;		
	}
	if (back_neighbor != NULL)
	{
		size_t old_size = GET_SIZE(HDRP(back_neighbor));		
		GET_SIZE(HDRP(back_neighbor)) += GET_SIZE(HDRP(pp));
		GET_SIZE(FTRP(back_neighbor)) += old_size;
	}
	
	if (back_neighbor == NULL)
	{
	        list_node *free_chunk = (list_node *)pp;
		list_node *fl_head = free_list;
		
		free_chunk->next = fl_head;
		fl_head->prev = free_chunk;
		free_chunk->prev = NULL;
		free_list = free_chunk;
	}
	//else
	//{
	//  list_node *free_chunk = (list_node *)pp;
	//  free_chunk->next = NULL;
	//  free_chunk->prev = NULL;
	//}
	
	if (fwrd_neighbor != NULL)
	{
		if (fwrd_neighbor->prev != NULL)
		{
		  
		  //print_free_list();
		  //printf("fwrd_neighbor is at %zu. Its prev is %zu. Its next is %zu\n", fwrd_neighbor, fwrd_neighbor->prev, fwrd_neighbor->next);
			fwrd_neighbor->prev->next = fwrd_neighbor->next;
		}
		if (fwrd_neighbor->next != NULL)
		{
		  //printf("coalesce count: %d\n", debugCounter2++);
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
      printf("prologue is broken\n");
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
      printf("%zu(%zu) --> ",iterator, GET_SIZE(HDRP(iterator)));
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
