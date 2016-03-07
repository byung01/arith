/*
 *      pack40.c
 *      by Theodore Tan and Bill Yung, 10/21/2015
 *      COMP40 HW #4
 *
 *      Implementation for the conversion between CV structs and
 *      32-bit codewords
 */

#include "pack40.h"
#include "bitpack.h"

#include "assert.h"

/******************************************************************************
 *                                   STRUCTS                                  *
 ******************************************************************************/
typedef struct bitWord {
        unsigned a;
        signed   b, c, d;
        unsigned Pba, Pra;
} *bitWord;

typedef struct Average {
        float Pba;
        float Pra;
} Average;

/******************************************************************************
 *                                  MACROS                                    *
 ******************************************************************************/

#define SIZE_A  9
#define SIZE_B  5
#define SIZE_C  5
#define SIZE_D  5
#define SIZE_Pb 4
#define SIZE_Pr 4

#define LSB_A  23
#define LSB_B  18
#define LSB_C  13
#define LSB_D  8
#define LSB_Pb 4
#define LSB_Pr 0

#define COEF_A 511
#define COEF_B 50
#define COEF_C 50
#define COEF_D 50

/******************************************************************************
 *                        HELPER FUNCTION DECLARATIONS                        *
 ******************************************************************************/

/* Helper functions for pack40 */
static bitWord initialize_bitWord(UArray_T block);
static bitWord set_chroma_index(UArray_T block, bitWord word);
static Average find_average(UArray_T block, Average average);
static void check_range(float *num);
static bitWord cosine_transform(UArray_T block, bitWord word);
static uint64_t pack_word(bitWord word);

/* Helper functions for unpack40 */
static bitWord unpack_word(uint64_t codeword);
static UArray_T initialize_colorspace(bitWord word);
static UArray_T reverse_cosine_transform(UArray_T block, bitWord word);
static UArray_T get_chroma(UArray_T block, bitWord word);

/******************************************************************************
 *                             PACK40 IMPLEMENTATION                          *
 ******************************************************************************/

/*
 * pack40 - takes in a 2 x 2 block of CV structs and packs it into a codeword
 */
extern uint64_t pack40(UArray_T block)
{
        bitWord word = initialize_bitWord(block);
        uint64_t codeword = pack_word(word);
        free(word);

        return codeword;
}

/*
 * initialize bitWord - takes in a 2 x 2 block of CV structs and converts it
 *                      into a bitWord struct
 */
static bitWord initialize_bitWord(UArray_T block)
{
        bitWord word = malloc(sizeof(*word));
        word = set_chroma_index(block, word);
        word = cosine_transform(block, word);
        return word;
}

static bitWord set_chroma_index(UArray_T block, bitWord word)
{
        Average average = {.Pba = 0.0, .Pra = 0.0};

        average = find_average(block, average);

        unsigned Pba_chroma = Arith40_index_of_chroma(average.Pba);
        unsigned Pra_chroma = Arith40_index_of_chroma(average.Pra);
        
        word->Pba = Pba_chroma;
        word->Pra = Pra_chroma;

        return word;
}

static Average find_average(UArray_T block, Average average)
{
        for (int i = 0; i < UArray_length(block); i++) {
                CV temp = (CV)UArray_at(block, i);
                (average.Pba) += temp->Pb;
                (average.Pra) += temp->Pr;
        }

        (average.Pba) /= (float)UArray_length(block);
        (average.Pra) /= (float)UArray_length(block);

        return average;
}

static bitWord cosine_transform(UArray_T block, bitWord word)
{
        float Y1 = ((CV)UArray_at(block, 0))->Y;
        float Y2 = ((CV)UArray_at(block, 1))->Y;
        float Y3 = ((CV)UArray_at(block, 2))->Y;
        float Y4 = ((CV)UArray_at(block, 3))->Y;

        float a = ((Y4 + Y3 + Y2 + Y1) / 4.0);
        float b = ((Y4 + Y3 - Y2 - Y1) / 4.0);
        float c = ((Y4 - Y3 + Y2 - Y1) / 4.0);
        float d = ((Y4 - Y3 - Y2 + Y1) / 4.0);

        /* ensure that b, c, and d are between -0.3 and 0.3 */
        check_range(&b);
        check_range(&c);
        check_range(&d);
        
        word->a = (unsigned)(roundf(a * COEF_A));
        word->b = (signed)(roundf(b * COEF_B));
        word->c = (signed)(roundf(c * COEF_C));
        word->d = (signed)(roundf(d * COEF_D));
        
        return word;
}

/*
 * check_range - ensures that b, c, and d are between -0.3 and 0.3
 */
static void check_range(float *num)
{
        if (*num < -0.3) {
                *num = -0.3;
        }
        if (*num > 0.3) {
                *num = 0.3;
        }
}

