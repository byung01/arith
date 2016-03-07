/*
 *      ppmdiff.c
 *      by Theodore Tan and Bill Yung
 *      COMP40 HW #3
 *      
 *      This is a program that takes in a two ppm images and compares the diff.    
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "assert.h"
#include "a2methods.h"
#include "a2plain.h"
#include "pnm.h"

const float MAX_INTENSITY = 255.0;

static FILE *open_file (int argc, char *argv);
static bool valid_dims(Pnm_ppm image1, Pnm_ppm image2);
static void compute_diff(Pnm_ppm image1, Pnm_ppm image2);
static int compare(int x, int y);
static float rgb_diff(Pnm_rgb pixel1, Pnm_rgb pixel2);

int main(int argc, char *argv[]) {
        
        /* default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain; 
        assert(methods);

        /* default to best map */
        A2Methods_mapfun *map = methods->map_default; 
        assert(map);

        assert (argc <= 3);

        FILE *fp1, *fp2;
        Pnm_ppm image1, image2;

        fp1 = open_file(argc, argv[1]);
        fp2 = open_file(argc, argv[2]);

        image1 = Pnm_ppmread(fp1, methods);
        image2 = Pnm_ppmread(fp2, methods);

        if (!valid_dims(image1, image2)) {
                fprintf(stderr, "Invalid Dimensions\n");
                fprintf(stdout, "1.0\n");
        } else {
                compute_diff(image1, image2);
        }

        fclose(fp1);
        fclose(fp2);

        Pnm_ppmfree(&image1);
        Pnm_ppmfree(&image2);

        return 0;
}

/*
 * (Short Function)
 * open_file - opens a ppm file
 *
 * Checked Runtime Errors:
 *      1. File pointer is NULL. Program terminates.
 */
static FILE *open_file (int argc, char *argv)
{
        FILE *fp = stdin;

        (void)argc;

        if (argv[0] != '-') {
                assert((fp = fopen(argv, "rb")) != NULL);
        }
        
        return fp;
}

static bool valid_dims(Pnm_ppm image1, Pnm_ppm image2)
{
        int heightdiff = (image1->height - image2->height);
        int widthdiff = (image1->width - image2->width);

        if (!(heightdiff == 0 || heightdiff == 1 || heightdiff == -1)) {
                return false;
        }

        if (!(widthdiff == 0 || widthdiff == 1 || widthdiff == -1)) {
                return false;
        }

        return true;
}

static void compute_diff(Pnm_ppm image1, Pnm_ppm image2)
{
        int sm_width  = compare(image1->width, image2->width);
        int sm_height = compare(image1->height, image2->height);

        const struct A2Methods_T *methods1 = image1 -> methods;
        const struct A2Methods_T *methods2 = image2 -> methods;

        float sum = 0;
        float E;
        
        for (int row = 0; row < sm_height; row++) {
                for (int col = 0; col < sm_width; col++) {
                        Pnm_rgb pixel1 = methods1->at(image1->pixels, col, row);
                        Pnm_rgb pixel2 = methods2->at(image2->pixels, col, row);
                        sum += rgb_diff(pixel1, pixel2);
                        //printf("Sum %f\n\n", sum);
                }
        }

        sum /= (3 * sm_width * sm_height);

        E = sqrtf(sum);

        fprintf(stdout, "%1.4f\n", E);
}

static int compare(int x, int y)
{
        if (x > y) {
                return y;
        }
        else return x;
}

static float rgb_diff(Pnm_rgb pixel1, Pnm_rgb pixel2)
{
        float r, g, b;

        r = ((int)(pixel1->red) - (int)(pixel2->red)) / MAX_INTENSITY;
        g = ((int)(pixel1->green) - (int)(pixel2->green)) / MAX_INTENSITY;
        b = ((int)(pixel1->blue) - (int)(pixel2->blue)) / MAX_INTENSITY;

        r *= r;
        g *= g;
        b *= b;

        return r + g + b;
}