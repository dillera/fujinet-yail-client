# FujiNet YAIL (Yet Another Image Loader) Client

A cc65-based image loading and viewing application for 8-bit systems that works with the FujiNet hardware for image loading. YAIL allows you to load and view images from network sources directly on your FujiNet equipped computer.

The Atari target is fully implemented. Apple II (`apple2`, `apple2enh`) and Commodore 64 (`c64`) targets build placeholder binaries that link against fujinet-lib but do not display images yet.

## Features

- Load and display images from network sources via FujiNet
- Graphics 8 (320×220 dithered), Graphics 9 (80×220, 16 shades), and VBXE (320×240, 256 colors) display modes
- Console interface for commands and settings
- Image search and AI image generation via the YAIL server (`search`, `generate`)
- Persistent settings storage using FujiNet AppKey functionality
- Support for PBM image format

The client/server wire protocol is documented in the server repository's `PROTOCOL.md`.

## Build Requirements

- cc65 compiler suite
- FujiNet library (version 4.7.4, downloaded automatically into `_cache/`)
- `dir2atr` utility for creating Atari disk images (required for `make disk` on the atari target)
- `cc1541` for C64 disk images; the scripts in `apple-tools/` for Apple II disks

## Building

```shell
# clean all artifacts
make clean

# build all targets (atari, apple2, apple2enh, c64)
make all

# just the Atari target
make TARGETS="atari" all

# generate the Atari "disk" (ATR)
make TARGETS="atari" disk
```

### Important Build Notes

- The Atari target uses a custom linker configuration file (`src/atari/yail.atari-xex.cfg`) that defines a `FRAMEBUFFER` memory area of at least 20KB. The display lists and the 4KB-boundary loading scheme depend on this layout — do not change it casually.
- The fujinet-lib version is set in the top-level `Makefile` (`FUJINET_LIB_VERSION := 4.7.4`).

## Default YAIL Backend Server

The YAIL client connects to a backend server that provides the images. The default server URL is defined in `src/atari/settings.c`:

```c
#define DEFAULT_URL "N:TCP://fujinet.org:5556/"
```

To change the default server:

1. Edit this line with your preferred server address
2. The format must be: `N:TCP://[server-address]:[port]/`
3. Rebuild with `make clean`, `make all`, and `make TARGETS="atari" disk`

The URL can also be changed at runtime in the console (`set server N:TCP://host:5556/`), and is persisted on the FujiNet via AppKeys.

## Output Files

After a successful build:
- Executables are in `build/` (`yail.atari`, `yail.apple2`, `yail.apple2enh`, `yail.c64`)
- `make TARGETS="atari" disk` creates an ATR disk image in `dist/yail.atr`

## Usage

1. Boot the ATR disk image on an Atari with FujiNet hardware attached
2. Press any key to open the console; type `help` for commands
3. Common commands: `search <terms>`, `generate <prompt>`, `video`, `gfx 8|9|20`, `set server <url>`, `set model <name>`, `quit`

## Project Structure

- `src/main.c` — platform-neutral entry point (calls `platform_run()`)
- `src/platform.h` — per-platform entry point API
- `src/atari/` — full Atari implementation (graphics, console, networking, settings)
- `src/apple2/`, `src/c64/` — placeholder platform implementations
- `makefiles/` — modular build system (`build.mk`, `os.mk`, `common.mk`, `custom-<platform>.mk`, `fujinet-lib.mk`)
- `build/` — build output, `dist/` — disk images

## License

See the LICENSE file for details.
