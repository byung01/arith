/*
 *      compress40.c
 *      by Theodore Tan and Bill Yung, 10/21/2015
 *      COMP40 HW #4
 */

#include "pnm.h"
#include "assert.h"
#include "a2methods.h"
#include "a2blocked.h"
#include "compress40.h"
#include "pack40.h"

#define DENOMINATOR 255

typedef A2Methods_UArray2 A2;   /* private abbreviation */

/******************************************************************************
 *                             FUNCTION DECLARATIONS                          *
 ******************************************************************************/

/* compress */
static Pnm_ppm read_image(FILE *image, A2Methods_T methods);
static void trim_image(Pnm_ppm image);
static A2 convert_CV(Pnm_ppm image);
static void RGB_to_CV(int col, int row, A2 pixels, void *elem, void *cl);
static UArray_T compressor(A2 array_CV, Pnm_ppm image);
static void get_codewords(int col, int row, A2 pixels, void *elem, void *cl);
static void write_compressed(UArray_T codewords, Pnm_ppm image);

/* decompress */
static struct Pnm_ppm create_image(A2Methods_T methods, FILE *input);
static A2 decompressor(UArray_T codewords, Pnm_ppm image);
static void get_CV(int col, int row, A2 pixels, void *elem, void *cl);
static Pnm_ppm convert_RGB(A2 array_CV, Pnm_ppm image);
static void CV_to_RGB(int col, int row, A2 pixels, void *elem, void *cl);
static void check_range(float *num);
static void write_decompressed(Pnm_ppm image);

/******************************************************************************
 *                       FUNCTION DEFINITIONS - COMPRESS                      *
 ******************************************************************************/

/*
 * compress40 - takes an image and compresses it. prints out the compressed
 *              result in compressed binary image format.
 *
 * input: the original image that the client passes into the program
 */
void compress40(FILE *input)
{
        Pnm_ppm image;
        A2 array_CV;

        A2Methods_T methods = uarray2_methods_blocked;
        assert(methods);

        image = read_image(input, methods);
        trim_image(image);
        array_CV = convert_CV(image);
        UArray_T codewords = compressor(array_CV, image);
        write_compressed(codewords, image);
        
        UArray_free(&codewords);
        methods->free(&array_CV);
        Pnm_ppmfree(&image);
}

/*
 * (Short Function)
 * read_image - reads an image file uses Pnm_ppmread. Raises Pnm_Badformat if
 *              not given a proper PNM file.
 */
static Pnm_ppm read_image(FILE *image, A2Methods_T methods)
{
        return Pnm_ppmread(image, methods);
}

/*
 * (Short Function)
 * trim_image - takes in an image and trim its height and/or width if the
 *              initial value is odd.
 */
static void trim_image(Pnm_ppm image)
{
        if (image->width % 2 != 0)      image->width  -= 1;
        if (image->height % 2 != 0)     image->height -= 1;
}

/*
 * convert_CV - takes an image and converts the RGB values of each individual
 *              pixel to its corresponding Y, Pb, and Pr value. Returns the
 *              converted values in an A2.
 *
 * image: the image to be compressed
 */
static A2 convert_CV(Pnm_ppm image)
{
        A2 array_CV;

        const struct A2Methods_T *methods = image->methods;

        array_CV = methods->new_with_blocksize(image->width, image->height,
                                               sizeof(struct CV),
                                               BS);

        methods->map_block_major(array_CV, RGB_to_CV, image);

        return array_CV;
}

/*
 * RGB_to_CV - does the math for the actual RGB to CV value conversion
 *
 * col: the current column being evaluated
 * row: the current row being evaluated
 * pixels: the array of pixels being mapped through
 * elem: pointer to the current element being evaluated
 * cl: closure; in this case a Pnm_ppm image.
 */
