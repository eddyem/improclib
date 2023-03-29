/*
 * This file is part of the loccorr project.
 * Copyright 2021 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

// simplest interface to draw lines & ellipses

#include <usefull_macros.h>
#include <math.h>
#include <stdio.h>

#include "improclib.h"
#include "openmp.h"

#ifndef ABS
#define ABS(x)  (((x) > 0) ? (x) : -(x))
#endif

// base colors:
const uint8_t
    ilColor_red[3] = {255, 0, 0},
    ilColor_green[3] = {0, 255, 0},
    ilColor_blue[3] = {0, 0, 255},
    ilColor_black[3] = {0, 0, 0},
    ilColor_white[3] = {255,255,255};

ilImg3 *ilImg3_new(int w, int h){
    if(w < 1 || h < 1) return NULL;
    ilImg3 *o = MALLOC(ilImg3, 1);
    if(!o) return NULL;
    o->data = MALLOC(uint8_t, 3*w*h);
    if(!o->data){
        FREE(o);
        return NULL;
    }
    o->width = w;
    o->height = h;
    return o;
}
void ilImg3_free(ilImg3 **I){
    if(!I || !*I) return;
    FREE((*I)->data);
    FREE(*I);
}

ilPattern *ilPattern_new(int w, int h){
    if(w < 1 || h < 1) return NULL;
    ilPattern *o = MALLOC(ilPattern, 1);
    if(!o) return NULL;
    o->data = MALLOC(uint8_t, w*h);
    if(!o->data){
        FREE(o);
        return NULL;
    }
    o->width = w;
    o->height = h;
    return o;
}
void ilPattern_free(ilPattern **I){
    if(!I || !*I) return;
    FREE((*I)->data);
    FREE(*I);
}

// make a single-channel (opaque) mask for cross; allocated here!!!
ilPattern *ilPattern_cross(int w, int h){
    int hmid = h/2, wmid = w/2;
    ilPattern *p = ilPattern_new(w, h);
    if(!p) return NULL;
    uint8_t *ptr = &p->data[wmid];
    for(int y = 0; y < h; ++y, ptr += w) *ptr = 255;
    ptr = &p->data[hmid*w];
    for(int x = 0; x < w; ++x, ++ptr) *ptr = 255;
    return p;
}

// complicated cross
ilPattern *ilPattern_xcross(int w, int h){
    int hmid = h/2, wmid = w/2;
    ilPattern *p = ilPattern_new(w, h);
    if(!p) return NULL;
    uint8_t *data = p->data;
    data[hmid*w + wmid] = 255; // point @ center
    if(h < 7 || w < 7) return p;
    int idxy1 = (hmid-3)*w, idxy2 = (hmid+3)*w;
    int idxx1 = wmid-3, idxx2 = wmid+3;
    for(int i = 0; i < wmid - 3; ++i){
        data[idxy1+i] = data[idxy1+w-1-i] = 255;
        data[idxy2+i] = data[idxy2+w-1-i] = 255;
    }
    for(int i = 0; i < hmid - 3; ++i){
        data[idxx1 + i*w] = data[idxx1 + (h-1-i)*w] = 255;
        data[idxx2 + i*w] = data[idxx2 + (h-1-i)*w] = 255;
    }
    return p;
}

#define DRAW_star(type, max) \
OMP_FOR() \
for(int y = 0; y < h; ++y){ \
    double ry2 = (double)(y-h2); \
    ry2 *= ry2; \
    type *data = &((type*)(p->data))[y*w]; \
        for(int x = 0; x < w; ++x, ++data){ \
        double rx = (double)(x-w2); \
        double Intens = max * pow(1. + (rx*rx + ry2)/theta2, -beta); \
        *data = (type) Intens; \
    } \
}

/**
 * @brief ilPattern_star - create pseudo-star Moffat pattern with max ampl. 255 and given FWHM
 * @param w - width
 * @param h - height
 * @param fwhm - FWHM
 * @param beta - `beta` parameter of Moffat
 * @return pattern or NULL if error
 */
ilPattern *ilPattern_star(int w, int h, double fwhm, double beta){
    if(fwhm < 1.) return NULL;
    ilPattern *p = ilPattern_new(w, h);
    if(!p) return NULL;
    int w2 = w/2, h2 = h/2; // center of image
    double hwhm = fwhm / 2., theta2 = hwhm*hwhm;
    DRAW_star(uint8_t, 255.);
    return p;
}

