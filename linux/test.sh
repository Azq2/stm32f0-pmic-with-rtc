#!/bin/bash
make all
sudo rmmod stm32f0-pmic
sudo insmod ./stm32f0-pmic.ko
