raspberrypi-incubator
=====================

Software for controlling the temperature and humidity of an incubator
using a Raspberry Pi.

This project uses a Raspberry Pi together with an AM2315
temperature/humidity sensor and a MCP4725 DAC connected to its I2C bus
to measure and control the environment in an incubator.

There is a web interface to show a graph of the most recent data
(together with average values and standard deviation) and change the
wanted temperature and humidity levels.

The master branch should always be in a stable state. Development
takes place in the develop branch and all feature/* branches. The
current stable version is 0.4.4.

Feel free to send me a pull request if you have anything to
contribute.

Installation
------------

- Get libmicrohttpd and zlib for your RPi.

- Adjust the values in include/config.h.

- Change the script source in html/root.html if you want to modify the
  javascript and/or host it on your own server.

- Set the CROSSROOT variable in the first line of the Makefile. This
  variable defines where the build process will look for the
  headers. If you have a compiler on your RPi, you can simply use
  /usr/ here.

- Run make (don't forget to set CC if you're using a crosscompiler).

- Copy js/* to your webserver (if you're using your own) and main and
  html/root.html (as 'roottemplate') to your RPi.

- Run main in the same directory where you copied your root.html. It
  should start printing its log data to the standard output.

Todo
----

- The control part is not actually done yet, but will use a 1 ohm
  resistor on the DAC to increase the temperature and most likely an
  ultrasonic fogger over a relay connected to a GPIO pin for the
  humidity control.

- Maybe a webcam with motion detection for when the eggs start
  hatching?

- Right now, the javascript is served over an external page. Maybe
  have the builtin httpd handle that. If you don't have a httpd, you
  may either install one on the Raspberry Pi, or copy the contents of
  js/graph.js into html/root.html.

- src/httpd.c could probably use a bit of refactoring...

- Change user/password through the web interface, and store them.
