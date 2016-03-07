/*
 *      bitpack.c
 *      by Theodore Tan and Bill Yung, 10/21/2015
 *      COMP40 HW #4 
 *
 *      This is the implementation of bitpack.h, acoording to the given 
 *      interface and specifications. All the functions work as expected.  
 */

#include "bitpack.h"
#include "assert.h"
#include <stdio.h>
#include "inttypes.h"

 /******************************************************************************
 *                                 EXCEPTIONS
 ******************************************************************************/
Except_T Bitpack_Overflow = { "Overflow packing bits" };

/******************************************************************************
 *                              FUNCTION DECLARATION                          *
 ******************************************************************************/

static uint64_t Bitpack_replace(uint64_t word, unsigned width, unsigned lsb,  
                                int64_t value);

/******************************************************************************
 *                         STATIC FUNCTION DEFINITIONS                        *
 ******************************************************************************/

static inline uint64_t leftshift(uint64_t word, unsigned offset)
{
        assert(offset <= 64);

        if (offset <= 63) {
                word = (word << offset);
        }
        else {
                word = 0;
        }

        return word;
}

static inline int64_t arithright(int64_t word, unsigned offset)
{
        assert(offset <= 64);

        if (offset <= 63) {
                word = (word >> offset);
        }
        else if (word >= 0) {
                word = 0;
        }
        else {
                word = ~0;       
        }

        return word;
}

static inline uint64_t logicalright(uint64_t word, unsigned offset)
{
        assert(offset <= 64);

        if (offset <= 63) {
                word = (word >> offset);
        }
        else {
                word = 0;
        }

        return word;
}


/******************************************************************************
 *                              FUNCTION DEFINITIONS                          *
 ******************************************************************************/

bool Bitpack_fitsu(uint64_t n, unsigned width)
{
        assert(width <= 64);

        uint64_t max = ~0;
        max = leftshift(max, width);
        max = ~max;

        if (n <= max) return true;
        else return false;
}


bool Bitpack_fitss(int64_t n, unsigned width)
{
        assert(width <= 64);
        
        int64_t max;
        int64_t min = ~0;

        min = (int64_t)(leftshift(min, width - 1));
        max = ~min;

        if (min <= n && n <= max) return true;
        else                      return false;
}


uint64_t Bitpack_getu(uint64_t word, unsigned width, unsigned lsb)
{
        assert(width <= 64);
        assert(width + lsb <= 64);

        uint64_t mask = ~0;
        mask = leftshift(mask, (64 - width));
        mask = logicalright(mask, (64 - width - lsb)); 

        word = (word & mask);
        word = logicalright(word, lsb);

        return word;
}

int64_t Bitpack_gets(uint64_t word, unsigned width, unsigned lsb)
{
        assert(width <= 64);
        assert(width + lsb <= 64);

        uint64_t mask = ~0;
        mask = leftshift(mask, (64 - width));
        mask = logicalright(mask, (64 - width - lsb)); 

        word = (word & mask);
        word = leftshift(word, (64 - width - lsb));
        word = arithright(word, (64 - width));

        return (int64_t)word;
}


uint64_t Bitpack_newu(uint64_t word, unsigned width, unsigned lsb, 
                      uint64_t value)
{
        assert(width <= 64);
        assert(width + lsb <= 64);
        
        if (Bitpack_fitsu(value, width)) {
                word = Bitpack_replace(word, width, lsb, value);
        }
        else {
                RAISE(Bitpack_Overflow);
        }

        return word;
}

uint64_t Bitpack_news(uint64_t word, unsigned width, unsigned lsb,  
                      int64_t value)
{
        assert(width <= 64);
        assert(width + lsb <= 64);
        
        if (Bitpack_fitss(value, width)) {
                word = Bitpack_replace(word, width, lsb, value);

        }
        else {
                RAISE(Bitpack_Overflow);
        }

        return word;

}

static uint64_t Bitpack_replace(uint64_t word, unsigned width, unsigned lsb,  
                                int64_t value)
{
        int64_t mask = ~0;
        mask = leftshift(mask, (64 - width));
        mask = logicalright(mask, (64 - width - lsb));

        value = leftshift(value, lsb);
        value = (value & mask);

        word = (word & ~mask);
        word = (word | value);

        return word;
}