/**
 * @brief ilImage_star - generate subimage with 'star'; max amplitude for float and double == 1.
 * @param type - image type
 * @param w - image width
 * @param h - height
 * @param fwhm - 'star' FWHM
 * @param beta - beta parameter
 * @return
 */
ilImage *ilImage_star(ilimtype_t type, int w, int h, double fwhm, double beta){
    if(fwhm < 1.) return NULL;
    ilImage *p = ilImage_new(w, h, type);
    if(!p) return NULL;
    int w2 = w/2, h2 = h/2;
    double hwhm = fwhm / 2., theta2 = hwhm*hwhm;
    switch(type){
        case IMTYPE_U8:
            DRAW_star(uint8_t, UINT8_MAX);
        break;
        case IMTYPE_U16:
            DRAW_star(uint16_t, UINT16_MAX);
        break;
        case IMTYPE_U32:
            DRAW_star(uint32_t, UINT32_MAX);
        break;
        case IMTYPE_F:
            DRAW_star(float, 1.);
        break;
        case IMTYPE_D:
            DRAW_star(double, 1.);
        break;
        default:
            ERRX("ilImage_star(): wrong image type");
    }
    return p;
}
#undef DRAW_star

/**
 * @brief ilPattern_draw3 - draw pattern @ 3-channel image
 * @param img (io)    - image
 * @param p (i)       - the pattern
 * @param xc, yc      - coordinates of pattern center @ image
 * @param color       - color to draw pattern (when opaque == 255)
 */
void ilImg3_drawpattern(ilImg3 *img, const ilPattern *p, int xc, int yc, const uint8_t color[3]){
    if(!img || !p) return;
    int xul = xc - p->width/2, yul = yc - p->height/2;
    int xdr = xul+p->width-1, ydr = yul+p->height-1;
    int R = img->width, D = img->height; // right and down border coordinates + 1
    if(ydr < 0 || xdr < 0 || xul > R-1 || yul > D-1) return; // box outside of image

    int oxlow, oxhigh, oylow, oyhigh; // output limit coordinates
    int ixlow, iylow; // intput limit coordinates
    if(xul < 0){
        oxlow = 0; ixlow = -xul;
    }else{
        oxlow = xul; ixlow = 0;
    }
    if(yul < 0){
        oylow = 0; iylow = -yul;
    }else{
        oylow = yul; iylow = 0;
    }
    if(xdr < R){
        oxhigh = xdr;
    }else{
        oxhigh = R;
    }
    if(ydr < D){
        oyhigh = ydr;
    }else{
        oyhigh = D;
    }
    OMP_FOR()
    for(int y = oylow; y < oyhigh; ++y){
        uint8_t *in = &p->data[(iylow+y-oylow)*p->width + ixlow]; // opaque component
        uint8_t *out = &img->data[(y*img->width + oxlow)*3]; // 3-colours
        for(int x = oxlow; x < oxhigh; ++x, ++in, out += 3){
            float opaque = ((float)*in)/255.;
            for(int c = 0; c < 3; ++c){
                out[c] = (uint8_t)(color[c] * opaque + out[c]*(1.-opaque));
            }
        }
    }
}

#define ADD_subim(type, max) \
    OMP_FOR() \
    for(int y = oylow; y < oyhigh; ++y){ \
        type *in = &((type*)(p->data))[(iylow+y-oylow)*p->width + ixlow];  \
        type *out = &((type*)(img->data))[y*img->width + oxlow];  \
        for(int x = oxlow; x < oxhigh; ++x, ++in, ++out){ \
            double res = *in * weight + *out; \
            if(max && res > max) res = max; \
            *out = (type)res; \
        } \
    }

/**
 * @brief iladd_subimage - draw subimage over given image (by sum)
 * @param img (io)    - image
 * @param p (i)       - subimage
 * @param xc, yc      - coordinates of pattern center @ image
 * @param weight      - img = img + p*weight
 */
