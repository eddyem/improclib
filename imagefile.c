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

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#include "improclib.h"
#include "openmp.h"

typedef struct{
    const char signature[8];
    uint8_t len;
    ilInputType it;
} imsign;

const imsign signatures[] = {
    {"BM", 2, T_BMP},
    {"SIMPLE", 6, T_FITS},
    {{0x1f, 0x8b, 0x08}, 3, T_GZIP},
    {"GIF8", 4, T_GIF},
    {{0xff, 0xd8, 0xff, 0xdb}, 4, T_JPEG},
    {{0xff, 0xd8, 0xff, 0xe0}, 4, T_JPEG},
    {{0xff, 0xd8, 0xff, 0xe1}, 4, T_JPEG},
    {{0x89, 0x50, 0x4e, 0x47}, 4, T_PNG},
   // {{0x49, 0x49, 0x2a, 0x00}, 4, T_TIFF},
    {"", 0, T_WRONG}
};

#ifdef EBUG
static char *hexdmp(const char sig[8]){
    static char buf[128];
    char *bptr = buf;
    bptr += sprintf(bptr, "[ ");
    for(int i = 0; i < 7; ++i){
        bptr += sprintf(bptr, "%02X ", (uint8_t)sig[i]);
    }
    bptr += sprintf(bptr, "]");
    return buf;
}
#endif

const int bytes[IMTYPE_AMOUNT] = {
    [IMTYPE_U8] = 1,
    [IMTYPE_U16] = 2,
    [IMTYPE_U32] = 4,
    [IMTYPE_F] = sizeof(float),
    [IMTYPE_D] = sizeof(double)
};

// return amount of bytes per pixel for given image type
int ilgetpixbytes(ilimtype_t type){
    if(type >= IMTYPE_AMOUNT) return -1;
    return bytes[type];
}

/**
 * @brief imtype - check image type of given file
 * @param f - opened image file structure
 * @return image type or T_WRONG
 */
static ilInputType imtype(FILE *f){
    char signature[8];
    int x = fread(signature, 1, 7, f);
    DBG("x=%d", x);
    if(7 != x){
        WARN("Can't read file signature");
        return T_WRONG;
    }
    signature[7] = 0;
    const imsign *s = signatures;
    DBG("Got signature: %s (%s)", hexdmp(signature), signature);
    while(s->len){
        DBG("Check %s", s->signature);
        if(0 == memcmp(s->signature, signature, s->len)){
            DBG("Found signature %s", s->signature);
            return s->it;
        }
        ++s;
    }
    return T_WRONG;
}

/**
 * @brief ilchkinput - check file/directory name
 * @param name - name of file or directory
 * @return type of `name`
 */
ilInputType ilchkinput(const char *name){
    DBG("input name: %s", name);
    struct stat fd_stat;
    if(stat(name, &fd_stat)){
        WARN("Can't stat() %s", name);
        return T_WRONG;
    }
    if(S_ISDIR(fd_stat.st_mode)){
        DBG("%s is a directory", name);
        DIR *d = opendir(name);
        if(!d){
            WARN("Can't open directory %s", name);
            return T_WRONG;
        }
        closedir(d);
        return T_DIRECTORY;
    }
    FILE *f = fopen(name, "r");
    if(!f){
        WARN("Can't open file %s", name);
        return T_WRONG;
    }
    ilInputType tp = imtype(f);
    DBG("Image type of %s is %d", name, tp);
    fclose(f);
    return tp;
}

/**
 * @brief ilu8toImage - convert uint8_t data to Image structure
 * @param data      - original image data
 * @param width     - image width
 * @param height    - image height
 * @param stride    - image width with alignment
 * @return Image structure (fully allocated, you can FREE(data) after it)
 */
ilImage *ilu8toImage(const uint8_t *data, int width, int height){
    FNAME();
    ilImage *outp = ilImage_new(width, height, IMTYPE_U8);
    memcpy(outp->data, data, width*height);
    ilImage_minmax(outp);
    return outp;
}

/**
 * @brief im_loadmono - load image file
 * @param name - filename
 * @return Image structure or NULL
 */
