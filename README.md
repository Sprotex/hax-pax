# PAX 920 reverse engineered kit (formerly Enhanced MHI ZXEM emulator)

## Quick start guide

1. Run `./quick-start.sh` to set up the environment.
    - This script will handle the virtual environment setup and execute the necessary commands.
2. Run `source env/bin/activate` to activate the environment created in `quick-start.sh`.
3. Run `sudo make upload` to upload the firmware to the device.
    - Make sure to run it with superuser privileges, as it interfaces with ttyACM0.
4. Update variables at the top of `Makefile` to follow your paths.
    - Intended for users moving to this repository with paths set to different files and folders.
5. Connect to your device (instructions below), run `make <your_project>` (for example `make zxem`), and reload your terminal.

Study the `Makefile` for more information how to run separate projects.

## Connecting your device
The PAX device can be connected using a usb cable, or Wi-Fi connection. Make sure you have followed the [Quick start guide](#quick-start-guide) before continuing.

### USB
It should work out of the box. Only extra step needed is to connect the micro USB cable into the device and your computer.

### Wi-Fi
There are two main steps to ensure Wi-Fi connectivity.

1. Connecting your device to your selected Wi-Fi:
    1. In your PAX device, from diagostic menu, enter 1. System Config. Enter the default password.
    2. On PAX device, press 3. WiFi
    3. Make sure the Wi-Fi network you want to connect to runs on 2.4 GHz.
    4. On PAX device, press 1. Auto Set, then select the SSID of your Wi-Fi network.
    5. Use the device keyboard to enter the selected Wi-Fi network password. After a few seconds, it should display "Connect Success".
    6. Depending on your network, select DHCP or Static.
        - For this guide, we will assume DHCP on your network is enabled, so we continue by pressing 1. DHCP. If it succeeds, you will see the device IP address, netmask, network gateway and DNS server
        - **IMPORTANT**: make sure, you remember or write down the IP address, it will be needed later.
    7. Press red X button until Quit TM is shown on display. Your device is now connected to your selected Wi-Fi!
2. Configuring the upload script to work on Wi-Fi:
    1. Open [../prolin-xcb-client/client.py](../prolin-xcb-client/client.py)
    2. Find line starting with port variable (around line 10)
        - Change the variable to your IP address from step 1.6., followed by port number 5555
        - So the line will look something like this: `port = '192.168.0.1:5555'`

## Known Errors
- Last known working versions are on Ubuntu 20.04
    - Native compiler from Ubuntu 22.04 seems to not work for us. Beware.

## Simple telnet client connection
For ease of use, try the telnet functionality. Run `make telnet`, then connect to the telnet client in your shell of choice. The port of the telnet client is 2323.

Example:
`telnet 192.168.0.1 2323`

## Further info
- https://git.lsd.cat/g/pax-pwn