void ilImage_addsub(ilImage *img, const ilImage *p, int xc, int yc, double weight){
    if(!img || !p) return;
    if(img->type != p->type){
        WARNX("iladd_subimage(): types of image and subimage must match");
        return;
    }
    int xul = xc - p->width/2, yul = yc - p->height/2;
    int xdr = xul+p->width-1, ydr = yul+p->height-1;
    int R = img->width, D = img->height; // right and down border coordinates + 1
    if(ydr < 0 || xdr < 0 || xul > R-1 || yul > D-1) return; // box outside of image

    int oxlow, oxhigh, oylow, oyhigh; // output limit coordinates
    int ixlow, iylow; // intput limit coordinates
    if(xul < 0){
        oxlow = 0; ixlow = -xul;
    }else{
        oxlow = xul; ixlow = 0;
    }
    if(yul < 0){
        oylow = 0; iylow = -yul;
    }else{
        oylow = yul; iylow = 0;
    }
    if(xdr < R){
        oxhigh = xdr;
    }else{
        oxhigh = R;
    }
    if(ydr < D){
        oyhigh = ydr;
    }else{
        oyhigh = D;
    }
    switch(img->type){
        case IMTYPE_U8:
            ADD_subim(uint8_t, UINT8_MAX);
        break;
        case IMTYPE_U16:
            ADD_subim(uint16_t, UINT16_MAX);
        break;
        case IMTYPE_U32:
            ADD_subim(uint32_t, UINT32_MAX);
        break;
        case IMTYPE_F:
            ADD_subim(float, 0);
        break;
        case IMTYPE_D:
            ADD_subim(double, 0);
        break;
        default:
            ERRX("iladd_subimage(): wrong image type");
    }
}
#undef ADD_subim

#define PUTP(type) do{((type*)I->data)[I->width*y+x] = *((type*)val);}while(0)
/**
 * @brief ilImage_drawpix - put pixel @(x,y)
 * @param I - image
 * @param x - point coordinates
 * @param y
 * @param val - data value to set (the same type as I->data)
 */
void ilImage_drawpix(ilImage *I, int x, int y, const void *val){
    if(x < 0 || x >= I->width || y < 0 || y >= I->height) return;
    switch(I->type){
        case IMTYPE_U8:
            PUTP(uint8_t);
        break;
        case IMTYPE_U16:
            PUTP(uint16_t);
        break;
        case IMTYPE_U32:
            PUTP(uint32_t);
        break;
        case IMTYPE_F:
            PUTP(float);
        break;
        case IMTYPE_D:
            PUTP(double);
        break;
        default:
            ERRX("ilImage_drawpix(): wrong image type");
    }
}
#undef PUTP

static void plotLineLow(ilImage *I, int x0, int y0, int x1, int y1, const void *val){
    int dx = x1 - x0, dy = y1 - y0, yi = 1;
    if(dy < 0){ yi = -1; dy = -dy; }
    int D = (2 * dy) - dx,  y = y0;
    for(int x = x0; x <= x1; ++x){
        ilImage_drawpix(I, x, y, val);
        if(D > 0){
            y += yi;
            D += 2 * (dy - dx);
        }else{
            D += 2*dy;
        }
    }
}
static void plotLineHigh(ilImage *I, int x0, int y0, int x1, int y1, const void *val){
    int dx = x1 - x0, dy = y1 - y0, xi = 1;
    if(dx < 0){ xi = -1; dx = -dx; }
    int D = (2 * dx) - dy,  x = x0;
    for(int y = y0; y <= y1; ++y){
        ilImage_drawpix(I, x, y, val);
        if(D > 0){
            x += xi;
            D += 2 * (dx - dy);
        }else{
            D += 2*dx;
        }
    }
}
/**
 * @brief ilImage_drawline - Bresenham's line drawing on Image
 * @param I - image
 * @param x0 - first point
 * @param y0
 * @param x1 - last point
 * @param y1
 * @param val - value to put
 */
void ilImage_drawline(ilImage *I, int x0, int y0, int x1, int y1, const void *val){
    if(!I || !I->data) return;
    if(ABS(y1 - y0) < ABS(x1 - x0)){
        if(x0 > x1) plotLineLow(I, x1, y1, x0, y0, val);
        else plotLineLow(I, x0, y0, x1, y1, val);
    }else{
        if(y0 > y1) plotLineHigh(I, x1, y1, x0, y0, val);
        else plotLineHigh(I, x0, y0, x1, y1, val);
    }
}

