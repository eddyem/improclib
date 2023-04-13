/*
 * This file is part of the improclib project.
 * Copyright 2023 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memcpy
#include <math.h>
#include <sys/time.h>

#include "openmp.h"
#include "improclib.h"

//#define TESTMSGS
#ifdef TESTMSGS
#define TEST(...) printf(__VA_ARGS__)
#else
#define TEST(...)
#endif

// global arrays for erosion/dilation masks
static uint8_t *ER = NULL, *DIL = NULL;

/*
 * =================== AUXILIARY FUNCTIONS ===================>
 */

/**
 * This function inits masks arrays for erosion and dilation
 * You may call it yourself or it will be called when one of
 * `erosion` or `dilation` functions will be ran first time
 */
static void morph_init(){
    if(ER) return;
    ER = MALLOC(uint8_t, 256);
    DIL = MALLOC(uint8_t, 256);
    OMP_FOR()
    for(int i = 0; i < 256; i++){
        ER[i]  = i & ((i << 1) | 1) & ((i >> 1) | (0x80)); // don't forget that << and >> set borders to zero
        DIL[i] = i | (i << 1) | (i >> 1);
    }
}

/*
 * =================== MORPHOLOGICAL OPERATIONS ===================>
 */

/**
 * Remove all non-4-connected pixels
 * @param image (i) - input image
 * @param W, H      - size of binarized image (in pixels)
 * @return allocated memory area with converted input image
 */
uint8_t *ilfilter4(uint8_t *image, int W, int H){
    //FNAME();
    if(W < MINWIDTH || H < MINHEIGHT) return NULL;
    uint8_t *ret = MALLOC(uint8_t, W*H);
    int W0 = (W + 7) / 8; // width in bytes
    int w = W0-1, h = H-1;
    {
    // top of image, y = 0
    #define IM_UP
    #include "fc_filter.inc"
    #undef IM_UP
    }
    {
    // mid of image, y = 1..h-1
    #include "fc_filter.inc"
    }
    {
    // image bottom, y = h
    #define IM_DOWN
    #include "fc_filter.inc"
    #undef IM_DOWN
    }
    return ret;
}

/**
 * Remove all non-8-connected pixels (single points)
 * @param image (i) - input image
 * @param W, H      - size of binarized image (in pixels)
 * @return allocated memory area with converted input image
 */
uint8_t *ilfilter8(uint8_t *image, int W, int H){
    //FNAME();
    if(W < MINWIDTH || H < MINHEIGHT) return NULL;
    uint8_t *ret = MALLOC(uint8_t, W*H);
    int W0 = (W + 7) / 8; // width in bytes
    int w = W0-1, h = H-1;
    {
    #define IM_UP
    #include "ec_filter.inc"
    #undef IM_UP
    }
    {
    #include "ec_filter.inc"
    }
    {
    #define IM_DOWN
    #include "ec_filter.inc"
    #undef IM_DOWN
    }
    return ret;
}

/**
 * Make morphological operation of dilation
 * @param image (i) - input image
 * @param W, H      - size of image (pixels)
 * @return allocated memory area with dilation of input image
 */
uint8_t *ildilation(const uint8_t *image, int W, int H){
    //FNAME();
    if(W < MINWIDTH || H < MINHEIGHT) return NULL;
    int W0 = (W + 7) / 8; // width in bytes
    int w = W0-1, h = H-1, rest = 7 - (W - w*8);
    uint8_t lastmask = ~(1<<rest);
    if(!DIL) morph_init();
    uint8_t *ret = MALLOC(uint8_t, W0*H);
    {
    // top of image, y = 0
    #define IM_UP
    #include "dilation.inc"
    #undef IM_UP
    }
    {
    // mid of image, y = 1..h-1
    #include "dilation.inc"
    }
    {
    // image bottom, y = h
    #define IM_DOWN
    #include "dilation.inc"
    #undef IM_DOWN
    }
    return ret;
}

