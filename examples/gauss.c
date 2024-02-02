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

#include "improclib.h"
#include <usefull_macros.h>
#include <stdio.h>

static int help = 0, w = 1024, h = 1024, Niter = 1000000;
static double xsigma = 10., ysigma = 10., x0 = 512., y0 = 512.;
static char *outp = "output.png";

static myoption cmdlnopts[] = {
    {"help",    NO_ARGS,    NULL,   '?',    arg_int,    APTR(&help),    "show this help"},
    {"width",   NEED_ARG,   NULL,   'w',    arg_int,    APTR(&w),       "resulting image width (default: 1024)"},
    {"height",  NEED_ARG,   NULL,   'h',    arg_int,    APTR(&h),       "resulting image height (default: 1024)"},
    {"output",  NEED_ARG,   NULL,   'o',    arg_string, APTR(&outp),    "output file name (default: output.png)"},
    {"xstd",    NEED_ARG,   NULL,   'X',    arg_double, APTR(&xsigma),  "STD of 'photons' distribution by X (default: 10)"},
    {"ystd",    NEED_ARG,   NULL,   'Y',    arg_double, APTR(&ysigma),  "STD of 'photons' distribution by Y (default: 10)"},
    {"xcenter", NEED_ARG,   NULL,   'x',    arg_double, APTR(&x0),      "X coordinate of 'image' center (default: 512)"},
    {"ycenter", NEED_ARG,   NULL,   'y',    arg_double, APTR(&y0),      "Y coordinate of 'image' center (default: 512)"},
    {"niter",   NEED_ARG,   NULL,   'n',    arg_int,    APTR(&Niter),   "iterations (\"falling photons\") number (default: 1000000)"},
    end_option
};

int main(int argc, char **argv){
    initial_setup();
    parseargs(&argc, &argv, cmdlnopts);
    if(help) showhelp(-1, cmdlnopts);
    if(w < 1 || h < 1) ERRX("Wrong image size");
    if(xsigma < DBL_EPSILON || ysigma < DBL_EPSILON) ERRX("STD should be >0");
    if(Niter < 1) ERRX("Iteration number should be a large positive number");
    il_Image *I = il_Image_new(w, h, IMTYPE_U8);
    if(!I) ERRX("Can't create image %dx%d pixels", w, h);
    int hits = 0;
    for(int i = 0; i < Niter; ++i){
        //int x = (int)ilNormal(x0, sigma), y = (int)ilNormal(y0, sigma);
        double x, y;
        il_NormalPair(&x, &y, x0, y0, xsigma, ysigma);
        if(x < 0 || x >= I->width || y < 0 || y >= I->height) continue;
        uint8_t *pix = I->data + (int)x + ((int)y)*I->width;
        if(*pix < 255) ++*pix;
        ++hits;
    }
    int ret = il_write_png(outp, I->width, I->height, 1, I->data);
    il_Image_free(&I);
    if(!ret) return 1;
    printf("File %s ready; %d hits of %d\n", outp, hits, Niter);
    return 0;
}
