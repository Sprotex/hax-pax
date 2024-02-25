# Enhanced MHI ZX Spectrum (ZXEM) emulator for PAX 920

## Quick start guide

1. Run `./quick-start.sh` to set up the environment.
    - This script will handle the virtual environment setup and execute the necessary commands.
2. Run `source env/bin/activate` to activate the environment created in `quick-start.sh`.
3. Run `sudo make upload` to upload the firmware to the device.
    - Make sure to run it with superuser privileges, as it interfaces with ttyACM0.
4. Update variables at the top of `Makefile` to follow your paths.
    - Intended for users moving to this repository with paths set to different files and folders.

## Known Errors
- Last known working versions are on Ubuntu 20.04
    - Native compiler from Ubuntu 22.04 seems to not work for us. Beware.

Further info
- https://git.lsd.cat/g/pax-pwn
