# Enhanced MHI ZX Spectrum (ZXEM) emulator for PAX 920

## Quick start guide

1. Install a working arm-gnu-toolchain: `sudo apt-get install gcc-arm-linux-gnueabi g++-arm-linux-gnueabi build-essential python3-pip libssl-dev swig python3-dev gcc python3.10-venv`
2. Download and unzip uploader script: `wget https://git.lsd.cat/g/prolin-xcb-client/archive/master.zip && unzip master.zip -d ../ && rm master.zip`
3. Run `python3 -m venv env; source env/bin/activate; pip3 install -r requirements.txt`
4. Run `sudo make upload` to upload snapshots and rom files into the device.
    - We need super user, because this interfaces with ttyACM0.

Further info
- https://git.lsd.cat/g/pax-pwn