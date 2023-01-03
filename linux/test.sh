#!/bin/bash
echo 0x34 > /sys/bus/i2c/devices/i2c-1/delete_device
sudo rmmod stm32f0-pmic
make all && sudo insmod ./stm32f0-pmic.ko && echo stm32f0-pmic 0x34 > /sys/bus/i2c/devices/i2c-1/new_device