static inline ilImage *im_loadmono(const char *name){
    int width, height, channels;
    uint8_t *img = stbi_load(name, &width, &height, &channels, 1);
    if(!img){
        WARNX("Error in loading the image %s\n", name);
        return NULL;
    }
    ilImage *I = MALLOC(ilImage, 1);
    I->data = img;
    I->width = width;
    I->height = height;
    I->type = IMTYPE_U8;
    I->pixbytes = 1;
    ilImage_minmax(I);
    return I;
}

/**
 * @brief ilImage_read - read image from any supported file type
 * @param name - path to image
 * @return image or NULL if failed
 */
ilImage *ilImage_read(const char *name){
    ilInputType tp = ilchkinput(name);
    if(tp == T_DIRECTORY || tp == T_WRONG){
        WARNX("Bad file type to read");
        return NULL;
    }
    ilImage *outp = im_loadmono(name);
    return outp;
}

/**
 * @brief ilImg3_read - read color image from file 'name'
 * @param name - input file name
 * @return image read or NULL if error
 */
ilImg3 *ilImg3_read(const char *name){
    ilInputType tp = ilchkinput(name);
    if(tp == T_DIRECTORY || tp == T_WRONG){
        WARNX("Bad file type to read");
        return NULL;
    }
    ilImg3 *I = MALLOC(ilImg3, 1);
    int channels;
    I->data = stbi_load(name, &I->width, &I->height, &channels, 3);
    if(!I->data){
        FREE(I);
        return NULL;
    }
    return I;
}

/**
 * @brief ilImage_new - allocate memory for new struct Image & Image->data
 * @param w, h - image size
 * @return data allocated here
 */
ilImage *ilImage_new(int w, int h, ilimtype_t type){
    if(w < 1 || h < 1) return NULL;
    if(type >= IMTYPE_AMOUNT) return NULL;
    ilImage *o = MALLOC(ilImage, 1);
    o->width = w;
    o->height = h;
    o->type = type;
    o->pixbytes = ilgetpixbytes(type);
    o->data = calloc(w*h, o->pixbytes);
    if(!o->data) ERR("calloc()");
    return o;
}

/**
 * @brief ilImage_sim - allocate memory for new empty Image with similar size & data type
 * @param i - sample image
 * @return data allocated here (with empty keylist & zeros in data)
 */
ilImage *ilImage_sim(const ilImage *i){
    if(!i) return NULL;
    ilImage *outp = ilImage_new(i->width, i->height, i->type);
    return outp;
}

// free image data
void ilImage_free(ilImage **img){
    if(!img || !*img) return;
    FREE((*img)->data);
    FREE(*img);
}


/**
 * @brief ilhistogram8 - calculate image histogram for 8-bit image
 * @param I - orig
 * @return 256 byte array
 */
size_t *ilhistogram8(const ilImage *I){
    if(!I || !I->data || I->type != IMTYPE_U8) return NULL;
    size_t *histogram = MALLOC(size_t, 256);
    int wh = I->width * I->height;
    uint8_t *data = (uint8_t*)I->data;
#pragma omp parallel
{
    size_t histogram_private[256] = {0};
    #pragma omp for nowait
    for(int i = 0; i < wh; ++i){
        ++histogram_private[data[i]];
    }
    #pragma omp critical
    {
        for(int i = 0; i < 256; ++i) histogram[i] += histogram_private[i];
    }
}
    return histogram;
}

/**
 * @brief ilhistogram16 - calculate image histogram for 16-bit image
 * @param I - orig
 * @return 65536 byte array
 */
size_t *ilhistogram16(const ilImage *I){
    if(!I || !I->data || I->type != IMTYPE_U16) return NULL;
    size_t *histogram = MALLOC(size_t, 65536);
    int wh = I->width * I->height;
    uint16_t *data = (uint16_t*)I->data;
#pragma omp parallel
{
    size_t histogram_private[65536] = {0};
    #pragma omp for nowait
    for(int i = 0; i < wh; ++i){
        ++histogram_private[data[i]];
    }
    #pragma omp critical
    {
        for(int i = 0; i < 65536; ++i) histogram[i] += histogram_private[i];
    }
}
    return histogram;
}


