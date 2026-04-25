#ifndef PIXEL_H
#define PIXEL_H

#include <stdio.h>

//Each pixel is 3 bytes
typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} pixel;

typedef struct {
    float current;
    float topLeft;
    float topMiddle;
    float topRight;
    float left;
    float right;
    float bottomLeft;
    float bottomMiddle;
    float bottomRight;
} surroundingPixels;

#endif