static void mkerosion(const uint8_t *in, uint8_t *out, int W, int H){
    if(!ER) morph_init();
    int W0 = (W + 7) / 8; // width in bytes
    int w = W0-1, h = H-1, rest = 8 - (W - w*8);
    uint8_t lastmask = ~(1<<rest);
    // clear first and last rows:
    bzero(out, W0);
    bzero(out + W0*h, W0);
    //DBG("rest=%d, mask:0x%x", rest, lastmask);
    OMP_FOR()
    for(int y = 1; y < h; y++){ // reset first & last rows of image
        const uint8_t *iptr = &in[W0*y];
        uint8_t *optr = &out[W0*y];
        register uint8_t p;
        // x == 0, reset left bit
        p = ER[*iptr] & iptr[-W0] & iptr[W0];
        if(!(*(++iptr) & 0x80)) p &= 0xfe;
        *optr++ = p & 0x7f;
        for(int x = 1; x < w; x++, iptr++, optr++){
            p = iptr[-W0];
            p = ER[*iptr] & iptr[-W0] & iptr[W0];
            if(!(iptr[-1] & 1)) p &= 0x7f;
            if(!(iptr[1] & 0x80)) p &= 0xfe;
            *optr = p;
        }
        // x == w, reset right bit
        p = ER[*iptr] & iptr[-W0] & iptr[W0];
        if(!(iptr[-1] & 1)) p &= 0x7f;
        *optr = p & lastmask; // reset right bit
    }
}

/**
 * Make morphological operation of erosion by cross 3x3 pixels
 * @param image (i) - input image
 * @param W, H      - size of image (in pixels)
 * @return allocated memory area with erosion of input image
 */
uint8_t *ilerosion(const uint8_t *image, int W, int H){
    if(W < MINWIDTH || H < MINHEIGHT) return NULL;
    int W0 = (W + 7) / 8; // width in bytes
    uint8_t *ret = MALLOC(uint8_t, W0*H);
    mkerosion(image, ret, W, H);
    return ret;
}
// Make erosion N times
uint8_t *ilerosionN(const uint8_t *image, int W, int H, int N){
    if(W < MINWIDTH || H < MINHEIGHT || N < 1) return NULL;
    int W0 = (W + 7) / 8, sz = W0*H;
    uint8_t *in = MALLOC(uint8_t, sz), *out = MALLOC(uint8_t, sz);
    memcpy(out, image, sz);
    for(int i = 0; i < N; ++i){
        register uint8_t *tmp = in;
        in = out; out = tmp;
        mkerosion(in, out, W, H);
    }
    FREE(in);
    return out;
}
// Make dilation N times
uint8_t *ildilationN(const uint8_t *image, int W, int H, int N){
    if(W < MINWIDTH || H < MINHEIGHT || N < 1) return NULL;
    uint8_t *cur = (uint8_t*)image, *next = NULL;
    for(int i = 0; i < N; ++i){
        next = ildilation(cur, W, H);
        if(cur != image) FREE(cur);
        cur = next;
    }
    return next;
}

// Ntimes opening
uint8_t *ilopeningN(uint8_t *image, int W, int H, int N){
    //FNAME();
    if(W < MINWIDTH || H < MINHEIGHT || N < 1) return NULL;
    uint8_t *er = ilerosionN(image, W, H, N);
    uint8_t *op = ildilationN(er, W, H, N);
    FREE(er);
    return op;
}

// Ntimes closing
uint8_t *ilclosingN(uint8_t *image, int W, int H, int N){
    //FNAME();
    if(W < MINWIDTH || H < MINHEIGHT || N < 1) return NULL;
    uint8_t *di = ildilationN(image, W, H, N);
    uint8_t *cl = ilerosionN(di, W, H, N);
    FREE(di);
    return cl;
}

// top hat operation: image - opening(image)
uint8_t *iltopHat(uint8_t *image, int W, int H, int N){
    //FNAME();
    if(W < MINWIDTH || H < MINHEIGHT || N < 1) return NULL;
    uint8_t *op = ilopeningN(image, W, H, N);
    int W0 = (W + 7) / 8; // width in bytes
    int wh = W0 * H;
    OMP_FOR()
    for(int i = 0; i < wh; ++i)
        op[i] = image[i] & (~op[i]);
    return op;
}

// bottom hat operation: closing(image) - image
uint8_t *ilbotHat(uint8_t *image, int W, int H, int N){
    //FNAME();
    if(W < MINWIDTH || H < MINHEIGHT || N < 1) return NULL;
    uint8_t *op = ilclosingN(image, W, H, N);
    int W0 = (W + 7) / 8; // width in bytes
    int wh = W0 * H;
    OMP_FOR()
    for(int i = 0; i < wh; ++i)
        op[i] &= ~image[i];
    return op;
}

