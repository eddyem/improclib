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
#include <string.h>

static int w = 1024, h = 1024, help = 0;
static double fwhm = 3.5, beta = 1.;
static char *outp = "output.png", *inp = NULL;

static ilImage *star = NULL;

static myoption cmdlnopts[] = {
    {"help",    NO_ARGS,    NULL,   '?',    arg_int,    APTR(&help),    "show this help"},
    {"width",   NEED_ARG,   NULL,   'w',    arg_int,    APTR(&w),       "resulting image width (default: 1024)"},
    {"height",  NEED_ARG,   NULL,   'h',    arg_int,    APTR(&h),       "resulting image height (default: 1024)"},
    {"output",  NEED_ARG,   NULL,   'o',    arg_string, APTR(&outp),    "output file name (default: output.png)"},
    {"halfwidth",NEED_ARG,  NULL,   's',    arg_double, APTR(&fwhm),    "FWHM of 'star' images (default: 3.5)"},
    {"beta",    NEED_ARG,   NULL,   'b',    arg_double, APTR(&beta),    "beta Moffat parameter of 'star' images (default: 1)"},
    {"input",   NEED_ARG,   NULL,   'i',    arg_string, APTR(&inp),     "input file with coordinates and amplitudes (comma separated)"},
    end_option
};

static int getpars(const char *argv, int *x, int *y, double *w){
    char *eptr, *start = (char*)argv;
    long l;
    l = strtol(start, &eptr, 0);
    if(eptr == start || *eptr != ',' || l < 0 || l > INT_MAX) return FALSE;
    *x = (int)l;
    start = eptr + 1;
    l = strtol(start, &eptr, 0);
    if(eptr == start || l < 0 || l > INT_MAX) return FALSE;
    *y = (int)l;
    if(*eptr == ','){
        start = eptr + 1;
        double d = strtod(start, &eptr);
        if(eptr == start) return FALSE;
        *w = d;
    }else *w = 1.;
    return TRUE;
}

static void addstar(ilImage *I, const char *str){
    int x, y;
    double w;
    if(!getpars(str, &x, &y, &w)) return;
    printf("Add 'star' at %d,%d (weight=%g)\n", x,y,w);
    iladd_subimage(I, star, x, y, w);
}

static void addfromfile(ilImage *I){
    FILE *f = fopen(inp, "r");
    if(!f){
        WARN("Can't open %s", inp);
        return;
    }
    char *line = NULL;
    size_t n = 0;
    while(getline(&line, &n, f) > 0) addstar(I, line);
    fclose(f);
}

int main(int argc, char **argv){
    initial_setup();
    char *helpstring = "Usage: %s [args] x1,y1[,w1] x2,y2[,w2] ... xn,yn[,w3] - draw 'stars' at coords xi,yi with weight wi (default: 1.)\n\n\tWhere args are:\n";
    change_helpstring(helpstring);
    parseargs(&argc, &argv, cmdlnopts);
    if(help) showhelp(-1, cmdlnopts);
    if(w < 1 || h < 1) ERRX("Wrong image size");
    if(argc == 0 && inp == NULL) ERRX("Point at least one coordinate pair or file name");
    ilImage *I = ilImage_new(w, h, IMTYPE_U16);
    if(!I) ERRX("Can't create image %dx%d pixels", w, h);
    int par = (int)(fwhm*25.);
    star = ilImage_star(IMTYPE_U16, par, par, fwhm, beta);
    if(!star) ERRX("Can't create 'star' pattern");
    for(int i = 0; i < argc; ++i) addstar(I, argv[i]);
    if(inp) addfromfile(I);
    ilImage_free(&star);
    ilImage_putstring(I, "Hello, world!!", -10, 10);
    ilImage_putstring(I, "0", 0, 1016);
    ilImage_putstring(I, "Hello, world.!?\"'\nMore again", 50, 500);
    ilImage_putstring(I, "Hello, world!", 950, 1018);
    for(int _ = 0; _ < 1024; _ += 50){
        char s[6];
        snprintf(s, 6, "%d", _);
        ilImage_putstring(I, s, _, 300);
    }
    uint8_t *bytes = ilImage2u8(I, 1);
    int ret = ilwrite_png(outp, I->width, I->height, 1, bytes);
    ilImage_free(&I);
    FREE(bytes);
    if(!ret) return 4;
    printf("File %s ready\n", outp);
    return 0;
}
