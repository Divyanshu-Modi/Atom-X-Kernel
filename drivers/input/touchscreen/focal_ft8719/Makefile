#------------------------------------------------#
# Makefile for the focaltech touchscreen drivers.#
#------------------------------------------------#

focaltech-objs := focaltech_core.o focaltech_point_report_check.o focaltech_ex_fun.o focaltech_ex_mode.o focaltech_flash.o focaltech_gesture.o focaltech_i2c.o

# Each configuration option enables a list of files.
obj-$(CONFIG_TOUCHSCREEN_FTS) += focaltech.o
obj-$(CONFIG_TOUCHSCREEN_FTS) += focaltech_flash/
obj-$(CONFIG_TOUCHSCREEN_FTS) += focaltech_test/
