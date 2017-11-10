#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * Please fill in the following student struct 
 */
student_t student = {
  "Eric Komperud",     /* Full name */
  "u0844210@utah.edu",  /* Email address */

};

/******************************************************
 * PINWHEEL KERNEL
 *
 * Your different versions of the pinwheel kernel go here
 ******************************************************/

/*
 * v1_pinwheel - First Try!
 */
char v1_pinwheel_descr[] = "v1_pinwheel: First Try!";
void v1_pinwheel(pixel *src, pixel *dest)
{
  int qi, qj, i, j, ii, jj;

  /* qi & qj are column and row of quadrant
     i & j are column and row within quadrant */

  /* Loop over 4 quadrants: */
  int dims = src->dim;
  int quad_size = dims >> 1;
  for (qi = 0; qi < 2; qi++)
    for (qj = 0; qj < 2; qj++) {
      int qj_offset = qj * quad_size;
      int qi_offset = qi * quad_size;
      /* Loop within quadrant: */
      for (i = 0; i < quad_size; i+=16)
        for (j = 0; j < quad_size; j+=16) {
	  for (ii = i; ii < i+16; ii+=4) {
	    for (jj = j; jj < j+16; jj++) {
	      int s_idx_1 = RIDX(qj_offset + ii,
				 jj + qi_offset, dims);
	      int s_idx_2 = RIDX(qj_offset + ii + 1,
				 jj + qi_offset, dims);
	      int s_idx_3 = RIDX(qj_offset + ii + 2,
				 jj + qi_offset, dims);
	      int s_idx_4 = RIDX(qj_offset + ii + 3,
				 jj + qi_offset, dims);
	  
	      int d_idx_1 = RIDX(qj_offset + quad_size - 1 - jj,
				 ii + qi_offset, dims);
	      int d_idx_2 = RIDX(qj_offset + quad_size - 1 - jj,
				 ii + 1 + qi_offset, dims);
	      int d_idx_3 = RIDX(qj_offset + quad_size - 1 - jj,
				 ii + 2 + qi_offset, dims);
	      int d_idx_4 = RIDX(qj_offset + quad_size - 1 - jj,
				 ii + 3 + qi_offset, dims);
			     
	  
	      pixel *d_idx_p_1 = & dest[d_idx_1];
	      pixel *d_idx_p_2 = & dest[d_idx_2];
	      pixel *d_idx_p_3 = & dest[d_idx_3];
	      pixel *d_idx_p_4 = & dest[d_idx_4];
	  
	      pixel s_idx_p_1 = src[s_idx_1];
	      pixel s_idx_p_2 = src[s_idx_2];
	      pixel s_idx_p_3 = src[s_idx_3];
	      pixel s_idx_p_4 = src[s_idx_4];
	  
	      int avg_1 = (s_idx_p_1.red
			   + s_idx_p_1.green
			   + s_idx_p_1.blue) / 3;
	      int avg_2 = (s_idx_p_2.red
			   + s_idx_p_2.green
			   + s_idx_p_2.blue) / 3;
	      int avg_3 = (s_idx_p_3.red
			   + s_idx_p_3.green
			   + s_idx_p_3.blue) / 3;
	      int avg_4 = (s_idx_p_4.red
			   + s_idx_p_4.green
			   + s_idx_p_4.blue) / 3;
	  
	      d_idx_p_1[0].red = avg_1;
	      d_idx_p_1[0].green = avg_1;
	      d_idx_p_1[0].blue = avg_1;
	      d_idx_p_2[0].red = avg_2;
	      d_idx_p_2[0].green = avg_2;
	      d_idx_p_2[0].blue = avg_2;
	      d_idx_p_3[0].red = avg_3;
	      d_idx_p_3[0].green = avg_3;
	      d_idx_p_3[0].blue = avg_3;
	      d_idx_p_4[0].red = avg_4;
	      d_idx_p_4[0].green = avg_4;
	      d_idx_p_4[0].blue = avg_4;
	    }
	  }
        }
    }
}

/* 
 * naive_pinwheel - The naive baseline version of pinwheel 
 */
