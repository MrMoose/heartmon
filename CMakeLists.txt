set(CMAKE_TOOLCHAIN_FILE ../cmake/ArduinoToolchain.cmake) # Arduino Toolchain

cmake_minimum_required(VERSION 2.8)
#====================================================================#
#  Setup Project                                                     #
#====================================================================#
project(HeartmonTest C CXX)

print_board_list()
print_programmer_list()


# This configuration file outlines some of the ways Arduino CMake
# can be used. For a detailed explenation of all the options please
# reade README.rst.

set(ARDUINO_DEFAULT_BOARD flora) # Default Board ID, when not specified
set(ARDUINO_DEFAULT_PORT /dev/ttyACM0) # Default Port, when not specified

generate_arduino_library(Adafruit_NeoPixel
	SRCS
		../Adafruit_NeoPixel/Adafruit_NeoPixel.cpp
	HDRS
		../Adafruit_NeoPixel/Adafruit_NeoPixel.h
	BOARD
		flora
)

include_directories(${HeartmonTest_SOURCE_DIR})
include_directories(AFTER "../Adafruit_NeoPixel/")

generate_arduino_firmware(heartmon
	SKETCH
		Heartmon.ino
	HDRS
		Macros.h
	LIBS
		Adafruit_NeoPixel
	BOARD
		flora
)

