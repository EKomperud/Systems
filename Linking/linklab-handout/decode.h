#include <elf.h>

/* A `code_t*` is represented as `unsigned char *`, because that's
   convenient for decoding bytes, and because pointer arithmetic is in
   bytes. */
typedef unsigned char code_t;

typedef enum {
  MOV_ADDR_TO_REG_OP,
  JMP_TO_ADDR_OP,
  MAYBE_JMP_TO_ADDR_OP,
  RET_OP,
  OTHER_OP,
} instruction_op_t;

typedef struct {
  instruction_op_t op; /* operation performed by the instruction */
  int length; /* length of the instruction in bytes */
  /* Additional info, if any, depends on the operation: */
  union {
    struct {
      /* no info */
      Elf64_Addr addr;
    } mov_addr_to_reg;
    struct {
      Elf64_Addr addr;
    } jmp_to_addr;
    struct {
      Elf64_Addr addr;
    } maybe_jmp_to_addr;
    struct {
      /* no info */
    } ret;
    struct {
      /* no info */
    } other;
  };
} instruction_t;

/* Fill `*ins` with information about the instruction at `code_ptr`,
   where `code_addr` is the address where the code will reside at run
   time. */
void decode(instruction_t *ins, code_t *code_ptr, Elf64_Addr code_addr);

/* Changes the instruction at `code_ptr` so that it's a crash
   (followed by filler no-ops). The `ins` argument determines how many
   bytes are replaced. */
void replace_with_crash(code_t *code_ptr, instruction_t *ins);