char naive_pinwheel_descr[] = "naive_pinwheel: baseline implementation";
void naive_pinwheel(pixel *src, pixel *dest)
{
  int qi, qj, i, j;

  /* qi & qj are column and row of quadrant
     i & j are column and row within quadrant */

  /* Loop over 4 quadrants: */
  for (qi = 0; qi < 2; qi++)
    for (qj = 0; qj < 2; qj++)
      /* Loop within quadrant: */
      for (i = 0; i < src->dim/2; i++)
        for (j = 0; j < src->dim/2; j++) {
          int s_idx = RIDX((qj * src->dim/2) + i,
                           j + (qi * src->dim/2), src->dim);
          int d_idx = RIDX((qj * src->dim/2) + src->dim/2 - 1 - j,
                           i + (qi * src->dim/2), src->dim);
          dest[d_idx].red = (src[s_idx].red
                             + src[s_idx].green
                             + src[s_idx].blue) / 3;
          dest[d_idx].green = (src[s_idx].red
                               + src[s_idx].green
                               + src[s_idx].blue) / 3;
          dest[d_idx].blue = (src[s_idx].red
                              + src[s_idx].green
                              + src[s_idx].blue) / 3;
        }
}

/* 
 * pinwheel - Your current working version of pinwheel
 * IMPORTANT: This is the version you will be graded on
 */
char pinwheel_descr[] = "pinwheel: Current working version";
void pinwheel(pixel *src, pixel *dest)
{
  v1_pinwheel(src, dest);
}

/*********************************************************************
 * register_pinwheel_functions - Register all of your different versions
 *     of the pinwheel kernel with the driver by calling the
 *     add_pinwheel_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_pinwheel_functions() {
  add_pinwheel_function(&pinwheel, pinwheel_descr);
  add_pinwheel_function(&naive_pinwheel, naive_pinwheel_descr);
}


/***************************************************************
 * MOTION KERNEL
 * 
 * Starts with various typedefs and helper functions for the motion
 * function, and you may modify these any way you like.
 **************************************************************/

/* A struct used to compute averaged pixel value */
typedef struct {
  int red;
  int green;
  int blue;
} pixel_sum;

/* 
 * initialize_pixel_sum - Initializes all fields of sum to 0 
 */
static void initialize_pixel_sum(pixel_sum *sum) 
{
  sum->red = sum->green = sum->blue = 0;
}

/* 
 * accumulate_sum - Accumulates field values of p in corresponding 
 * fields of sum 
 */
static void accumulate_weighted_sum(pixel_sum *sum, pixel p, double weight) 
{
  sum->red += (int) p.red * weight;
  sum->green += (int) p.green * weight;
  sum->blue += (int) p.blue * weight;
}

/* 
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel 
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) 
{
  current_pixel->red = (unsigned short)sum.red;
  current_pixel->green = (unsigned short)sum.green;
  current_pixel->blue = (unsigned short)sum.blue;
}

/* 
 * weighted_combo - Returns new pixel value at (i,j) 
 */
static pixel weighted_combo(int dim, int i, int j, pixel *src) 
{
  int ii, jj;
  pixel_sum sum;
  pixel current_pixel;
  double weights[3][3] = { { 0.60, 0.03, 0.00 },
                           { 0.03, 0.30, 0.03 },
                           { 0.00, 0.03, 0.10 } };

  initialize_pixel_sum(&sum);
  for (ii=0; ii < 3; ii++)
    for (jj=0; jj < 3; jj++) 
      if ((i + ii < dim) && (j + jj < dim))
        accumulate_weighted_sum(&sum,
                                src[RIDX(i+ii,j+jj,dim)],
                                weights[ii][jj]);
  
  assign_sum_to_pixel(&current_pixel, sum);

  return current_pixel;
}

static pixel new_weighted_combo_1(int dim, int i, int j, pixel *src)
{
  pixel new_pixel;
  pixel source_pixel;
  int spr = RIDX(i,j,dim);
  int Tdim = dim << 1;
  double tDf = 5/3;
  double tDt = 10/3;
  double tDh = 100/3;

  new_pixel.red = 0;
  new_pixel.green = 0;
  new_pixel.blue = 0;

  source_pixel = src[spr];
  new_pixel.red += (source_pixel.red / tDf);
  new_pixel.green += (source_pixel.green / tDf);
  new_pixel.blue += (source_pixel.blue / tDf);

    source_pixel = src[spr + Tdim + 2];
    new_pixel.red += (source_pixel.red / 10);
      new_pixel.green += (source_pixel.green / 10);
      new_pixel.blue += (source_pixel.blue / 10);

      source_pixel = src[spr + dim + 1];
      new_pixel.red += (source_pixel.red / tDt);
      new_pixel.green += (source_pixel.green / tDt);
      new_pixel.blue += (source_pixel.blue / tDt);

      source_pixel = src[spr + dim];
      new_pixel.red += (source_pixel.red / tDh);
      new_pixel.green += (source_pixel.green / tDh);
      new_pixel.blue += (source_pixel.blue / tDh);

      source_pixel = src[spr + Tdim + 1];
      new_pixel.red += (source_pixel.red / tDh);
      new_pixel.green += (source_pixel.green / tDh);
      new_pixel.blue += (source_pixel.blue / tDh);

      source_pixel = src[spr + 1];
      new_pixel.red += (source_pixel.red / tDh);
      new_pixel.green += (source_pixel.green / tDh);
      new_pixel.blue += (source_pixel.blue / tDh);

      source_pixel = src[spr + dim + 2];
      new_pixel.red += (source_pixel.red / tDh);
      new_pixel.green += (source_pixel.green / tDh);
      new_pixel.blue += (source_pixel.blue / tDh);

  return new_pixel;
}

