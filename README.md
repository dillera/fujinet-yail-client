# FujiNet YAIL (Yet Another Image Loader) Client

A cc65-based image loading and viewing application for 8-bit systems that works with the FujiNet hardware for image loading. YAIL allows you to load and view images from network sources directly on your FujiNet equipped computer.

## Features

- Load and display images from network sources via FujiNet
- Support for various graphics modes
- Console interface for commands and settings
- Persistent settings storage using FujiNet AppKey functionality
- Support for PBM image format

## Build Requirements

- cc65 compiler suite
- FujiNet library (version 4.7.4 recommended)
- `dir2atr` utility for creating Atari disk images (required for `make disk`)

## Building

To build the application, ensure you have the correct compiler/linker for your platform (e.g. cc65), and
make on your path, then simply run make.

```shell
# to clean all artifacts
make clean

# to generate the application for all targets
make all

# to generate a "disk" (ATR)
make disk
```

By default, the build will target multiple platforms (atari, apple2enh, apple2, c64), but you can specify just the Atari target:

```shell
# just the Atari target
make TARGETS="atari" all
```

### Important Build Notes

- The project uses a custom linker configuration file (`src/atari/yail.atari-xex.cfg`) that defines a `FRAMEBUFFER` memory area of at least 20KB
- FujiNet library version 4.7.4 is specified in the Makefile, but version 4.5.1 has been confirmed to work well

## Default YAIL Backend Server

The YAIL client connects to a backend server that provides the images to be displayed. The default server URL is defined in `src/atari/settings.c` on line 16:

```c
#define DEFAULT_URL "N:TCP://fisheye.diller.org:5556/"
```

### Memory Layout Note (Atari)
The Atari implementation requires careful memory management due to the ANTIC chip's 4K boundary limitations. The framebuffer is split into blocks to avoid crossing these boundaries. The `framebuffer` array in `src/atari/welcome_splash.h` includes padding to accommodate the gaps introduced by this splitting. Ensure any modifications to the display list or image loading logic respect these 4K boundaries to prevent display corruption and crashes.

To change the default server:

1. Edit this line with your preferred server address
2. The format must be: `N:TCP://[server-address]:[port]/`
3. Rebuild the application with `make clean` followed by `make all` and `make disk`

The user can also change this URL at runtime through the console interface, but modifying the default ensures the client connects to your server automatically on first run.

## Output Files

After a successful build:
- The executable will be in `build/yail.atari`
- Running `make disk` will create an ATR disk image in `dist/yail.atr`

## Usage

1. Boot the ATR disk image in an Atari computer with FujiNet hardware attached
2. The application will start and display the console interface
3. Use the interface to specify image URLs and view images

## Project Structure

- `src/` - Main source files
- `src/atari/` - Atari-specific implementation files
- `makefiles/` - Build system files
- `build/` - Build output directory
- `dist/` - Distribution files (ATR disk images)

## Makefile System

The build uses a modular Makefile system:
- `Makefile` - Main entry point
- `makefiles/build.mk` - Core build logic
- `makefiles/custom-atari.mk` - Atari-specific settings
- `makefiles/common.mk` - Common settings across platforms
- `makefiles/fujinet-lib.mk` - FujiNet library integration

## License

See the LICENSE file for details.
