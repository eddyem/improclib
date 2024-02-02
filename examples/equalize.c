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
#include <stdio.h>
#include <usefull_macros.h>

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "Usage: %s filename - open bw image file, equalize histogram, plot two crosses ans save as output.jpg\n", argv[0]);
        return 1;
    }
    il_Image *I = il_Image_read(argv[1]);
    if(!I){
        fprintf(stderr, "Can't read %s\n", argv[1]);
        return 2;
    }
    int w = I->width, h = I->height;
    double t0 = dtime();
    uint8_t *eq = il_equalize8(I, 3, 0.1);
    green("Equalize: %g ms\n", (dtime() - t0)*1e3);
    il_Image_free(&I);
    if(!eq) return 3;
    il_Img3 *I3 = MALLOC(il_Img3, 1);
    I3->data = eq;
    I3->height = h;
    I3->width = w;
    il_Pattern *cross = il_Pattern_xcross(25, 25);
    il_Img3_drawpattern(I3, cross, 30, 30, il_Color_red);
    il_Img3_drawpattern(I3, cross, 150, 50, il_Color_green);
    il_Pattern_free(&cross);
    int ret = il_Img3_jpg("output.jpg", I3, 95);
    il_Img3_free(&I3);
    if(!ret) return 4;
    printf("File 'output.jpg' ready\n");
    return 0;
}
