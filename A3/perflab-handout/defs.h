/*
 * driver.h - Various definitions for the Performance Lab.
 * 
 * DO NOT MODIFY ANYTHING IN THIS FILE
 */
#ifndef _DEFS_H_
#define _DEFS_H_

#include <stdlib.h>

#define RIDX(i,j,n) (1+(i)*(n)+(j))

typedef struct {
  char *name, *email;
} student_t;

extern student_t student;

typedef union {
  struct {
    unsigned short red;
    unsigned short green;
    unsigned short blue;
  };
  int dim;
} pixel;

typedef void (*pinwheel_test_func) (pixel*, pixel*);
typedef void (*motion_test_func) (pixel*, pixel*);

void pinwheel(pixel *, pixel *);
void motion(pixel *, pixel *);

void register_pinwheel_functions(void);
void register_motion_functions(void);
void add_pinwheel_function(pinwheel_test_func, char*);
void add_motion_function(motion_test_func, char*);

#endif /* _DEFS_H_ */

