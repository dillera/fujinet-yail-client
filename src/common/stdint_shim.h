#ifndef STDINT_SHIM_H
#define STDINT_SHIM_H

// Force define the guard to prevent system stdint.h from doing anything if it exists
#define _STDINT_H
#define _STDINT

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned int uint16_t;
typedef signed int int16_t;
typedef unsigned long uint32_t;
typedef signed long int32_t;

#endif // STDINT_SHIM_H
