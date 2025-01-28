#!/usr/bin/env bash

if [ -z "$1" ]
then
  echo "No argument supplied";
  exit;
fi

if [ "$1" -gt "1" ] || [ "$1" -lt "0" ]
then
  echo "Invalid Value";
  exit;
fi

echo "$1" | sudo tee /sys/class/backlight/rpi_backlight/bl_power;
