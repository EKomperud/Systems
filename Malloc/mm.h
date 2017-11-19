#include <stdio.h>

/* A group within a script: */
typedef struct block_header 
{
	size_t size;					/* size of this block_header's payload + 2 */
	char allocated;
} block_header;

typedef struct block_footer
{
	size_t size;					/* size of this block_footer's payload + 2 */
	int filler;
} block_footer;

typedef struct list_node
{
	struct list_node *next;
	struct list_node *prev;
} list_node;

extern int mm_init(void);
extern void *mm_malloc(size_t size);
extern void mm_free(void *ptr);

extern int mm_check(void);
extern int mm_can_free(void *ptr);

