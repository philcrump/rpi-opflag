#!/usr/bin/env bash

if [ -z "$1" ]
then
  echo "No argument supplied";
  exit;
fi

if [ "$1" -gt "180" ] || [ "$1" -lt "30" ]
then
  echo "Invalid Value";
  exit;
fi

echo "$1" | sudo tee /sys/class/backlight/rpi_backlight/brightness;
