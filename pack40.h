/*
 *      pack40.h
 *      by Theodore Tan and Bill Yung, 10/21/2015
 *      COMP40 HW #4
 *      
 *      Interface for the conversion between CV_colorspace structs and 
 *      32-bit codewords
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "uarray.h"
#include "arith40.h"
#include "math.h"

#define BS 2

/******************************************************************************
 *                                   STRUCTS                                  * 
 ******************************************************************************/
typedef struct Codewords {
        int counter;
        UArray_T block; /* contains the CV_colorspace structs of a block */
        UArray_T codewords; /* contains a list of 32-bit codewords */
} *Codewords;

typedef struct CV {
        float Y;
        float Pb;
        float Pr;
} *CV;

/******************************************************************************
 *                                 FUNCTIONS                                  * 
 ******************************************************************************/

/* 
 * Reads in an 2x2 block of CV_colorspace structs
 * Converts it to a 32-bit codeword 
 */
extern uint64_t pack40   (UArray_T block);

/*
 * Reads in a 32-bit codeword
 * Converts it to a 2x2 block of CV_colorspace structs
 */
extern UArray_T unpack40 (uint64_t codeword);

extern void print_packed(UArray_T codewords, unsigned width, unsigned height);

extern UArray_T read_packed(FILE *input, unsigned width, unsigned height);