/**
 * @brief calc_background - Simple background calculation by histogram
 * @param img (i) - input image (here will be modified its top2proc field)
 * @param bk (o)  - background value
 * @return 0 if error
 */
int ilImage_background(ilImage *img, double *bkg){
    if(!img || !img->data || !bkg) return FALSE;
    ilImage_minmax(img);
    if(img->maxval == img->minval){
        WARNX("Zero or overilluminated image!");
        return FALSE;
    }
    size_t *histogram = NULL;
    int histosize = 0;
    switch(img->type){
        case IMTYPE_U8:
            histogram = ilhistogram8(img);
            histosize = 256;
        break;
        case IMTYPE_U16:
            histogram = ilhistogram16(img);
            histosize = 65536;
        break;
        default:
        break;
    }
    if(!histogram){
        WARNX("calc_background() supported only 8- and 16-bit images");
        return FALSE;
    }

    size_t modeidx = 0, modeval = 0;
    for(int i = 0; i < histosize; ++i)
        if(modeval < histogram[i]){
            modeval = histogram[i];
            modeidx = i;
        }
    //DBG("Mode=%g @ idx%d (N=%d)", ((Imtype)modeidx / 255.)*ampl, modeidx, modeval);
    ssize_t *diff2 = MALLOC(ssize_t, histosize);
    int lastidx = histosize - 1;
    OMP_FOR()
    for(int i = 2; i < lastidx; ++i)
        diff2[i] = (histogram[i+2]+histogram[i-2]-2*histogram[i])/4;
    //green("HISTO:\n");
    //for(int i = 0; i < 256; ++i) printf("%d:\t%d\t%d\n", i, histogram[i], diff2[i]);
    FREE(histogram);
    if(modeidx < 2) modeidx = 2;
    if((int)modeidx > lastidx-1){
        WARNX("Overilluminated image");
        FREE(diff2);
        return FALSE; // very bad image: overilluminated
    }
    size_t borderidx = modeidx;
    for(int i = modeidx; i < lastidx; ++i){ // search bend-point by second derivate
        if(diff2[i] <= 0 && diff2[i+1] <= 0){
            borderidx = i; break;
        }
    }
    //DBG("borderidx=%d -> %d", borderidx, (borderidx+modeidx)/2);
    //*bk = (borderidx + modeidx) / 2;
    *bkg = borderidx;
    FREE(diff2);
    return TRUE;
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

static uint8_t *Iu8(const ilImage *I, int nchannels){
    TRANSMACRO(uint8_t);
    return outp;
}
static uint8_t *Iu16(const ilImage *I, int nchannels){
    TRANSMACRO(uint16_t);
    return outp;
}
static uint8_t *Iu32(const ilImage *I, int nchannels){
    TRANSMACRO(uint32_t);
    return outp;
}
static uint8_t *If(const ilImage *I, int nchannels){
    TRANSMACRO(float);
    return outp;
}
static uint8_t *Id(const ilImage *I, int nchannels){
    TRANSMACRO(double);
    return outp;
}

/**
 * @brief ilImage2u8 - linear transform for preparing file to save as JPEG or other type
 * @param I - input image
 * @param nchannels - 1 or 3 colour channels
 * @return allocated here image for jpeg/png storing
 */
uint8_t *ilImage2u8(ilImage *I, int nchannels){ // only 1 and 3 channels supported!
    if(!I || !I->data || (nchannels != 1 && nchannels != 3)) return NULL;
    ilImage_minmax(I);
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

/**
 * @brief equalize8 - 8bit image hystogram equalization
 * @param I - input image
 * @param nchannels - 1 or 3 colour channels
 * @param throwpart - which part of black pixels (from all amount) to throw away
 * @return allocated here image for jpeg/png storing
 */
uint8_t *ilequalize8(ilImage *I, int nchannels, double throwpart){
    if(!I || !I->data || (nchannels != 1 && nchannels != 3)) return NULL;
    ilImage_minmax(I);
    int width = I->width, height = I->height;
    size_t stride = width*nchannels, S = height*stride;
    size_t *orig_histo = ilhistogram8(I); // original hystogram (linear)
    if(!orig_histo) return NULL;
    uint8_t *outp = MALLOC(uint8_t, S);
    uint8_t eq_levls[256] = {0};   // levels to convert: newpix = eq_levls[oldpix]
    int s = width*height;
    int Nblack = 0, bpart = (int)(throwpart * (double)s);
    int startidx;
    // remove first part of black pixels
    for(startidx = 0; startidx < 256; ++startidx){
        Nblack += orig_histo[startidx];
        if(Nblack >= bpart) break;
    }
    ++startidx;
    //DBG("Throw %d (real: %d black) pixels, startidx=%d", bpart, Nblack, startidx);
    double part = (double)(s + 1. - Nblack) / 256., N = 0.;
    for(int i = startidx; i < 256; ++i){
        N += orig_histo[i];
        eq_levls[i] = (uint8_t)(N/part);
    }
    //for(int i = stopidx; i < 256; ++i) eq_levls[i] = 255;
#if 0
    DBG("Original / new histogram");
    for(int i = 0; i < 256; ++i) printf("%d\t%d\t%d\n", i, orig_hysto[i], eq_levls[i]);
#endif
    uint8_t *Idata = (uint8_t*)I->data;
    if(nchannels == 3){
        OMP_FOR()
        for(int y = 0; y < height; ++y){
            uint8_t *Out = &outp[y*stride];
            uint8_t *In = &Idata[y*width];
            for(int x = 0; x < width; ++x){
                Out[0] = Out[1] = Out[2] = eq_levls[*In++];
                Out += 3;
            }
        }
    }else{
        OMP_FOR()
        for(int y = 0; y < height; ++y){
            uint8_t *Out = &outp[y*width];
            uint8_t *In = &Idata[y*width];
            for(int x = 0; x < width; ++x){
                *Out++ = eq_levls[*In++];
            }
        }
    }
    FREE(orig_histo);
    return outp;
}

/**
 * @brief equalize16 - 16bit image hystogram equalization
 * @param I - input image
 * @param nchannels - 1 or 3 colour channels
 * @param throwpart - which part of black pixels (from all amount) to throw away
 * @return allocated here image for jpeg/png storing
 */
uint8_t *ilequalize16(ilImage *I, int nchannels, double throwpart){
    if(!I || !I->data || (nchannels != 1 && nchannels != 3)) return NULL;
    ilImage_minmax(I);
    int width = I->width, height = I->height;
    size_t stride = width*nchannels, S = height*stride;
    size_t *orig_histo = ilhistogram8(I); // original hystogram (linear)
    if(!orig_histo) return NULL;
    uint8_t *outp = MALLOC(uint8_t, S);
    uint16_t *eq_levls = MALLOC(uint16_t, 65536);   // levels to convert: newpix = eq_levls[oldpix]
    int s = width*height;
    int Nblack = 0, bpart = (int)(throwpart * (double)s);
    int startidx;
    // remove first part of black pixels
    for(startidx = 0; startidx < 65536; ++startidx){
        Nblack += orig_histo[startidx];
        if(Nblack >= bpart) break;
    }
    ++startidx;
    //DBG("Throw %d (real: %d black) pixels, startidx=%d", bpart, Nblack, startidx);
    double part = (double)(s + 1. - Nblack) / 65536., N = 0.;
    for(int i = startidx; i < 65536; ++i){
        N += orig_histo[i];
        eq_levls[i] = (uint8_t)(N/part);
    }
    uint8_t *Idata = (uint8_t*)I->data;
    if(nchannels == 3){
        OMP_FOR()
        for(int y = 0; y < height; ++y){
            uint8_t *Out = &outp[y*stride];
            uint8_t *In = &Idata[y*width];
            for(int x = 0; x < width; ++x){
                Out[0] = Out[1] = Out[2] = eq_levls[*In++];
                Out += 3;
            }
        }
    }else{
        OMP_FOR()
        for(int y = 0; y < height; ++y){
            uint8_t *Out = &outp[y*width];
            uint8_t *In = &Idata[y*width];
            for(int x = 0; x < width; ++x){
                *Out++ = eq_levls[*In++];
            }
        }
    }
    FREE(orig_histo);
    FREE(eq_levls);
    return outp;
}

static void u8minmax(ilImage *I){
    uint8_t *data = (uint8_t*)I->data;
    double min = *data, max = min;
    int wh = I->width * I->height;
    #pragma omp parallel shared(min, max)
    {
        double min_p = min, max_p = max;
        #pragma omp for nowait
        for(int i = 0; i < wh; ++i){
            double pixval = (double)data[i];
            if(pixval < min_p) min_p = pixval;
            else if(pixval > max_p) max_p = pixval;
        }
        #pragma omp critical
        {
            if(min > min_p) min = min_p;
            if(max < max_p) max = max_p;
        }
    }
    I->maxval = max;
    I->minval = min;
}
static void u16minmax(ilImage *I){
    uint16_t *data = (uint16_t*)I->data;
    double min = *data, max = min;
    int wh = I->width * I->height;
    #pragma omp parallel shared(min, max)
    {
        double min_p = min, max_p = max;
        #pragma omp for nowait
        for(int i = 0; i < wh; ++i){
            double pixval = (double)data[i];
            if(pixval < min_p) min_p = pixval;
            else if(pixval > max_p) max_p = pixval;
        }
        #pragma omp critical
        {
            if(min > min_p) min = min_p;
            if(max < max_p) max = max_p;
        }
    }
    I->maxval = max;
    I->minval = min;
}
static void u32minmax(ilImage *I){
    uint32_t *data = (uint32_t*)I->data;
    double min = *data, max = min;
    int wh = I->width * I->height;
    #pragma omp parallel shared(min, max)
    {
        double min_p = min, max_p = max;
        #pragma omp for nowait
        for(int i = 0; i < wh; ++i){
            double pixval = (double)data[i];
            if(pixval < min_p) min_p = pixval;
            else if(pixval > max_p) max_p = pixval;
        }
        #pragma omp critical
        {
            if(min > min_p) min = min_p;
            if(max < max_p) max = max_p;
        }
    }
    I->maxval = max;
    I->minval = min;
}
static void fminmax(ilImage *I){
    float *data = (float*)I->data;
    double min = *data, max = min;
    int wh = I->width * I->height;
    #pragma omp parallel shared(min, max)
    {
        double min_p = min, max_p = max;
        #pragma omp for nowait
        for(int i = 0; i < wh; ++i){
            double pixval = (double)data[i];
            if(pixval < min_p) min_p = pixval;
            else if(pixval > max_p) max_p = pixval;
        }
        #pragma omp critical
        {
            if(min > min_p) min = min_p;
            if(max < max_p) max = max_p;
        }
    }
    I->maxval = max;
    I->minval = min;
}
static void dminmax(ilImage *I){
    double *data = (double*)I->data;
    double min = *data, max = min;
    int wh = I->width * I->height;
    #pragma omp parallel shared(min, max)
    {
        double min_p = min, max_p = max;
        #pragma omp for nowait
        for(int i = 0; i < wh; ++i){
            double pixval = (double)data[i];
            if(pixval < min_p) min_p = pixval;
            else if(pixval > max_p) max_p = pixval;
        }
        #pragma omp critical
        {
            if(min > min_p) min = min_p;
            if(max < max_p) max = max_p;
        }
    }
    I->maxval = max;
    I->minval = min;
}

// calculate extremal values of image data and store them in it
void ilImage_minmax(ilImage *I){
    if(!I || !I->data) return;
#ifdef EBUG
    double t0 = dtime();
#endif
    switch(I->type){
        case IMTYPE_U8:
            u8minmax(I);
        break;
        case IMTYPE_U16:
            u16minmax(I);
        break;
        case IMTYPE_U32:
            u32minmax(I);
        break;
        case IMTYPE_F:
            fminmax(I);
        break;
        case IMTYPE_D:
            dminmax(I);
        break;
        default:
            return;
    }
    DBG("Image_minmax(): Min=%g, Max=%g, time: %gms", I->minval, I->maxval, (dtime()-t0)*1e3);
}

/*
 * =================== CONVERT IMAGE TYPES ===================>
 */

/**
 * @brief bin2Im - convert binarized image into uint8t
 * @param image - binarized image
 * @param W, H  - its size (in pixels!)
 * @return Image structure
 */
ilImage *ilbin2Image(const uint8_t *image, int W, int H){
    ilImage *ret = ilImage_new(W, H, IMTYPE_U8);
    int stride = (W + 7) / 8, s1 = (stride*8 == W) ? stride : stride - 1;
    uint8_t *data = (uint8_t*) ret->data;
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
        int rest = W - s1*8;
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
uint8_t *ilImage2bin(const ilImage *im, double bk){
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
    OMP_FOR()
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
        int rest = W - s1*8;
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
size_t *ilbin2sizet(const uint8_t *image, int W, int H){
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


/*
 * <=================== CONVERT IMAGE TYPES ===================
 */

/*
 * =================== SAVE IMAGES ===========================>
 */

/**
 * @brief write_jpg - write jpeg file
 * @param name - filename
 * @param w - width
 * @param h - height
 * @param ncolors - 1 or 3 (mono/color)
 * @param bytes - image data
 * @param quality - jpeg quality
 * @return FALSE if failed
 */
int ilwrite_jpg(const char *name, int w, int h, int ncolors, uint8_t *bytes, int quality){
    if(!bytes || !name || quality < 5 || quality > 100 || (ncolors != 1 && ncolors != 3) || w < 1 || h < 1) return FALSE;
    char *tmpnm = MALLOC(char, strlen(name) + 10);
    sprintf(tmpnm, "%s-tmp.jpg", name);
    int r = stbi_write_jpg(tmpnm, w, h, ncolors, bytes, quality);
    if(r){
        if(rename(tmpnm, name)){
            WARN("rename()");
            r = FALSE;
        }
    }
    FREE(tmpnm);
    return r;
}

/**
 * @brief ilImg3_jpg - save Img3 as jpeg
 * @param name - output file name
 * @param I3 - image
 * @param quality - jpeg quality
 * @return FALSE if failed
 */
int ilImg3_jpg(const char *name, ilImg3 *I3, int quality){
    if(!name || quality < 5 || quality > 100 || !I3 || !I3->data || I3->width < 1 || I3->height < 1) return FALSE;
    char *tmpnm = MALLOC(char, strlen(name) + 10);
    sprintf(tmpnm, "%s-tmp.jpg", name);
    int r = stbi_write_jpg(tmpnm, I3->width, I3->height, 3, I3->data, quality);
    if(r){
        if(rename(tmpnm, name)){
            WARN("rename()");
            r = FALSE;
        }
    }
    FREE(tmpnm);
    return r;
}

/**
 * @brief write_png - write png file
 * @param name - filename
 * @param w - width
 * @param h - height
 * @param ncolors - 1 or 3 (mono/color)
 * @param bytes - image data
 * @return FALSE if failed
 */
int ilwrite_png(const char *name, int w, int h, int ncolors, uint8_t *bytes){
    if(!bytes || !name || (ncolors != 1 && ncolors != 3) || w < 1 || h < 1) return FALSE;
    char *tmpnm = MALLOC(char, strlen(name) + 10);
    sprintf(tmpnm, "%s-tmp.jpg", name);
    int r = stbi_write_png(tmpnm, w, h, ncolors, bytes, w);
    if(r){
        if(rename(tmpnm, name)){
            WARN("rename()");
            r = FALSE;
        }
    }
    FREE(tmpnm);
    return r;
}

/**
 * @brief ilImg3_png - save Img3 as jpeg
 * @param name - output file name
 * @param I3 - image
 * @param quality - jpeg quality
 * @return FALSE if failed
 */
int ilImg3_png(const char *name, ilImg3 *I3){
    if(!name || !I3 || !I3->data || I3->width < 1 || I3->height < 1) return FALSE;
    char *tmpnm = MALLOC(char, strlen(name) + 10);
    sprintf(tmpnm, "%s-tmp.png", name);
    int r = stbi_write_png(tmpnm, I3->width, I3->height, 3, I3->data, 0);
    if(r){
        if(rename(tmpnm, name)){
            WARN("rename()");
            r = FALSE;
        }
    }
    FREE(tmpnm);
    return r;
}


/*
 * <=================== SAVE IMAGES ===========================
 */
