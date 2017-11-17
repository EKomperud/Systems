#include <stdio.h>
#include <string.h>
#include "memlib.h"
#include "chaos.h"

#define DATA_SIZE 100

typedef struct chunk {
  struct chunk *next;
  char data[DATA_SIZE];
} chunk;

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(sz) (((sz) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

chunk *make_page_list(int len, char *content)
{
  void *p = NULL;
  int i;
    
  for (i = 0; i < len; i++) {
    chunk *more = mem_map(PAGE_ALIGN(sizeof(chunk)));
    more->next = p;
    strcpy(more->data, content);
    p = more;
  }
  
  return p;
}
  
#define ADDRESS_PAGE_START(p) ((void *)(((size_t)p) & ~(mem_pagesize()-1)))

int ptr_is_mapped(void *p, size_t len) {
  void *s = ADDRESS_PAGE_START(p);
  return mem_is_mapped(s, PAGE_ALIGN((p + len) - s));
}

int page_list_count(chunk *p, char *find)
{
  int count = 0;
  
  while (p != NULL) {
    if (!ptr_is_mapped(p, sizeof(chunk)))
      return -1; /* give up */
    if (!strcmp(p->data, find))
      count++;
    p = p->next;
  }
  
  return count;
}

int main()
{
	chunk *p;
	p = make_page_list(11, "apple");

	int i = 0;
	while(page_list_count(p, "apple") == 11)
	{
		random_chaos();
		i++;
	}

	printf("%d\n", i);
	printf("%d\n", sizeof(int));
  
  	return 0;
}
