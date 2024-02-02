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
} il_Img3;

// 1-channel image - pattern
typedef struct{
    uint8_t *data;  // image data
    int width;      // width
    int height;     // height
} il_Pattern;

typedef enum{
    IMTYPE_U8,      // uint8_t
    IMTYPE_U16,     // uint16_t
    IMTYPE_U32,     // uint32_t
    IMTYPE_F,       // float
    IMTYPE_D,       // double
    IMTYPE_AMOUNT
} il_imtype_t;

typedef struct{
    int width;			// width
    int height;			// height
    il_imtype_t type;    // data type
    int pixbytes;       // size of one pixel data (bytes)
    void *data;         // picture data
    double minval;      // extremal data values
    double maxval;
} il_Image;

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
} il_InputType;

/*================================================================================*
 *                                converttypes.c                                  *
 *================================================================================*/
il_Image *il_u82Image(const uint8_t *data, int width, int height);
uint8_t *il_Image2u8(il_Image *I, int nchannels);
il_Image *il_bin2Image(const uint8_t *image, int W, int H);
uint8_t *il_Image2bin(const il_Image *im, double bk);
size_t *il_bin2sizet(const uint8_t *image, int W, int H);

/*================================================================================*
 *                                   draw.c                                       *
 *================================================================================*/
extern const uint8_t il_Color_red[3], il_Color_green[3], il_Color_blue[3], il_Color_black[3], il_Color_white[3];

il_Pattern *il_Pattern_new(int w, int h);
void il_Pattern_free(il_Pattern **I);
il_Img3 *il_Img3_new(int w, int h);
void il_Img3_free(il_Img3 **I3);

il_Pattern *il_Pattern_cross(int w, int h);
il_Pattern *il_Pattern_xcross(int w, int h);
il_Pattern *il_Pattern_star(int w, int h, double fwhm, double beta);
il_Image *il_Image_star(il_imtype_t type, int w, int h, double fwhm, double beta);

void il_Image_addsub(il_Image *img, const il_Image *p, int xc, int yc, double weight);
void il_Image_drawpix(il_Image *I, int x, int y, const void *val);
void il_Image_drawline(il_Image *I, int x0, int y0, int x1, int y1, const void *val);
void il_Image_drawcircle(il_Image *I, int x0, int y0, int R, const void *val);

void il_Img3_drawpattern(il_Img3 *img, const il_Pattern *p, int xc, int yc, const uint8_t color[3]);
void il_Img3_setcolor(uint8_t impixel[3], const uint8_t color[3]);
void il_Img3_drawpix(il_Img3 *img, int x, int y, const uint8_t color[3]);
void il_Img3_drawline(il_Img3 *img, int x0, int y0, int x1, int y1, const uint8_t color[3]);
void il_Img3_drawcircle(il_Img3 *I, int x0, int y0, int R, const uint8_t color[3]);
void il_Img3_drawgrid(il_Img3 *img, int x0, int y0, int xstep, int ystep, const uint8_t color[3]);

il_Img3 *il_Img3_subimage(const il_Img3 *I, int x0, int y0, int x1, int y1);

/*================================================================================*
 *                                 imagefile.c                                    *
 *================================================================================*/
int il_getpixbytes(il_imtype_t type);
void il_Image_minmax(il_Image *I);
uint8_t *il_equalize8(il_Image *I, int nchannels, double throwpart);
uint8_t *il_equalize16(il_Image *I, int nchannels, double throwpart);

il_InputType il_chkinput(const char *name);
il_Image *il_Image_read(const char *name);
il_Image *il_Image_new(int w, int h, il_imtype_t type);
il_Image *il_Image_sim(const il_Image *i);
void il_Image_free(il_Image **I);

size_t *il_histogram8(const il_Image *I);
size_t *il_histogram16(const il_Image *I);
int il_Image_background(il_Image *img, double *bkg);

il_Img3 *il_Img3_read(const char *name);
int il_Img3_jpg(const char *name, il_Img3 *I3, int quality);
int il_Img3_png(const char *name, il_Img3 *I3);
int il_write_jpg(const char *name, int w, int h, int ncolors, uint8_t *bytes, int quality);
int il_write_png(const char *name, int w, int h, int ncolors, uint8_t *bytes);

/*================================================================================*
 *                                letters.c                                       *
 *================================================================================*/
int il_Image_putstring(il_Image *I, const char *str, int x, int y);
int il_Img3_putstring(il_Img3 *I, const char *str, int x, int y, const uint8_t color[3]);


/*================================================================================*
 *                                random.c                                        *
 *================================================================================*/
double il_NormalBase();
double il_Normal(double mean, double std);
void il_NormalPair(double *x, double *y, double xmean, double ymean, double xstd, double ystd);

int il_PoissonSetStep(double s);
int il_Poisson(double lambda);
void il_Image_addPoisson(il_Image *I, double lambda);
void il_Img3_addPoisson(il_Img3 *I, double lambda);

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
} il_Box;

typedef struct{
    size_t Nobj;
    il_Box *boxes;
} il_ConnComps;

// morphological operations:
uint8_t *il_dilation(const uint8_t *image, int W, int H);
uint8_t *il_dilationN(const uint8_t *image, int W, int H, int N);
uint8_t *il_erosion(const uint8_t *image, int W, int H);
uint8_t *il_erosionN(const uint8_t *image, int W, int H, int N);
uint8_t *il_openingN(uint8_t *image, int W, int H, int N);
uint8_t *il_closingN(uint8_t *image, int W, int H, int N);
uint8_t *il_topHat(uint8_t *image, int W, int H, int N);
uint8_t *il_botHat(uint8_t *image, int W, int H, int N);

// logical operations
uint8_t *il_imand(uint8_t *im1, uint8_t *im2, int W, int H);
uint8_t *il_substim(uint8_t *im1, uint8_t *im2, int W, int H);

// clear non 4-connected pixels
uint8_t *il_filter4(uint8_t *image, int W, int H);
// clear single pixels
uint8_t *il_filter8(uint8_t *image, int W, int H);

size_t *il_CClabel4(uint8_t *Img, int W, int H, il_ConnComps **CC);
//size_t *il_cclabel8(uint8_t *Img, int W, int H, size_t *Nobj);

/*================================================================================*
 *                                                                                *
 *================================================================================*/
