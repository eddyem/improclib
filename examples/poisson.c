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

static int help = 0, w = 1024, h = 1024;
static double lambda = 15.;
static char *outp = "output.png";

static myoption cmdlnopts[] = {
    {"help",    NO_ARGS,    NULL,   '?',    arg_int,    APTR(&help),    "show this help"},
    {"width",   NEED_ARG,   NULL,   'w',    arg_int,    APTR(&w),       "resulting image width (default: 1024)"},
    {"height",  NEED_ARG,   NULL,   'h',    arg_int,    APTR(&h),       "resulting image height (default: 1024)"},
    {"output",  NEED_ARG,   NULL,   'o',    arg_string, APTR(&outp),    "output file name (default: output.png)"},
    {"lambda",  NEED_ARG,   NULL,   'l',    arg_double, APTR(&lambda),  "mean (and dispersion) of distribution (default: 15.)"},
    end_option
};

int main(int argc, char **argv){
    initial_setup();
    parseargs(&argc, &argv, cmdlnopts);
    if(help) showhelp(-1, cmdlnopts);
    if(w < 1 || h < 1) ERRX("Wrong image size");
    if(lambda < 1.) ERRX("LAMBDA should be >=1");
    il_Image *I = il_Image_new(w, h, IMTYPE_U8);
    if(!I) ERRX("Can't create image %dx%d pixels", w, h);
    int npix = I->height * I->width;
    uint8_t *d = I->data;
    for(int i = 0; i < npix; ++i, ++d){
        int ampl = il_Poisson(lambda);
        *d = ampl < 255 ? ampl : 255;
    }
    int ret = il_write_png(outp, I->width, I->height, 1, I->data);
    il_Image_free(&I);
    if(!ret) return 1;
    printf("File %s ready\n", outp);
    return 0;
}
