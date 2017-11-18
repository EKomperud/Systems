#include <stdio.h>

/* A group within a script: */
typedef struct block_header {
	size_t size;					/* size of this block_header's payload + 2 */
} block_header;

typedef struct block_footer
{
	size_t size;					/* size of this block_footer's payload + 2 */
} block_footer;

extern int mm_init(void);
extern void *mm_malloc(size_t size);
extern void mm_free(void *ptr);
extern void mm_coalesce(void *pp);

extern int mm_check(void);
extern int mm_can_free(void *ptr);

