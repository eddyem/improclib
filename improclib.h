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

#pragma once

#include <stdint.h>
#include <stdlib.h>  // size_t

/*================================================================================*
 *                              basic types                                       *
 *================================================================================*/
// 3-channel image for saving into jpg/png
typedef struct{
    uint8_t *data;  // image data
    int width;      // width
    int height;     // height
} ilImg3;

// 1-channel image - pattern
typedef struct{
    uint8_t *data;  // image data
    int width;      // width
    int height;     // height
} ilPattern;

typedef enum{
    IMTYPE_U8,      // uint8_t
    IMTYPE_U16,     // uint16_t
    IMTYPE_U32,     // uint32_t
    IMTYPE_F,       // float
    IMTYPE_D,       // double
    IMTYPE_AMOUNT
} ilimtype_t;

typedef struct{
    int width;			// width
    int height;			// height
    ilimtype_t type;    // data type
    int pixbytes;       // size of one pixel data (bytes)
    void *data;         // picture data
    double minval;      // extremal data values
    double maxval;
} ilImage;

// input file/directory type
typedef enum{
    T_WRONG,
    T_DIRECTORY,
    T_BMP,
    T_GIF,
    T_JPEG,
    T_PNG,
    T_FITS,     // only to check type: FITS are supported in fitsmaniplib
    T_GZIP,
    T_AMOUNT
} ilInputType;

/*================================================================================*
 *                                converttypes.c                                  *
 *================================================================================*/
ilImage *ilu82Image(const uint8_t *data, int width, int height);
uint8_t *ilImage2u8(ilImage *I, int nchannels);
ilImage *ilbin2Image(const uint8_t *image, int W, int H);
uint8_t *ilImage2bin(const ilImage *im, double bk);
size_t *ilbin2sizet(const uint8_t *image, int W, int H);

/*================================================================================*
 *                                   draw.c                                       *
 *================================================================================*/
extern const uint8_t ilColor_red[3], ilColor_green[3], ilColor_blue[3], ilColor_black[3], ilColor_white[3];

ilPattern *ilPattern_new(int w, int h);
void ilPattern_free(ilPattern **I);
ilImg3 *ilImg3_new(int w, int h);
void ilImg3_free(ilImg3 **I3);

ilPattern *ilPattern_cross(int w, int h);
ilPattern *ilPattern_xcross(int w, int h);
ilPattern *ilPattern_star(int w, int h, double fwhm, double beta);
ilImage *ilImage_star(ilimtype_t type, int w, int h, double fwhm, double beta);

void ilImage_addsub(ilImage *img, const ilImage *p, int xc, int yc, double weight);
void ilImage_drawpix(ilImage *I, int x, int y, const void *val);
void ilImage_drawline(ilImage *I, int x0, int y0, int x1, int y1, const void *val);
void ilImage_drawcircle(ilImage *I, int x0, int y0, int R, const void *val);

void ilImg3_drawpattern(ilImg3 *img, const ilPattern *p, int xc, int yc, const uint8_t color[3]);
void ilImg3_setcolor(uint8_t impixel[3], const uint8_t color[3]);
void ilImg3_drawpix(ilImg3 *img, int x, int y, const uint8_t color[3]);
void ilImg3_drawline(ilImg3 *img, int x0, int y0, int x1, int y1, const uint8_t color[3]);
void ilImg3_drawcircle(ilImg3 *I, int x0, int y0, int R, const uint8_t color[3]);
void ilImg3_drawgrid(ilImg3 *img, int x0, int y0, int xstep, int ystep, const uint8_t color[3]);

ilImg3 *ilImg3_subimage(const ilImg3 *I, int x0, int y0, int x1, int y1);

/*================================================================================*
 *                                 imagefile.c                                    *
 *================================================================================*/
int ilgetpixbytes(ilimtype_t type);
void ilImage_minmax(ilImage *I);
uint8_t *ilequalize8(ilImage *I, int nchannels, double throwpart);
uint8_t *ilequalize16(ilImage *I, int nchannels, double throwpart);

ilInputType ilchkinput(const char *name);
ilImage *ilImage_read(const char *name);
ilImage *ilImage_new(int w, int h, ilimtype_t type);
ilImage *ilImage_sim(const ilImage *i);
void ilImage_free(ilImage **I);

size_t *ilhistogram8(const ilImage *I);
size_t *ilhistogram16(const ilImage *I);
int ilImage_background(ilImage *img, double *bkg);

ilImg3 *ilImg3_read(const char *name);
int ilImg3_jpg(const char *name, ilImg3 *I3, int quality);
int ilImg3_png(const char *name, ilImg3 *I3);
int ilwrite_jpg(const char *name, int w, int h, int ncolors, uint8_t *bytes, int quality);
int ilwrite_png(const char *name, int w, int h, int ncolors, uint8_t *bytes);

/*================================================================================*
 *                                letters.c                                       *
 *================================================================================*/
int ilImage_putstring(ilImage *I, const char *str, int x, int y);
int ilImg3_putstring(ilImg3 *I, const char *str, int x, int y, const uint8_t color[3]);


/*================================================================================*
 *                                random.c                                        *
 *================================================================================*/
double ilNormalBase();
double ilNormal(double mean, double std);
void ilNormalPair(double *x, double *y, double xmean, double ymean, double xstd, double ystd);

int ilPoissonSetStep(double s);
int ilPoisson(double lambda);
void ilImage_addPoisson(ilImage *I, double lambda);
void ilImg3_addPoisson(ilImg3 *I, double lambda);

/*================================================================================*
 *                                    binmorph.c                                  *
 *================================================================================*/
// minimal image size for morph operations
#define MINWIDTH    (9)
#define MINHEIGHT   (3)

// simple box with given borders
typedef struct{
    uint16_t xmin;
    uint16_t xmax;
    uint16_t ymin;
    uint16_t ymax;
    uint32_t area; // total amount of object pixels inside the box
} ilBox;

typedef struct{
    size_t Nobj;
    ilBox *boxes;
} ilConnComps;

// morphological operations:
uint8_t *ildilation(const uint8_t *image, int W, int H);
uint8_t *ildilationN(const uint8_t *image, int W, int H, int N);
uint8_t *ilerosion(const uint8_t *image, int W, int H);
uint8_t *ilerosionN(const uint8_t *image, int W, int H, int N);
uint8_t *ilopeningN(uint8_t *image, int W, int H, int N);
uint8_t *ilclosingN(uint8_t *image, int W, int H, int N);
uint8_t *iltopHat(uint8_t *image, int W, int H, int N);
uint8_t *ilbotHat(uint8_t *image, int W, int H, int N);

// logical operations
uint8_t *ilimand(uint8_t *im1, uint8_t *im2, int W, int H);
uint8_t *ilsubstim(uint8_t *im1, uint8_t *im2, int W, int H);

// clear non 4-connected pixels
uint8_t *ilfilter4(uint8_t *image, int W, int H);
// clear single pixels
uint8_t *ilfilter8(uint8_t *image, int W, int H);

size_t *ilCClabel4(uint8_t *Img, int W, int H, ilConnComps **CC);
//size_t *ilcclabel8(uint8_t *Img, int W, int H, size_t *Nobj);

/*================================================================================*
 *                                                                                *
 *================================================================================*/
