// Copyright (C) 2021 Brad Colbert

#ifndef TYPES_H
#define TYPES_H

typedef unsigned char byte;     // just a little easier to use
typedef unsigned short ushort;  // just a little easier to use

typedef struct
{
    unsigned char v1;
    unsigned char v2;
    unsigned char v3;
    unsigned char gfx;
} FileHeader;

typedef struct
{
    FileHeader header;
    byte* data;
} ImageData;

#endif // TYPES_H