/*
 * pack_word - uses the bitpack interface to pack the bitWord struct into
 *             a codeword
 */
static uint64_t pack_word(bitWord word)
{
        uint64_t codeword = 0;

        codeword = Bitpack_newu(codeword, SIZE_A, LSB_A, word->a);
        codeword = Bitpack_news(codeword, SIZE_B, LSB_B, word->b);
        codeword = Bitpack_news(codeword, SIZE_C, LSB_C, word->c);
        codeword = Bitpack_news(codeword, SIZE_D, LSB_D, word->d);
        codeword = Bitpack_newu(codeword, SIZE_Pb, LSB_Pb, word->Pba);
        codeword = Bitpack_newu(codeword, SIZE_Pr, LSB_Pr, word->Pra);

        return codeword;
}


/******************************************************************************
 *                           UNPACK40 IMPLEMENTATION                          *
 ******************************************************************************/

/*
 * unpack40 - takes in a codeword and returns a 2 x 2 block of CV structs
 */
extern UArray_T unpack40(uint64_t codeword)
{
        bitWord word = unpack_word(codeword);
        UArray_T block = initialize_colorspace(word);
        free(word);
        return block;
}

/*
 * unpack_word - uses the bitpack interface to unpack the codeword into 
 *               a bitWord struct
 */
static bitWord unpack_word(uint64_t codeword)
{
        bitWord word = malloc(sizeof(*word));

        word->Pra = Bitpack_getu(codeword, SIZE_Pr, LSB_Pr);
        word->Pba = Bitpack_getu(codeword, SIZE_Pb, LSB_Pb);
        word->d = Bitpack_gets(codeword, SIZE_D, LSB_D);
        word->c = Bitpack_gets(codeword, SIZE_C, LSB_C);
        word->b = Bitpack_gets(codeword, SIZE_B, LSB_B);
        word->a = Bitpack_getu(codeword, SIZE_A, LSB_A);
        
        return word;
}

/*
 * initialize colorspace - takes a bitWord struct and converts in into a 1D
 *                         array of Cv structs
 */
static UArray_T initialize_colorspace(bitWord word)
{
        UArray_T block = UArray_new(BS * BS, sizeof(struct CV));
        block = get_chroma(block, word);
        block = reverse_cosine_transform(block, word);

        return block;
}

static UArray_T get_chroma(UArray_T block, bitWord word)
{
        float Pb = Arith40_chroma_of_index(word->Pba);
        float Pr = Arith40_chroma_of_index(word->Pra);

        for (int i = 0; i < UArray_length(block); i++) {
                CV temp = (CV)UArray_at(block, i);
                temp->Pb = Pb;
                temp->Pr = Pr;
        }

        return block;
}

static UArray_T reverse_cosine_transform(UArray_T block, bitWord word)
{
        float a = ((float)(word->a) / COEF_A);
        float b = ((float)(word->b) / COEF_B);
        float c = ((float)(word->c) / COEF_C);
        float d = ((float)(word->d) / COEF_D);

        float Y1 = a - b - c + d;
        float Y2 = a - b + c - d;
        float Y3 = a + b - c - d;
        float Y4 = a + b + c + d;

        ((CV)UArray_at(block, 0))->Y = Y1;
        ((CV)UArray_at(block, 1))->Y = Y2;
        ((CV)UArray_at(block, 2))->Y = Y3;
        ((CV)UArray_at(block, 3))->Y = Y4;

        return block;
}

/******************************************************************************
 *                                PRINTED PACKED                              *
 ******************************************************************************/

extern void print_packed(UArray_T codewords, unsigned width, unsigned height)
{
        printf("COMP40 Compressed image format 2\n%u %u", width, height);

        printf("\n");
        
        for (int i = 0; i < UArray_length(codewords); i++) {
                uint64_t codeword = *(uint64_t *)UArray_at(codewords, i);
                for (int j = 3; j >= 0; j--) {
                        char byte = (char)Bitpack_getu(codeword, 8, (j * 8));
                        putchar(byte);
                }
        }               
}

extern UArray_T read_packed(FILE *input, unsigned width, unsigned height)
{
        int len_cw = (width * height) / (BS * BS);
        UArray_T codewords = UArray_new(len_cw, sizeof(uint64_t));
        
        for (int i = 0; i < UArray_length(codewords); i++) {
                uint64_t codeword = 0;
                for (int j = 3; j >= 0; j--) {
                        uint64_t byte = getc(input);
                        codeword = Bitpack_newu(codeword, 8, (j * 8), byte);
                }
                *(uint64_t *)UArray_at(codewords, i) = codeword;
        }

        return codewords;
}