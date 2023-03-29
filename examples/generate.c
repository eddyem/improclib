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
static char *outp = "output.jpg", *inp = NULL;

static ilPattern *star = NULL, *cross = NULL;

static myoption cmdlnopts[] = {
    {"help",    NO_ARGS,    NULL,   '?',    arg_int,    APTR(&help),    "show this help"},
    {"width",   NEED_ARG,   NULL,   'w',    arg_int,    APTR(&w),       "resulting image width (default: 1024)"},
    {"height",  NEED_ARG,   NULL,   'h',    arg_int,    APTR(&h),       "resulting image height (default: 1024)"},
    {"output",  NEED_ARG,   NULL,   'o',    arg_string, APTR(&outp),    "output file name (default: output.jpg)"},
    {"halfwidth",NEED_ARG,  NULL,   's',    arg_double, APTR(&fwhm),    "FWHM of 'star' images (default: 3.5)"},
    {"beta",    NEED_ARG,   NULL,   'b',    arg_double, APTR(&beta),    "beta Moffat parameter of 'star' images (default: 1)"},
    {"input",   NEED_ARG,   NULL,   'i',    arg_string, APTR(&inp),     "input file with coordinates and amplitudes (comma separated)"},
    end_option
};

static int getpars(const char *argv, int *x, int *y, int *a){
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
        l = strtol(start, &eptr, 0);
        if(eptr == start || l < 0 || l > 255) return FALSE;
        *a = (int)l;
    }else *a = 255;
    return TRUE;
}

static void addstar(ilImg3 *I, const char *str){
    int x, y, a;
    if(!getpars(str, &x, &y, &a)) return;
    printf("Add 'star' at %d,%d (ampl=%d)\n", x,y,a);
    uint8_t c[3] = {a,a,a};
    ilImg3_drawpattern(I, star, x, y, c);
    ilImg3_drawpattern(I, cross, x, y, ilColor_red);
}

static void addfromfile(ilImg3 *I){
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
    char *helpstring = "Usage: %s [args] x1,y1[,amp1] x2,y2[,amp2] ... xn,yn[,amp3] - draw 'stars' at coords xi,yi with amplitude ampi (default: 255)\n\n\tWhere args are:\n";
    change_helpstring(helpstring);
    parseargs(&argc, &argv, cmdlnopts);
    if(help) showhelp(-1, cmdlnopts);
    if(w < 1 || h < 1) ERRX("Wrong image size");
    if(argc == 0 && inp == NULL) ERRX("Point at least one coordinate pair or file name");
    ilImg3 *I = ilImg3_new(w, h);
    if(!I) ERRX("Can't create image %dx%d pixels", w, h);
    int par = (int)(fwhm*25.);
    star = ilPattern_star(par, par, fwhm, beta);
    cross = ilPattern_xcross(25, 25);
    for(int i = 0; i < argc; ++i) addstar(I, argv[i]);
    if(inp) addfromfile(I);
    ilPattern_free(&star);
    uint8_t color[] = {255, 0, 100};
    //uint8_t color[] = {0, 0, 0};
    ilImg3_putstring(I, "Test string", 450, 520, color);
    ilImg3_drawline(I, -10,900, 1600,1050, color);
    int ret = ilImg3_jpg(outp, I, 95);
    //int ret = ilImg3_png(outp, I);
    ilImg3_free(&I);
    if(!ret) return 4;
    printf("File %s ready\n", outp);
    return 0;
}
