#ifndef PIXEL_H
#define PIXEL_H

//Each pixel is 3 bytes
typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} pixel;

typedef struct {
    unsigned char current;
    unsigned char topLeft;
    unsigned char top;
    unsigned char topRight;
    unsigned char left;
    unsigned char right;
    unsigned char bottomLeft;
    unsigned char bottom;
    unsigned char bottomRight;
} surroundingPixels;

#endif
