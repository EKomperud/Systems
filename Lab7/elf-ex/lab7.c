#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <elf.h>

/* Given the in-memory ELF header pointer as `ehdr` and a section
   header pointer as `shdr`, returns a pointer to the memory that
   contains the in-memory content of the section */
#define AT_SEC(ehdr, shdr) ((void *)(ehdr) + (shdr)->sh_offset)

static void check_for_shared_object(Elf64_Ehdr *ehdr);
static void fail(char *reason, int err_code);
void print_symbols(Elf64_Ehdr* ehdr);

int main(int argc, char **argv) 
{
  int fd;
  size_t len;
  void *p;
  Elf64_Ehdr *ehdr;

  if (argc != 2)
    fail("expected one file on the command line", 0);

  /* Open the shared-library file */
  fd = open(argv[1], O_RDONLY);
  if (fd == -1)
    fail("could not open file", errno);

  /* Find out how big the file is: */
  len = lseek(fd, 0, SEEK_END);

  /* Map the whole file into memory: */
  p = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
  if (p == (void*)-1)
    fail("mmap failed", errno);

  /* Since the ELF file starts with an ELF header, the in-memory image
     can be cast to a `Elf64_Ehdr *` to inspect it: */
  ehdr = (Elf64_Ehdr *)p;

  /* Check that we have the right kind of file: */
  check_for_shared_object(ehdr);

  /* Your work for parts 1-6 goes here */

  /* Part 1 */
  int sectionTableOffset = ehdr->e_shoff;
  printf("%d\n", sectionTableOffset);
  int indexToSectionNameStrings = ehdr->e_shstrndx;
  printf("%d\n", indexToSectionNameStrings);

  /* Part 2 */
  Elf64_Shdr  *sectionHeaders = (void *)ehdr + ehdr->e_shoff;
  char* strings = AT_SEC(ehdr, sectionHeaders + indexToSectionNameStrings) + 1;
  char *strs = (void*)ehdr+sectionHeaders[ehdr->e_shstrndx].sh_offset;
  printf(strings);
  printf("\n");

  /* Part 3 & 4 */;
  int sectionHeadersLen = ehdr->e_shnum;
  int i;
  
  for (i = 0; i<sectionHeadersLen; i++) {
    Elf64_Shdr nextHeader = sectionHeaders[i];
    if (strcmp((strs + nextHeader.sh_name),".data") == 0) {
      printf("%d\n",i);
    }
    printf("%d %s\n", i, strs + nextHeader.sh_name);
  }

  /* Part 5 + 6*/
  for (i = 0; i<sectionHeadersLen; i++) {
    Elf64_Shdr nextHeader = sectionHeaders[i];
    int dynsymsF0 = 0;
    int textShAddr = 0;
    if (strcmp((strs + nextHeader.sh_name),".dynsym") == 0) {
      //Elf64_Sym* text = (void *)ehdr + nextHeader.sh_addr;
      Elf64_Sym* dynsyms = (void *)ehdr + nextHeader.sh_offset;
      printf("%p\n", dynsyms[7].st_value);
      dynsymsF0 = synsyms[7].st_value;
    }
    if (strcmp((strs + nextHeader.sh_name),".text") == 0) {
      //printf("%x\n",nextHeader.sh_offset);
      //Elf64_Sym* dynsyms = AT_SEC(ehdr, sectionHeaders + nextHeader.sh_offset);
      //Elf64_Sym* dynsyms = (void *)ehdr + nextHeader.sh_offset;
      textShAddr = nextHeader.sh_addr;
      printf("%p\n", nextHeader.sh_addr);
    }
    //printf("%d %s\n", i, strs + nextHeader.sh_name);
  }

  /* Part 6 */
  for (i = 0; i<sectionHeadersLen; i++) {
    Elf64_Shdr nextHeader = sectionHeaders[i];
    if (strcmp((strs + nextHeader.sh_name),".text") == 0) {
      //printf("%x\n",nextHeader.sh_offset);
      //Elf64_Sym* dynsyms = AT_SEC(ehdr, sectionHeaders + nextHeader.sh_offset);
      //Elf64_Sym* dynsyms = (void *)ehdr + nextHeader.sh_offset;
      printf("%p\n", nextHeader.sh_addr);
    }
    //printf("%d %s\n", i, strs + nextHeader.sh_name);
  }

  return 0;
}

/* Just do a little bit of error-checking:
   Make sure we're dealing with an ELF file. */
static void check_for_shared_object(Elf64_Ehdr *ehdr) 
{
  if ((ehdr->e_ident[EI_MAG0] != ELFMAG0)
      || (ehdr->e_ident[EI_MAG1] != ELFMAG1)
      || (ehdr->e_ident[EI_MAG2] != ELFMAG2)
      || (ehdr->e_ident[EI_MAG3] != ELFMAG3))
    fail("not an ELF file", 0);

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    fail("not a 64-bit ELF file", 0);

  if (ehdr->e_type != ET_DYN)
    fail("not a shared-object file", 0);
}

static void fail(char *reason, int err_code) 
{
  fprintf(stderr, "%s (%d)\n", reason, err_code);
  exit(1);
}