static void RGB_to_CV(int col, int row, A2 pixels, void *elem, void *cl)
{
        (void)pixels;
        CV value = malloc(sizeof(*value));

        const struct A2Methods_T *methods = ((Pnm_ppm)cl)->methods;
        Pnm_ppm image = (Pnm_ppm)cl;

        Pnm_rgb pixel = (Pnm_rgb)(methods->at(image->pixels, col, row));

        float r = (float)(pixel->red) / (float)(image->denominator);
        float g = (float)(pixel->green) / (float)(image->denominator);
        float b = (float)(pixel->blue) / (float)(image->denominator);
        
        float  Y = (0.299 * r) + (0.587 * g) + (0.114 * b);
        float Pb = (-0.168736 * r) - (0.331264 * g) + (0.5 * b);
        float Pr = (0.5 * r) - (0.418688 * g) - (0.081312 * b);

        value->Y = Y;
        value->Pb = Pb;
        value->Pr = Pr;
        
        *(CV)elem = (*value);
        free(value);
}

/*
 * compressor - takes in a 2D array of CV structs, packs them, and
 *              returns a 1D array of codewords
 *
 * array_CV: the CV structs to convert into codewords 
 * image: the image that is beign compressed
 */
static UArray_T compressor(A2 array_CV, Pnm_ppm image)
{
        int len_bl = BS * BS;
        int len_cw = (image->width * image->height) / (len_bl);

        struct Codewords cl = {.counter = 0,
                               .block = UArray_new(len_bl, sizeof(struct CV)),
                               .codewords = UArray_new(len_cw, sizeof(uint64_t))
                              };

        image->methods->map_block_major(array_CV, get_codewords, &cl);
        
        UArray_free(&(cl.block));
        return cl.codewords;
}

/*
 * (Apply Function)
 * get_codewords - pack blocks into codewords and saves them in the closure
 */
static void get_codewords(int col, int row, A2 pixels, void *elem, void *cl)
{
        (void)pixels;

        Codewords cw = (Codewords)cl;

        int blk_idx = BS * (col % BS) + (row % BS);

        *((CV)UArray_at(cw->block, blk_idx)) = *(CV)elem;
        (cw->counter)++;

        if ((cw->counter) % (BS * BS) == 0) {
                uint64_t codeword = pack40(cw->block);
                int cw_idx = (((cw->counter) / (BS * BS)) - 1);
                *(uint64_t *)UArray_at(cw->codewords, cw_idx) = codeword;
        }
}

/*
 * (Short Function)
 * write_compressed - takes in codewords and write to stdout as a compressed
 *                    binary image
 */
static void write_compressed(UArray_T codewords, Pnm_ppm image)
{
        print_packed(codewords, image->width, image->height);
}

/******************************************************************************
 *                      FUNCTION DEFINITIONS - DECOMPRESS                     *
 ******************************************************************************/


void decompress40(FILE *input)
{
        struct Pnm_ppm image;
        A2 array_CV;

        A2Methods_T methods = uarray2_methods_blocked;
        assert(methods);

        image = create_image(methods, input);
        UArray_T codewords = read_packed(input, image.width, image.height);
        
        array_CV = decompressor(codewords, &image);
        image = *(convert_RGB(array_CV, &image));
        write_decompressed(&image);

        methods->free(&array_CV);
        methods->free(&(image.pixels));
}

/*
 * create_image - declares a new Pnm_ppm image based on the parameters read
 *                in from file. 
 *
 * methods: the methods that you wish to set in the Pnm_ppm
 */
static struct Pnm_ppm create_image(A2Methods_T methods, FILE *input)
{
        unsigned width, height;
        int read = fscanf(input, "COMP40 Compressed image format 2\n%u %u",
                          &width, &height);
        assert(read == 2);
        int c = getc(input);
        assert(c == '\n');

        struct Pnm_ppm image = {.width = width, .height = height,
                                .denominator = DENOMINATOR, .pixels = NULL,
                                .methods = methods
                               };

        return image;
}

/*
 * decompressor - takes in a 1D array of codewords, unpacks them into local 
 *                variables and then returns a 2D array of CV structs
 *
 * codewords: the codewords to decompress and turn into an image
 * image: the image that all the image data will be stored in to print
 */