/*
 * <=================== MORPHOLOGICAL OPERATIONS ===================
 */

/*
 * =================== LOGICAL OPERATIONS ===================>
 */
/**
 * Logical AND of two images
 * @param im1, im2 (i) - two images
 * @param W, H         - their size (of course, equal for both images)
 * @return allocated memory area with   image = (im1 AND im2)
 */
uint8_t *ilimand(uint8_t *im1, uint8_t *im2, int W, int H){
    uint8_t *ret = MALLOC(uint8_t, W*H);
    int y;
    OMP_FOR()
    for(y = 0; y < H; y++){
        int x, S = y*W;
        uint8_t *rptr = &ret[S], *p1 = &im1[S], *p2 = &im2[S];
        for(x = 0; x < W; x++)
            *rptr++ = *p1++ & *p2++;
    }
    return ret;
}

/**
 * Substitute image 2 from image 1: reset to zero all bits of image 1 which set to 1 on image 2
 * @param im1, im2 (i) - two images
 * @param W, H         - their size (of course, equal for both images)
 * @return allocated memory area with    image = (im1 AND (!im2))
 */
uint8_t *ilsubstim(uint8_t *im1, uint8_t *im2, int W, int H){
    uint8_t *ret = MALLOC(uint8_t, W*H);
    int y;
    OMP_FOR()
    for(y = 0; y < H; y++){
        int x, S = y*W;
        uint8_t *rptr = &ret[S], *p1 = &im1[S], *p2 = &im2[S];
        for(x = 0; x < W; x++)
            *rptr++ = *p1++ & (~*p2++);
    }
    return ret;
}
/*
 * <=================== LOGICAL OPERATIONS ===================
 */

/*
 * =================== CONNECTED COMPONENTS LABELING ===================>
 */

// check table and rename all "oldval" into "newval"
static inline void remark(size_t newval, size_t oldval, size_t *assoc){
    TEST("\tnew = %zd, old=%zd; ", newval, oldval);
    // find the least values
    do{newval = assoc[newval];}while(assoc[newval] != newval);
    do{oldval = assoc[oldval];}while(assoc[oldval] != oldval);
    TEST("\trealnew = %zd, realold=%zd ", newval, oldval);
    // now change larger value to smaller
    if(newval > oldval){
        assoc[newval] = oldval;
        TEST("change %zd to %zd\n", newval, oldval);
    }else{
        assoc[oldval] = newval;
        TEST("change %zd to %zd\n", oldval, newval);
    }
}

/**
 * label 4-connected components on image
 * (slow algorythm, but easy to parallel)
 *
 * @param I (i)    - image ("packed")
 * @param W,H      - size of the image (W - width in pixels)
 * @param CC (o)   - connected components boxes (numeration starts from 1!!!), so first box is CC->box[1], amount of boxes is CC->Nobj-1 !!!
 * @return an array of labeled components
 */
