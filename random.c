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
#include "openmp.h"
#include <math.h>
#include <usefull_macros.h>

// Box&Muller method for gaussian RNG (mean=0, std=1)
double il_NormalBase(){
    double U = drand48(), V = drand48();
    double S = sqrt(-2*log(U));
    double X = S * cos(2*M_PI*V);
    double Y = S * sin(2*M_PI*V);
    return X * Y;
}

double il_Normal(double mean, double std){
    return il_NormalBase() * std + mean;
}

/**
 * @brief il_NormalPair - gaussian distributed pair of coordinates
 * @param x,y - coordinates
 * @param xmean, ymean - mean of coordinated
 * @param xstd, ystd - sigma
 */
void il_NormalPair(double *x, double *y, double xmean, double ymean, double xstd, double ystd){
    if(!x || !y) return;
    double U = drand48(), V = drand48();
    double S = sqrt(-2*log(U));
    *x = xmean + xstd * S * cos(2*M_PI*V);
    *y = ymean + ystd * S * sin(2*M_PI*V);
}

static double step = 500., es = -1.;
/**
 * @brief il_PoissonSetStep - change step value for il_Poisson algo
 * @param s - new step (>1)
 * @return TRUE if OK
 */
int il_PoissonSetStep(double s){
    if(s < 1.){
        WARNX("ilPoissonSetStep(): step should be > 1.");
        return FALSE;
    }
    step = s;
    es = exp(step);
    return TRUE;
}

/**
 * @brief il_Poisson - integer number with Poisson distribution
 * @param lambda - mean (and dispersion) of distribution
 * @return number
 */
int il_Poisson(double lambda){
    if(es < 0.) es = exp(step);
    double L = lambda, p = 1.;
    int k = 0;
    do{
        ++k;
        p *= drand48();
        while(p < 1. && L > 0.){
            if(L > step){
                p *= es;
                L -= step;
            }else{
                p *= exp(L);
                L = 0;
            }
        }
    }while(p > 1.);
    return k-1;
}

#define ADDPU(type, max) \
    type *d = (type*)I->data; \
    int wh = I->width * I->height; \
    for(int i = 0; i < wh; ++i, ++d){ \
        type res = *d + il_Poisson(lambda); \
        *d = (res >= *d) ? res : max; \
    }
#define ADDPF(type) \
    type *d = (type*)I->data; \
    int wh = I->width * I->height; \
    for(int i = 0; i < wh; ++i, ++d){ \
        *d += il_Poisson(lambda); \
    }
static void add8p(il_Image *I, double lambda){
    ADDPU(uint8_t, 0xff);
}
static void add16p(il_Image *I, double lambda){
    ADDPU(uint16_t, 0xffff);
}
static void add32p(il_Image *I, double lambda){
    ADDPU(uint32_t, 0xffffffff);
}
static void addfp(il_Image *I, double lambda){
    ADDPF(float);
}
static void adddp(il_Image *I, double lambda){
    ADDPF(double);
}
/**
 * @brief il_Image_addPoisson - add poisson noice to each image pixel
 * @param I (inout) - image
 * @param lambda - lambda of noice
 */
void il_Image_addPoisson(il_Image *I, double lambda){
    switch(I->type){
        case IMTYPE_U8:
            return add8p(I, lambda);
        break;
        case IMTYPE_U16:
            return add16p(I, lambda);
        break;
        case IMTYPE_U32:
            return add32p(I, lambda);
        break;
        case IMTYPE_F:
            return addfp(I, lambda);
        break;
        case IMTYPE_D:
            return adddp(I, lambda);
        break;
        default:
            ERRX("ilImage_addPoisson(): invalid data type");
    }
}

// the same as il_Image_addPoisson but for coloured image (add same noice to all three pixel colour components)
void il_Img3_addPoisson(il_Img3 *I, double lambda){
    int wh = I->width * I->height * 3;
    uint8_t *id = (uint8_t*)I->data;
    //OMP_FOR() - only will be more slowly
    for(int i = 0; i < wh; i += 3){
        uint8_t n = il_Poisson(lambda), *d = &id[i];
        for(int j = 0; j < 3; ++j){
            uint8_t newval = d[j] + n;
            d[j] = (newval >= d[j]) ? newval : 255;
        }
    }
}
