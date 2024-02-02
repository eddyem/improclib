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

#include <usefull_macros.h>
#include <stdio.h>

#include "improclib.h"
#include "openmp.h"

/**
 * @brief bin2Im - convert binarized image into uint8t
 * @param image - binarized image
 * @param W, H  - its size (in pixels!)
 * @return Image structure
 */
il_Image *il_bin2Image(const uint8_t *image, int W, int H){
    il_Image *ret = il_Image_new(W, H, IMTYPE_U8);
    int stride = (W + 7) / 8, s1 = (stride*8 == W) ? stride : stride - 1;
    uint8_t *data = (uint8_t*) ret->data;
    int rest = W - s1*8;
    OMP_FOR()
    for(int y = 0; y < H; y++){
        uint8_t *optr = &data[y*W];
        const uint8_t *iptr = &image[y*stride];
        for(int x = 0; x < s1; x++){
            register uint8_t inp = *iptr++;
            for(int i = 0; i < 8; ++i){
                *optr++ = (inp & 0x80) ? 255 : 0;
                inp <<= 1;
            }
        }
        if(rest){
            register uint8_t inp = *iptr;
            for(int i = 0; i < rest; ++i){
                *optr++ = (inp & 0x80) ? 255 : 0;
                inp <<= 1;
            }
        }
    }
    ret->minval = 0;
    ret->maxval = 255;
    return ret;
}

/**
 * Convert image into pseudo-packed (1 char == 8 pixels), all values > bk will be 1, else - 0
 * @param im (i)     - image to convert
 * @param stride (o) - new width of image
 * @param bk         - background level (all values < bk will be 0, other will be 1)
 * @return allocated memory area with "packed" image
 */
uint8_t *il_Image2bin(const il_Image *im, double bk){
    if(!im) return NULL;
    if(im->type != IMTYPE_U8){
        WARNX("ilImage2bin(): supported only 8-bit images");
        return NULL;
    }
    int W = im->width, H = im->height;
    if(W < 2 || H < 2) return NULL;
    int y, W0 = (W + 7) / 8, s1 = (W/8 == W0) ? W0 : W0 - 1;
    uint8_t *ret = MALLOC(uint8_t, W0 * H);
    uint8_t *data = (uint8_t*) im->data;
    int rest = W - s1*8;
    //OMP_FOR()
    for(y = 0; y < H; ++y){
        uint8_t *iptr = &data[y*W];
        uint8_t *optr = &ret[y*W0];
        for(int x = 0; x < s1; ++x){
            register uint8_t o = 0;
            for(int i = 0; i < 8; ++i){
                o <<= 1;
                if(*iptr++ > bk) o |= 1;
            }
            *optr++ = o;
        }
        if(rest){
            register uint8_t o = 0;
            for(int x = 0; x < rest; ++x){
                o <<= 1;
                if(*iptr++ > bk) o |= 1;
            }
            *optr = o << (8 - rest);
        }
    }
    return ret;
}

// transformation functions for Im2u8
#define TRANSMACRO(datatype)  \
    int height = I->height, width = I->width, stride = width * nchannels;  \
    uint8_t *outp = MALLOC(uint8_t, height * stride); \
    double min = I->minval, max = I->maxval, W = 255./(max - min); \
    datatype *Idata = (datatype*) I->data; \
    if(nchannels == 3){ \
        OMP_FOR() \
        for(int y = 0; y < height; ++y){ \
            uint8_t *Out = &outp[y*stride]; \
            datatype *In = &Idata[y*width]; \
            for(int x = 0; x < width; ++x){ \
                Out[0] = Out[1] = Out[2] = (uint8_t)(W*((double)(*In++) - min)); \
                Out += 3; \
            } \
        } \
    }else{ \
        OMP_FOR() \
        for(int y = 0; y < height; ++y){ \
            uint8_t *Out = &outp[y*stride]; \
            datatype *In = &Idata[y*width]; \
            for(int x = 0; x < width; ++x){ \
                *Out++ = (uint8_t)(W*((double)(*In++) - min)); \
            } \
        } \
    }

static uint8_t *Iu8(const il_Image *I, int nchannels){
    TRANSMACRO(uint8_t);
    return outp;
}
static uint8_t *Iu16(const il_Image *I, int nchannels){
    TRANSMACRO(uint16_t);
    return outp;
}
static uint8_t *Iu32(const il_Image *I, int nchannels){
    TRANSMACRO(uint32_t);
    return outp;
}
static uint8_t *If(const il_Image *I, int nchannels){
    TRANSMACRO(float);
    return outp;
}
static uint8_t *Id(const il_Image *I, int nchannels){
    TRANSMACRO(double);
    return outp;
}

/**
 * @brief il_Image2u8 - linear transform for preparing file to save as JPEG or other type
 * @param I - input image
 * @param nchannels - 1 or 3 colour channels
 * @return allocated here image for jpeg/png storing
 */
uint8_t *il_Image2u8(il_Image *I, int nchannels){ // only 1 and 3 channels supported!
    if(!I || !I->data || (nchannels != 1 && nchannels != 3)) return NULL;
    il_Image_minmax(I);
    //DBG("make linear transform %dx%d, %d channels", I->width, I->height, nchannels);
    switch(I->type){
        case IMTYPE_U8:
            return Iu8(I, nchannels);
        break;
        case IMTYPE_U16:
            return Iu16(I, nchannels);
        break;
        case IMTYPE_U32:
            return Iu32(I, nchannels);
        break;
        case IMTYPE_F:
            return If(I, nchannels);
        break;
        case IMTYPE_D:
            return Id(I, nchannels);
        break;
        default:
            WARNX("Im2u8: unsupported image type %d", I->type);
    }
    return NULL;
}

#if 0
UNUSED function! Need to be refactored
// convert size_t labels into Image
Image *ST2Im(const size_t *image, int W, int H){
    Image *ret = Image_new(W, H);
    OMP_FOR()
    for(int y = 0; y < H; ++y){
        Imtype *optr = &ret->data[y*W];
        const size_t *iptr = &image[y*W];
        for(int x = 0; x < W; ++x){
            *optr++ = (Imtype)*iptr++;
        }
    }
    Image_minmax(ret);
    return ret;
}
#endif

/**
 * Convert "packed" image into size_t array for conncomp procedure
 * @param image (i) - input image
 * @param W, H      - size of image in pixels
 * @return allocated memory area with copy of an image
 */
size_t *il_bin2sizet(const uint8_t *image, int W, int H){
    size_t *ret = MALLOC(size_t, W * H);
    int W0 = (W + 7) / 8, s1 = W0 - 1;
    OMP_FOR()
    for(int y = 0; y < H; y++){
        size_t *optr = &ret[y*W];
        const uint8_t *iptr = &image[y*W0];
        for(int x = 0; x < s1; ++x){
            register uint8_t inp = *iptr++;
            for(int i = 0; i < 8; ++i){
                *optr++ = (inp & 0x80) ? 1 : 0;
                inp <<= 1;
            }
        }
        int rest = W - s1*8;
        if(rest){
            register uint8_t inp = *iptr;
            for(int i = 0; i < rest; ++i){
                *optr++ = (inp & 0x80) ? 1 : 0;
                inp <<= 1;
            }
        }
    }
    return ret;
}
