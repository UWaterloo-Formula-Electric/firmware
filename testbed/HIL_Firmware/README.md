| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# HIL_Firmware

The HIL boards use esp32s2 microcontrollers
The extension used in VScode to build and flash the boards is Espressif IDF, note that you can manually install the espressif IDF, however using the extension
is much faster and easier for setup. (when selecting the version select master branch, as of July 2023 that's V5.2)
The hil firmware has been setup such that each board has it's own project and each project must include the common component and the driver component
Check espressif documentation for a better explanantion on the build system.


## How to use extension
To build, flash or change the sdkconfig file you must first open an ESP-IDF terminal in vscode
To open an ESP_IDF terminal go to the status bar and there should be a greater than symbol (">") click on this to open the terminal 
To modify the sdkconfig file for any of the board, open a terminal and type "idf.py menuconfig"
to build cd into your project directory and enter "idf.py build" in your esp_idf terminal
to flash enter "idf.py flash" or "idf.py -p <PORT> flash" to specify the port


## Possible errors when flashing
Error saying the target is esp32 but the board is esp32s2:
    1.open an idf terminal
    2.enter "idf.py set-target esp32s2"
Error flashing the board cause port doesn't exist:
    1.put the board into bootloader mdoe (hold boot then press the reset button)
    2.identify which com port the board is connected to in device manager
    3.flash the board with the flash command above specifying the port
    4.exit bootloader by pressing the reset button
Can't flash board for whatever reason:
    1. open idf terminal
    2. open the menuconfig
    3. go to component config
    4. go to ESP system settings
    5. change the console output from uart to usb cdc
if none of these work message #firmware for help