size_t *ilCClabel4(uint8_t *Img, int W, int H, ilConnComps **CC){
    size_t *assoc;
    if(W < MINWIDTH || H < MINHEIGHT) return NULL;
    uint8_t *f = ilfilter4(Img, W, H); // remove all non 4-connected pixels
    //DBG("convert to size_t");
    size_t *labels = ilbin2sizet(f, W, H);
    FREE(f);
    //DBG("Calculate");
    size_t Nmax = W*H/4; // max number of 4-connected labels
    assoc = MALLOC(size_t, Nmax); // allocate memory for "remark" array
    size_t last_assoc_idx = 1; // last index filled in assoc array
    for(int y = 0; y < H; ++y){
        bool found = false;
        size_t *ptr = &labels[y*W];
        size_t curmark = 0; // mark of pixel to the left
        for(int x = 0; x < W; ++x, ++ptr){
            if(!*ptr){found = false; continue;} // empty pixel
            size_t U = (y) ? ptr[-W] : 0; // upper mark
            if(found){ // there's a pixel to the left
                if(U && U != curmark){ // meet old mark -> remark one of them in assoc[]
                    TEST("(%d, %d): remark %zd --> %zd\n", x, y, U, curmark);
                    remark(U, curmark, assoc);
                    curmark = U; // change curmark to upper mark (to reduce further checks)
                }
            }else{ // new mark -> change curmark
                found = true;
                if(U) curmark = U; // current mark will copy upper value
                else{ // current mark is new value
                    curmark = last_assoc_idx++;
                    assoc[curmark] = curmark;
                    TEST("(%d, %d): new mark=%zd\n", x, y, curmark);
                }
            }
            *ptr = curmark;
        }
    }
    size_t *indexes = MALLOC(size_t, last_assoc_idx); // new indexes
    size_t cidx = 1;
    TEST("\n\n\nRebuild indexes\n\n");
    for(size_t i = 1; i < last_assoc_idx; ++i){
        TEST("%zd\t%zd ",i,assoc[i]);
        // search new index
        register size_t realnew = i, newval = 0;
        do{
            realnew = assoc[realnew];
            TEST("->%zd ", realnew);
            if(indexes[realnew]){ // find least index
                newval = indexes[realnew];
                TEST("real: %zd ", newval);
                break;
            }
        }while(assoc[realnew] != realnew);
        if(newval){
            TEST(" ==> %zd\n", newval);
            indexes[i] = newval;
            continue;
        }
        TEST("new index %zd\n", cidx);
        // enter next label
        indexes[i] = cidx++;
    }
    // cidx now is amount of detected objects + 1 - size of output array (0th idx is not used)
    //DBG("amount after rebuild: %zd", cidx-1);
    #ifdef TESTMSGS
    printf("\n\n\nI\tASS[I]\tIDX[I]\n");
    for(size_t i = 1; i < last_assoc_idx; ++i)
        printf("%zd\t%zd\t%zd\n",i,assoc[i],indexes[i]);
    #endif
    ilBox *boxes = MALLOC(ilBox, cidx);
    OMP_FOR()
    for(size_t i = 1; i < cidx; ++i){ // init borders
        boxes[i].xmin = W;
        boxes[i].ymin = H;
    }
#pragma omp parallel shared(boxes)
    {
        ilBox *l_boxes = MALLOC(ilBox, cidx);
        for(size_t i = 1; i < cidx; ++i){ // init borders
            l_boxes[i].xmin = W;
            l_boxes[i].ymin = H;
        }
        #pragma omp for nowait
    for(int y = 0; y < H; ++y){
        size_t *lptr = &labels[y*W];
        for(int x = 0; x < W; ++x, ++lptr){
            if(!*lptr) continue;
            register size_t mark = indexes[*lptr];
            *lptr = mark;
            ilBox *b = &l_boxes[mark];
            ++b->area;
            if(b->xmax < x) b->xmax = x;
            if(b->xmin > x) b->xmin = x;
            if(b->ymax < y) b->ymax = y;
            if(b->ymin > y) b->ymin = y;
        }
    }
    #pragma omp critical
    for(size_t i = 1; i < cidx; ++i){
        ilBox *ob = &boxes[i], *ib = &l_boxes[i];
        if(ob->xmax < ib->xmax) ob->xmax = ib->xmax;
        if(ob->xmin > ib->xmin) ob->xmin = ib->xmin;
        if(ob->ymax < ib->ymax) ob->ymax = ib->ymax;
        if(ob->ymin > ib->ymin) ob->ymin = ib->ymin;
        ob->area += ib->area;
    }
    FREE(l_boxes);
    }
    FREE(assoc);
    FREE(indexes);
#ifdef TESTMSGS
    for(size_t i = 1; i < cidx; ++i){
        printf("%8zd\t%6d\t(%4d..%4d, %4d..%4d)\t%.2f\n", i, boxes[i].area,
               boxes[i].xmin, boxes[i].xmax, boxes[i].ymin, boxes[i].ymax,
               (1.+boxes[i].xmax-boxes[i].xmin)/(1.+boxes[i].ymax-boxes[i].ymin));
    }printf("\n\n");
#endif
    if(CC){
        *CC = MALLOC(ilConnComps, 1);
        (*CC)->Nobj = cidx; (*CC)->boxes = boxes;
    }else{
        FREE(boxes);
    }
    return labels;
}

#if 0
// label 8-connected components, look cclabel4
size_t *cclabel8(size_t *labels, int W, int H, size_t *Nobj){
    if(W < MINWIDTH || H < MINHEIGHT) return NULL;
    #define LABEL_8
    #include "cclabling.h"
    #undef LABEL_8
    return labels;
}
#endif

/*
 * <=================== CONNECTED COMPONENTS LABELING ===================
 */


/*
 * <=================== template ===================>
 */