/**
 * @brief ilImage_drawcircle - Bresenham's circle drawing on Image
 * @param I - image
 * @param x0- circle center
 * @param y0
 * @param R - circle radius
 * @param val - value to put
 */
void ilImage_drawcircle(ilImage *I, int x0, int y0, int R, const void *val){
    int x = R;
    int y = 0;
    int radiusError = 1-x;
    while(x >= y){
        ilImage_drawpix(I, x + x0,   y + y0, val);
        ilImage_drawpix(I, y + x0,   x + y0, val);
        ilImage_drawpix(I, -x + x0,  y + y0, val);
        ilImage_drawpix(I, -y + x0,  x + y0, val);
        ilImage_drawpix(I, -x + x0, -y + y0, val);
        ilImage_drawpix(I, -y + x0, -x + y0, val);
        ilImage_drawpix(I, x + x0,  -y + y0, val);
        ilImage_drawpix(I, y + x0,  -x + y0, val);
        y++;
        if (radiusError < 0){
            radiusError += 2 * y + 1;
        }else{
            x--;
            radiusError += 2 * (y - x) + 1;
        }
    }

}

/**
 * @brief ilImg3_setcolor - set image pixel to given color or its negative (if original color is near to target)
 * @param impixel - pixel to change
 * @param color - desired color
 */
void ilImg3_setcolor(uint8_t impixel[3], const uint8_t color[3]){
    int invert = 0;
    for(int i = 0; i < 3; ++i)
        if(impixel[i] > color[i]){
            if(impixel[i] - color[i] < 127) ++invert;
        }else if(color[i] - impixel[i] < 127) ++invert;
    if(invert == 3) for(int i = 0; i < 3; ++i) impixel[i] = ~color[i];
    else for(int i = 0; i < 3; ++i) impixel[i] = color[i];
}

/**
 * @brief ilImg3_drawpix - draw pixel with `color` or its negative on coloured image
 * @param I - image
 * @param x - point coordinates
 * @param y
 * @param color - desired color
 */
void ilImg3_drawpix(ilImg3 *I, int x, int y, const uint8_t color[3]){
    if(!I || !I->data) return;
    if(x < 0 || x >= I->width) return;
    if(y < 0 || y >= I->height) return;
    ilImg3_setcolor(I->data + 3*(I->width*y+x), color);
}

static void plotLineLow3(ilImg3 *I, int x0, int y0, int x1, int y1, const uint8_t color[3]){
    int dx = x1 - x0, dy = y1 - y0, yi = 1;
    if(dy < 0){ yi = -1; dy = -dy; }
    int D = (2 * dy) - dx,  y = y0;
    for(int x = x0; x <= x1; ++x){
        ilImg3_drawpix(I, x, y, color);
        if(D > 0){
            y += yi;
            D += 2 * (dy - dx);
        }else{
            D += 2*dy;
        }
    }
}

static void plotLineHigh3(ilImg3 *I, int x0, int y0, int x1, int y1, const uint8_t color[3]){
    int dx = x1 - x0, dy = y1 - y0, xi = 1;
    if(dx < 0){ xi = -1; dx = -dx; }
    int D = (2 * dx) - dy,  x = x0;
    for(int y = y0; y <= y1; ++y){
        ilImg3_drawpix(I, x, y, color);
        if(D > 0){
            x += xi;
            D += 2 * (dx - dy);
        }else{
            D += 2*dx;
        }
    }
}

/**
 * @brief ilImg3_drawline - Bresenham's line drawing on Img3
 * @param I - image
 * @param x0 - first point
 * @param y0
 * @param x1 - last point
 * @param y1
 * @param color - drawing color
 */
void ilImg3_drawline(ilImg3 *I, int x0, int y0, int x1, int y1, const uint8_t color[3]){
    if(!I || !I->data) return;
    if(ABS(y1 - y0) < ABS(x1 - x0)){
        if(x0 > x1) plotLineLow3(I, x1, y1, x0, y0, color);
        else plotLineLow3(I, x0, y0, x1, y1, color);
    }else{
        if(y0 > y1) plotLineHigh3(I, x1, y1, x0, y0, color);
        else plotLineHigh3(I, x0, y0, x1, y1, color);
    }
}

