
1. Using memlib
---------------

See "main.c", which uses the memlib API that is part of the malloc
assignment. The initial "main.c" allocates 10 pages of memory, writes
a 10 to the first 4 bytes, reads that 10 back out and prints it, and
releases the 10 pages of memory.

Use `make` and `./main` to try it out.

(If you get stuck anywhere, "main_done.c" has all of the code frmo the
rest of this exercise.)


2. Trying to use an Unmapped Page
---------------------------------

Change "main.c" so that it tries to read and print the 10 after `p` is
unmapped. You should get a seg fault.


3. Guarding Against Access to an Unmapped Page
----------------------------------------------

The memlib API includes a `mem_is_mapped` function that you can use to
check whether a page is still mapped. The `mem_is_mapped` function
takes an address and a size, and it makes sure that all of the
addresses from the given address (inclusive) to the given address plus
size (exclusive) are mapped.

For example, if you change the starting code in `main` to

   size_t sz = 10 * mem_pagesize();
   void *p = mem_map(sz);

   printf("%ld mapped? %d\n", sz, mem_is_mapped(p, sz));
   printf("%ld mapped? %d\n", 2 * sz, mem_is_mapped(p, 2 * sz));

then the output will report that 10 pages are mapped, but 20 pages are
not all mapped. Try that out.

If you add a similar printout after the `mem_unmap` call, then you
should get a report that the 10 pages are not mapped after the
`mem_unmap`:

  mem_unmap(p, sz);

  printf("%ld mapped? %d\n", sz, mem_is_mapped(p, sz));


4. Random Chaos
---------------

We're eventually going to use `mem_is_mapped` to guard against chaos
that mangles a linked list of pages. First, let's try some chaos.

The makefile not only link memlib to "main.c", it also links a chaos
library that provides two functions:

 void random_chaos(void);
 void seed_chaos(int v);

The `random_chaos` function randomly changes changes on pages that
were allocated by `mem_map`. If you use `random_chaos` once, it's
unlikely to change any particular byte. For example,

  *(int *)p = 10;
  random_chaos();
  printf("%d\n", *(int *)p);

is likely to still print 10. But 100,000 chatic events is likely to
make some change. Try

  *(int *)p = 10;
  {
    int i;
    for (i = 0; i < 100000; i++)
      random_chaos();
  }
  printf("%d\n", *(int *)p);

and you'll almost certainly see a value other than 10.

We can also measure how long it takes to chaos to change the 10:

  *(int *)p = 10;
  {
    int count = 0;
    while (*(int *)p == 10) {
      random_chaos();
      count++;
    }
    printf("mangled after %d\n", count);
  }

What number do you get? What if you run `./main` again?

The random chaos isn't actually random. The `seed_chaos` function let
you pick a different "random" stream of chaos. Try adding

  seed_chaos(42);

before the loop and see how the output changes, and try some numbers
in place of 42.


5. Allocating a Linked List
---------------------------

Here are functions that allocate a linked list of pages, with string
content on each page, plus a function that counts instances of a
string in a list:
  
  #define DATA_SIZE 100
  
  typedef struct chunk {
    struct chunk *next;
    char data[DATA_SIZE];
  } chunk;
  
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
    
  int page_list_count(chunk *p, char *find)
  {
    int count = 0;
    
    while (p != NULL) {
      if (!strcmp(p->data, find))
        count++;
      p = p->next;
    }
    
    return count;
  }

Obviously, if you make a list of 10 "apples" `make_page_list`, then
checking for "apple" with `page_list_count` shoud return 10:

  p = make_page_list(10, "apple");
  printf("%d\n", page_list_count(p, "apple"));

Note that `sizeof(chunk)` is much smaller than `mem_pagesize()`, but
we have to allocate at least one page at a time when using `mem_map`.
When writing your own memory allocation, you might use something like
`chunk`, but make better use of the space in the rest of the page (or
pages) after `next`.


6. Chaos and a Linked List
--------------------------

Add 100,000 chaos events between the list allocation and sum check.
What happens?

Probably you didn't just get a mangled "apple" count, but a seg fault.
Why?

Measure how many times `random_chaos` is required to change the list
content. For different chaos seeds, you'll likely get a bad count
before a seg fault, while other seeds may cause crashes.

The malloc assignment includes a similar kind of chaos, and part of
your job is to detect it. Detecting chaos involves both detecting
whether data like "apple" has gone bad and making sure that memory
references are sensible.


7. A Cautious List Count
------------------------

A seg fault happens when `page_list_count` tries to follow a mangled
`->next` pointer. To guard against a crash, the `page_list_count`
function can check whether a part of memory that it's about to read is
valid as mapped memory.

The `mem_is_mapped` function from memlib is inconvenient for that
purposes, because it requires aligned addresses and sizes. Here's a
`ptr_is_mapped` function from the malloc assignment that is more
convenient:

  #define ADDRESS_PAGE_START(p) ((void *)(((size_t)p) & ~(mem_pagesize()-1)))

  int ptr_is_mapped(void *p, size_t len) {
    void *s = ADDRESS_PAGE_START(p);
    return mem_is_mapped(s, PAGE_ALIGN((p + len) - s));
  }

Using this function, `page_list_count` can avoid crashing after
any kind of chaos:

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

Try using this function with different kinds of chaos.

In this example, a single `ptr_is_mapped` call per list connection is
enough to check the validity of the list. When the data on a list
itself includes pointers, more `ptr_is_mapped` calls are needed.
