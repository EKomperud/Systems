#include <stdlib.h>
#include "pagemap.h"
#include "memlib.h"
#include "chaos.h"

static int mangle_pageno;
static size_t mangle_offset;
static size_t mangle_len;
static char mangle_v;

static void mangle_page(void *addr) {
  if (mangle_pageno == 0) {
    size_t len = mangle_len;
    size_t c = mangle_offset;
    
    while (len--) {
      ((char *)addr)[c++] = mangle_v;
    }

    mangle_pageno = random() % 16;
  } else
    mangle_pageno--;
}

void random_chaos(void)
{
  /* Mangle multiple pages at the same place */
  mangle_offset = random() % mem_pagesize();
  mangle_len = random() % 8;
  if (mangle_len + mangle_offset > mem_pagesize())
    mangle_len = mem_pagesize() - mangle_offset;  
  mangle_v = random() % 256;

  mangle_pageno = random() % 16;
  pagemap_for_each(mangle_page, 0);
}

void seed_chaos(int v)
{
  srandom(v);
}
