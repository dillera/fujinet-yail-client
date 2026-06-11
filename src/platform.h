// Copyright (C) 2021 Brad Colbert
//
// Per-platform entry point.  src/main.c calls platform_run(); each
// platform directory (src/atari, src/apple2, src/c64) provides an
// implementation.  Fully implemented on Atari; stubs elsewhere.

#ifndef PLATFORM_H
#define PLATFORM_H

int platform_run(int argc, char* argv[]);

#endif // PLATFORM_H
