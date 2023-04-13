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

// detect objects on an image

#include "improclib.h"
#include <usefull_macros.h>
#include <stdio.h>
#include <string.h>

static int help = 0, ndilat = 0, neros = 0;
double bg = -1.;
static char *infile = NULL, *outbg = NULL, *outbin = NULL;

static myoption cmdlnopts[] = {
    {"help",    NO_ARGS,    NULL,   'h',    arg_int,    APTR(&help),    "show this help"},
    {"input",   NEED_ARG,   NULL,   'i',    arg_string, APTR(&infile),  "input file name"},
    {"obg",     NEED_ARG,   NULL,   0,      arg_string, APTR(&outbg),   "input minus bg jpeg filename"},
    {"background",NEED_ARG, NULL,   'b',    arg_double, APTR(&bg),      "background level (default: auto)"},
    {"obin",    NEED_ARG,   NULL,   0,      arg_string, APTR(&outbin),  "--obg after binarizing"},
    {"ndilat",  NEED_ARG,   NULL,   'd',    arg_int,    APTR(&ndilat),  "amount of dilations after erosions"},
    {"neros",   NEED_ARG,   NULL,   'e',    arg_int,    APTR(&neros),   "amount of image erosions"},
    end_option
};

int main(int argc, char **argv){
    initial_setup();
    parseargs(&argc, &argv, cmdlnopts);
    if(help) showhelp(-1, cmdlnopts);
    if(!infile) ERRX("Point name of input file");
    ilImage *I = ilImage_read(infile);
    if(!I) ERR("Can't read %s", infile);
    if(bg < 0. && !ilImage_background(I, &bg)) ERRX("Can't calculate background");
    uint8_t ibg = (int)(bg + 0.5);
    printf("Background level: %d\n", ibg);
    int w = I->width, h = I->height, wh = w*h;
    ilImage *Ibg = ilImage_sim(I);
    memcpy(Ibg->data, I->data, wh);
    uint8_t *idata = (uint8_t*) Ibg->data;
    for(int i = 0; i < wh; ++i) idata[i] = (idata[i] > ibg) ? idata[i] - ibg : 0;
    if(outbg) ilwrite_jpg(outbg, Ibg->width, Ibg->height, 1, idata, 95);
    double t0 = dtime();
    uint8_t *Ibin = ilImage2bin(I, bg);
    if(!Ibin) ERRX("Can't binarize image");
    green("Binarization: %gms\n", 1e3*(dtime()-t0));
    if(neros > 0){
        t0 = dtime();
        uint8_t *eros = ilerosionN(Ibin, w, h, neros);
        FREE(Ibin);
        Ibin = eros;
        green("%d erosions: %gms\n", neros, 1e3*(dtime()-t0));
    }
    if(ndilat > 0){
        t0 = dtime();
        uint8_t *dilat = ildilationN(Ibin, w, h, ndilat);
        FREE(Ibin);
        Ibin = dilat;
        green("%d dilations: %gms\n", ndilat, 1e3*(dtime()-t0));
    }
    if(outbin){
        ilImage *tmp = ilbin2Image(Ibin, w, h);
        ilwrite_jpg(outbin, tmp->width, tmp->height, 1, (uint8_t*)tmp->data, 95);
        ilImage_free(&tmp);
    }
    ilConnComps *comps;
    t0 = dtime();
    size_t *labels = ilCClabel4(Ibin, w, h, &comps);
    green("Labeling: %gms\n", 1e3*(dtime()-t0));
    if(labels && comps->Nobj > 1){
        printf("Detected %zd components\n", comps->Nobj-1);
        ilBox *box = comps->boxes + 1;
        for(size_t i = 1; i < comps->Nobj; ++i, ++box){
            printf("\t%4zd: s=%d, LU=(%d, %d), RD=(%d, %d)\n", i, box->area, box->xmin, box->ymin, box->xmax, box->ymax);
        }
        FREE(labels);
        FREE(comps);
    }
    ;
    return 0;
}
