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
static int in_free_list(void* pp);
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
  //printf("init\n");
  while (mem_is_mapped(last_page,mem_pagesize()))
    {
      printf("address of last_page: %zu\n",last_page);
      size_t pageAlignTracker = (size_t *)last_page;
      //list_node *firstChunk = (list_node *)pageAlignTracker;
      pageAlignTracker += mem_pagesize();
      void *page = (void *)pageAlignTracker;
      printf("address of page: %zu\n",page);
      while (mem_is_mapped(page, mem_pagesize()))
	{
	  mem_unmap(page, mem_pagesize());
	  pageAlignTracker += mem_pagesize();
	  page = (void *)pageAlignTracker;
	  printf("address of page %zu\n",page);
	}
      //printf("address of last_page: %zu\n",last_page);
      last_page = last_page->prev;
      //printf("should match ^^^^^^^: %zu\n",last_page->next);
      mem_unmap(last_page->next, mem_pagesize());
    }
  debugCounter = 0;
  debugCounter2 = 0;
  lastAllocSize = (8 * mem_pagesize());
  free_list = NULL;
  last_page = NULL;
  size_t setupAddr = mem_map(8 * mem_pagesize());
  //printf("expand count: %zu\n", debugCounter2++);

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

  //printf("freelist: %zu\n", free_list);
  return 0;
}

/* 
 * mm_malloc - Allocate a block by iterating through the free list,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  //printf("malloc %zu\n", size);
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
  //print_free_list();
  while(iterator != NULL)
  {
    //printf("free counter = %d\n", freeCounter++);
    //printf("%zu <-- prev <-- %zu --> next --> %zu\n", iterator->prev, iterator, iterator->next);
    size_t thisSize = GET_SIZE(HDRP(iterator));
    //printf("thisSize:%zu vs newSize:%zu\n",thisSize,newSize);
    if (thisSize >= newSize)// && thisSize < bestFit)
    {
      if (thisSize > 200000000000000)
	{
	  print_free_list();
	}
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
    //printf("expand. no fit\n");
    size_t actualNeededSize = PAGE_ALIGN(newSize + (4 * ALIGNMENT));
    size_t allocSize = MAX(PAGE_ALIGN(actualNeededSize), (8 * mem_pagesize()));
    
    allocSize = MAX(allocSize, lastAllocSize);
    //printf("no fit found. allocate more memory of size %zu\n", allocSize);
    //printf("expand count: %d\n", debugCounter2++);
    if(allocSize >= (lastAllocSize >> 2))
      {
	lastAllocSize = allocSize * 2;
      }
    else {
      lastAllocSize = allocSize;
    }
    
    size_t setupAddr = (size_t)mem_map(allocSize);
    //printf("expand count: %zu\n", debugCounter2++);
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
  
  if ((bestFit - newSize) >=  (sizeof(list_node) + OVERHEAD))  // If there's leftover memory
  {
    //printf("leftover memory\n");
    // Set new header information
    GET_SIZE(HDRP(p)) = newSize;                         // Set header information for the newly allocated block
    GET_ALLOC(HDRP(p)) = 0x1;                            // Set the allocated status
    GET_SIZE(FTRP(p)) = newSize;                         // Set the footer pointer memory to footer
    size_t setupAddr = FTRP(p) + sizeof(block_footer);
    block_header *new_header = (block_header *)setupAddr;
    GET_SIZE(new_header) = bestFit - newSize;
    GET_ALLOC(new_header) = 0x0;
    setupAddr += sizeof(block_header);
    void *n = (void *)setupAddr;
    setupAddr += GET_SIZE(new_header) - OVERHEAD;
    block_footer *new_footer = (block_footer *)setupAddr;
    
    //if (!mm_check())
    //return 0;
    if (!ptr_is_mapped(new_footer, ALIGNMENT))
      {
	printf("%zu(h) --> %zu --> %zu(f)\n", new_header, GET_SIZE(new_header), new_footer);
	printf("header alloc: %zu\n", GET_ALLOC(new_header));
	printf("bestFit: %zu. newSize: %zu.\n",bestFit,newSize);
      }
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
    newSize = bestFit;
    GET_SIZE(HDRP(p)) = newSize;                         // Set header information for the newly allocated block
    GET_ALLOC(HDRP(p)) = 0x1;                            // Set the allocated status
    GET_SIZE(FTRP(p)) = newSize;                         // Set the footer pointer memory to footer
    //printf("no leftovers\n");
    list_node *old_node = (list_node *)p;
    if (old_node->next != NULL && old_node->prev != NULL)     // Node is in the middle of free list
    {
      //printf("node was in the middle of the free list\n");
      old_node->prev->next = old_node->next;
      old_node->next->prev = old_node->prev;
      old_node->prev = NULL;
      old_node->next = NULL;
    }
    else if (old_node->next != NULL && old_node->prev == NULL)  // Node is at the beginning of free list
    {
      //printf("node was the beginning of the free list\n");
      //print_free_list();
      //printf("old node = %zu. Is it the head of the free list? %d. What's it's ->next value? %zu\n", old_node, free_list = old_node, old_node->next);
      //printf("the newSize was %zu\n",newSize);
      //printf("previous block: %zu(%zu). next block: %zu(%zu).\n",HDRP(PREV_BLKP(p)),GET_SIZE(HDRP(PREV_BLKP(p))),HDRP(NEXT_BLKP(p)),GET_SIZE(HDRP(NEXT_BLKP(p))));
      old_node->next->prev = NULL;
      free_list = old_node->next;
      old_node->next = NULL;
      //print_free_list();
    }
    else if (old_node->next == NULL && old_node->prev != NULL)  // Node is at the end of the free list
    {
      //printf("node was the end of the free list\n");
      old_node->prev->next = NULL;
      old_node->prev = NULL;
    }
    else if (old_node->next == NULL && old_node->prev == NULL)  // Node IS the free list
    {
      //printf("node was the free list\n");
      //printf("free list: %zu\n", free_list);
      //printf("expand. no leftovers\n");
	  //size_t actualNeededSize = PAGE_ALIGN(newSize + (4 * ALIGNMENT));
      //size_t allocSize = MAX(PAGE_ALIGN(actualNeededSize), (8 * mem_pagesize()));
      size_t allocSize = MAX((8*mem_pagesize()), lastAllocSize);
      lastAllocSize = allocSize;
      size_t setupAddr = (size_t)mem_map(allocSize);
      //printf("%zu\n", setupAddr);
      //printf("expand count: %zu\n", debugCounter2++);
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
      GET_SIZE(initialFooter) = allocSize - (4 * ALIGNMENT);
      setupAddr += sizeof(block_footer);
    
      // Set up epiloge pointer
      block_header *epiloguePointer = (block_header *)setupAddr;
      GET_SIZE(epiloguePointer) = 0x0;
      GET_ALLOC(epiloguePointer) = 0x1;
    }
  }

  //printf("total Malloc'd: %d\n", debugCounter += newSize);
  GET_ALLOC((HDRP(p))) = 0x1;
  return p;
}

/*
 * mm_free 
 */