static A2 decompressor(UArray_T codewords, Pnm_ppm image)
{
        const struct A2Methods_T *methods = image->methods;

        A2 array_CV = methods->new_with_blocksize(image->width, image->height,
                                                  sizeof(struct CV),
                                                  BS);

        struct Codewords cl = {.counter = 0,
                               .block = NULL,
                               .codewords = codewords};

        methods->map_block_major(array_CV, get_CV, &cl);

        UArray_free(&(cl.block));
        UArray_free(&(cl.codewords));

        return array_CV;
}

/*
 * (Apply Function)
 * get_CV - transform codewords into CV structs and save them
 */
static void get_CV(int col, int row, A2 pixels, void *elem, void *cl)
{
        (void)pixels;

        Codewords cw = (Codewords)cl;

        if ((cw->counter) % (BS * BS) == 0) {
                if (cw->counter != 0) {
                        UArray_free(&(cw->block));
                }
                int cw_idx = (cw->counter) / 4;
                cw->block = unpack40(*(uint64_t *)UArray_at(cw->codewords, cw_idx));
        }

        int blk_idx = BS * (col % BS) + (row % BS);
        *(CV)elem = *(CV)(UArray_at(cw->block, blk_idx));

        (cw->counter)++;
}

/*
 * convert_RGB - takes an image and converts the CV values of each individual 
 *               pixel to its corresponding RGB values. Returns the converted 
 *               values in an A2.
 *
 * image: the image to be decompressed
 */
static Pnm_ppm convert_RGB(A2 array_CV, Pnm_ppm image)
{
        const struct A2Methods_T *methods = image->methods;

        image->pixels = methods->new_with_blocksize(image->width, image->height,
                                                    sizeof(struct Pnm_rgb), BS);

        methods->map_block_major(array_CV, CV_to_RGB, image);

        return image;
}

/*
 * CV_to_RGB - does the math for the actual CV to RGB value conversion
 *
 * col: the current column being evaluated
 * row: the current row being evaluated
 * pixels: the array of pixels being mapped through
 * elem: pointer to the current element being evaluated
 * cl: closure; in this case a Pnm_ppm image.
 */
static void CV_to_RGB(int col, int row, A2 pixels, void *elem, void *cl)
{
        (void)pixels;
        Pnm_rgb value = malloc(sizeof(struct Pnm_rgb));
        const struct A2Methods_T *methods = ((Pnm_ppm)cl)->methods;
        Pnm_ppm image = (Pnm_ppm)cl;
        CV pixel = (CV)elem;

        float Y  = pixel->Y;
        float Pb = pixel->Pb;
        float Pr = pixel->Pr;

        float r = (1.0 * Y) + (0.0 * Pb) + (1.402 * Pr);
        float g = (1.0 * Y) - (0.344136 * Pb) - (0.714136 * Pr);
        float b = (1.0 * Y) + (1.772 * Pb) + (0.0 * Pr);
        
        // Must make sure that rgb values are between 0 and 1
        check_range(&r);
        check_range(&g);
        check_range(&b);

        value->red   = (unsigned)(r * (image->denominator));
        value->green = (unsigned)(g * (image->denominator));
        value->blue  = (unsigned)(b * (image->denominator));

        (*(Pnm_rgb)methods->at(image->pixels, col, row)) = (*value);
        free(value);
}

/*
 * (Short Function)
 * check_range - takes in a float and ensures that its value is between 0 and 1
 */
static void check_range(float *num)
{
        if (*num < 0.0) {
                *num = 0.0;
        }
        if (*num > 1.0) {
                *num = 1.0;
        }
}

/*
 * (Short Function)
 * write_decompressed - takes in an image and calls Pnm_ppmwrite to print 
 *                      the image to stdout
 */
static void write_decompressed(Pnm_ppm image)
{
        Pnm_ppmwrite(stdout, image);
}
