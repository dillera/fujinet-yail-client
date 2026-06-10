// Copyright (C) 2021 Brad Colbert
//
// Platform-neutral entry point.  All real work happens in the
// platform_run() implementation selected by the build target.

#include "platform.h"

int main(int argc, char* argv[])
{
    return platform_run(argc, argv);
}
