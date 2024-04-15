# About

This is an application for the LPS22HB pressure and temperature sensors
driver. This program does the measuring, as well as connects and sends the 
measurements to the Campus IoT OTAA network, while implementing a Watchdog 
and a low-power mode on the board when it is not in use.

Default driver is `lps331ap`. To use the LPS22HB driver, set the `DRIVER` when
building the application:

    DRIVER=lps22hb make -j 4 flash

# Usage

This test application will initialize the pressure sensor with a sampling rate
of 7Hz (25Hz with lps22hb and lps22hh).

After initialization, the sensor reads the pressure and temperature values
every 250ms and prints them to the STDOUT and sends the data over to Campus IoT
Network, where it will be transfered to Grafana for further analysis.

# Testing

To test or modify the code of this application, the files contained in this project 
must be added to [RIOT OS](https://github.com/RIOT-OS/RIOT/tree/master/tests/drivers/lpsxxx) test for the lpsxxx driver, in tests/drivers/lpsxxx