static pixel new_weighted_combo_2(int dim, int i, int j, pixel *src)
{
  pixel new_pixel;
  pixel source_pixel;
  int spr = RIDX(i,j,dim);
  int Tdim = dim << 1;

  new_pixel.red = 0;
  new_pixel.green = 0;
  new_pixel.blue = 0;

  source_pixel = src[spr];
  new_pixel.red += (0.60 * source_pixel.red);
  new_pixel.green += (0.60 * source_pixel.green);
  new_pixel.blue += (0.60 * source_pixel.blue);

  if ((i+2 < dim) && (j+2 < dim)) {
    source_pixel = src[spr + Tdim + 2];
    new_pixel.red += (source_pixel.red / 10);
      new_pixel.green += (source_pixel.green / 10);
      new_pixel.blue += (source_pixel.blue / 10);
  }

    if ((i+1 < dim) && (j+1 < dim)) {
      source_pixel = src[spr + dim + 1];
      new_pixel.red += (0.30 * source_pixel.red);
      new_pixel.green += (0.30 * source_pixel.green);
      new_pixel.blue += (0.30 * source_pixel.blue);
    }

    if (i+1 < dim) {
      source_pixel = src[spr + dim];
      new_pixel.red += (0.03 * source_pixel.red);
      new_pixel.green += (0.03 * source_pixel.green);
      new_pixel.blue += (0.03 * source_pixel.blue);
    }

    if ((i+2 < dim) && (j+1 < dim)) {
      source_pixel = src[spr + Tdim + 1];
      new_pixel.red += (0.03 * source_pixel.red);
      new_pixel.green += (0.03 * source_pixel.green);
      new_pixel.blue += (0.03 * source_pixel.blue);
    }

    if (j+1 < dim) {
      source_pixel = src[spr + 1];
      new_pixel.red += (0.03 * source_pixel.red);
      new_pixel.green += (0.03 * source_pixel.green);
      new_pixel.blue += (0.03 * source_pixel.blue);
    }

    if ((i+1 < dim) && (j+2 < dim)) {
      source_pixel = src[spr + dim + 2];
      new_pixel.red += (0.03 * source_pixel.red);
      new_pixel.green += (0.03 * source_pixel.green);
      new_pixel.blue += (0.03 * source_pixel.blue);
    }

  return new_pixel;
}

/******************************************************
 * Your different versions of the motion kernel go here
 ******************************************************/

/*
 * v1_motion - First Try!
 */
char v1_motion_descr[] = "v1_motions: First Try!";
void v1_motion(pixel *src, pixel *dst) 
{
  int i, j;
  int dims = src->dim;
    
  for (i = 0; i < dims; i++)
    for (j = 0; j < dims; j++) {
      /* if ((i+2 < dims) && (j+2 < dims)) */
      /*   dst[RIDX(i, j, dims)] = new_weighted_combo_1(dims, i, j, src); */
      /* else */
	dst[RIDX(i, j, dims)] = new_weighted_combo_2(dims, i, j, src);
    }
}

/*
 * naive_motion - The naive baseline version of motion 
 */
char naive_motion_descr[] = "naive_motion: baseline implementation";
void naive_motion(pixel *src, pixel *dst) 
{
  int i, j;
    
  for (i = 0; i < src->dim; i++)
    for (j = 0; j < src->dim; j++)
      dst[RIDX(i, j, src->dim)] = weighted_combo(src->dim, i, j, src);
}

/*
 * motion - Your current working version of motion. 
 * IMPORTANT: This is the version you will be graded on
 */
char motion_descr[] = "motion: Current working version";
void motion(pixel *src, pixel *dst) 
{
  v1_motion(src, dst);
}

/********************************************************************* 
 * register_motion_functions - Register all of your different versions
 *     of the motion kernel with the driver by calling the
 *     add_motion_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_motion_functions() {
  add_motion_function(&motion, motion_descr);
  add_motion_function(&naive_motion, naive_motion_descr);
}