void mm_free(void *ptr)
{
  //printf("free\n");
  GET_ALLOC(HDRP(ptr)) = 0x0;
  mm_coalesce(ptr);
  //printf("did i at least make it here?\n");
}

/*
 * coalesce - If a block-to-be-freed neighbors free blocks, coalesce them.
  */
static void mm_coalesce(void *pp)
{
  //printf("freeing %zu\n",pp);
  //printf("free count: %d\n", debugCounter2++);
  list_node *freeing = (list_node *)pp;
	list_node *back_neighbor = NULL;
	size_t backAddr = HDRP(pp) - sizeof(block_footer);
	list_node *fwrd_neighbor = NULL;
	size_t fwrdAddr = FTRP(pp) + sizeof(block_footer);
  
	if (ptr_is_mapped(backAddr, ALIGNMENT))
	{
	  backAddr = backAddr - GET_SIZE(backAddr) + sizeof(block_footer);
	  if (GET_ALLOC(backAddr) == 0 && GET_SIZE(backAddr) > 0x20)
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
	  //printf("fwrd_n not null\n");
		size_t old_size = GET_SIZE(HDRP(pp));
		GET_SIZE(HDRP(pp)) += GET_SIZE(HDRP(fwrd_neighbor));
		GET_SIZE(FTRP(pp)) += old_size;		
	}
	if (back_neighbor != NULL)
	{
	  //printf("back_n not null\n");
		size_t old_size = GET_SIZE(HDRP(back_neighbor));		
		GET_SIZE(HDRP(back_neighbor)) += GET_SIZE(HDRP(pp));
		GET_SIZE(FTRP(back_neighbor)) += old_size;
	}
	
	if (back_neighbor == NULL)
	{
	  //printf("back_neighbor == NULL\n");
	        list_node *free_chunk = (list_node *)pp;
		list_node *fl_head = free_list;
		
		free_chunk->next = fl_head;
		fl_head->prev = free_chunk;
		free_chunk->prev = NULL;
		free_list = free_chunk;
	}
	else
	  {
	    freeing->next = NULL;
	    freeing->prev = NULL;
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
		
	//if (back_neighbor != NULL)
	//printf("back_neighbor should be in. Is it? %d. pp should not. Is pp in? %d\n",in_free_list(back_neighbor),in_free_list(pp));
	//else
	//printf("back_neighbor should not be in. Is it? %d. pp should. Is pp in? %d\n",in_free_list(back_neighbor),in_free_list(pp));
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
  //printf("UNALLOCATED BLOCKS --> ");
  list_node *page_iterator = last_page;
  size_t pageAddr = (size_t *)last_page;
  do
  {
    //printf("doing a check on a page\n");
    size_t pageSize = 64;
    size_t chunkAddr = pageAddr + sizeof(list_node);
    block_header *prologueHeader = (block_header *)chunkAddr;
    if (!ptr_is_mapped(prologueHeader, ALIGNMENT))
      {
	//printf("pointer(prologueHeader) isn't mapped\n");
	return 0;
      }
    chunkAddr += sizeof(block_header);
    block_footer *prologueFooter = (block_footer *)chunkAddr;
    if (!ptr_is_mapped(prologueFooter, ALIGNMENT))
      {
	//printf("pointer(prologueFooter) isn't mapped\n");
	return 0;
      }

    // check prologue information
    if (GET_SIZE(prologueHeader) != 0x20 || GET_ALLOC(prologueHeader) != 0x1 || GET_SIZE(prologueFooter) != 0x20)
    {
      //printf("prologue is broken\n");
      return 0;
    }

    char prev_alloc = 1;
    chunkAddr += sizeof(block_footer);
    block_header *chunk_iterator = (block_header *)chunkAddr;
    do
    {
      chunk_iterator = (block_header *)chunkAddr;
      size_t block_size = GET_SIZE(chunk_iterator);
      pageSize += block_size;
      char block_alloc  = GET_ALLOC(chunk_iterator);
      if (!ptr_is_mapped(chunk_iterator,block_size))
	{
	  //printf("pointer(chunk_iterator) isn't mapped\n");
	return 0;
	}
      if (!block_alloc && !prev_alloc)
      {
        //printf("failed coalesce\n");
        return 0;
      }
      prev_alloc = block_alloc;
      size_t footAddr = chunkAddr + GET_SIZE(chunk_iterator) - sizeof(block_footer);
      
      if (GET_SIZE(footAddr) != block_size)
      {
        //printf("inconsistent header/footers\n");
        return 0;
      }
      chunkAddr += sizeof(block_header);
      if (!block_alloc)
      {
        list_node *fl_node = (list_node *)chunkAddr;
        if ((fl_node->next == NULL && fl_node->prev == NULL) && free_list != fl_node)
        {
          //printf("free block isn't in the free list\n");
          return 0;
        }
	// TODO: make this more efficient
	//if (!in_free_list(fl_node))
	//{
	//  printf("Free node isn't in the free list\n");
	//  return 0;
	//}
	if (fl_node->next != NULL)
	  {
	    if (!ptr_is_mapped(fl_node->next,ALIGNMENT))
	    {
	      //printf("pointer(fl->next) isn't mapped\n");
	      return 0;
	    }
	    //if (fl_node->next->prev != fl_node)
	    //if (!in_free_list(fl_node))
	    //	{
	    //	  printf("Node %zu isn't connected foward properly\n",fl_node);
	    //	  return 0;
	    //	}
	  }
	if (fl_node->prev != NULL)
	  {
	    if (!ptr_is_mapped(fl_node->prev,ALIGNMENT))
	      {
	        //printf("pointer(fl->prev) isn't mapped\n");
	        return 0;
	      }
	    //if (fl_node->prev->next != fl_node)
	    //if (!in_free_list(fl_node))
	    //  {
	    //printf("Node %zu isn't connected back properly\n",fl_node);
	    //return 0;
	    //}
	  }
      }
      chunkAddr += GET_SIZE(chunk_iterator) - sizeof(block_header);

    } while (ptr_is_mapped(chunkAddr, OVERHEAD) && GET_SIZE(chunkAddr) != 0);
    if (pageSize % 4096 != 0)
      {
	//printf("header/footer sizes are inconsistent\n");
	return 0;
      }
    page_iterator = last_page->prev;
    if ( (size_t)page_iterator % 4096 != 0 )
      return 0;
  } while (page_iterator != NULL && mem_is_mapped(page_iterator,mem_pagesize()));

  //Check the free list:
  list_node *fli = free_list;
  while (fli != NULL)
    {
      if (!ptr_is_mapped(fli, ALIGNMENT))
	return 0;
      block_header *flh = HDRP(fli);
      block_footer *flf = FTRP(fli);
      if (!ptr_is_mapped(flh, GET_SIZE(flh)))
	return 0;
      if (GET_SIZE(flh) != GET_SIZE(flf))
	return 0;
      fli = fli->next;
    }
  
  return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{
  block_header *header = HDRP(p);
  block_footer *footer = FTRP(p);
  if (!ptr_is_mapped(header,ALIGNMENT) || !ptr_is_mapped(footer,ALIGNMENT))
    return 0;
  if (GET_SIZE(header) != GET_SIZE(footer))
    return 0;
  block_header *prev_header = HDRP(PREV_BLKP(p));
  block_header *next_header = HDRP(NEXT_BLKP(p));
  if (!ptr_is_mapped(prev_header,ALIGNMENT) || !ptr_is_mapped(next_header,ALIGNMENT))
    return 0;
  if ((GET_ALLOC(prev_header) != 1 && GET_ALLOC(prev_header) != 0) || (GET_ALLOC(next_header) != 1 && GET_ALLOC(next_header) != 0))
    return 0;
  return (GET_ALLOC(HDRP(p)) && !in_free_list(p));
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

static int in_free_list(void* pp)
{
  list_node *match = (list_node *)pp;
  list_node *iterator = free_list;
  while (iterator != NULL)
    {
      if (pp == iterator)
	return 1;
      iterator = iterator->next;
    }
  return 0;
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
