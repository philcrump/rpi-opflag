# OpFLag

_Work In Progress_

Icinga audio/visual alarm flag for network operations.

This software has been developed with the official Raspberry Pi 7" touchscreen and a Raspberry Pi 4, and Raspberry Pi OS Lite (64-bit) (Bullseye.)

## Dependencies

`sudo apt install build-essential libcurl4-openssl-dev`

## Configuration

Copy _config.ini.template_ to _config.ini_ and customise.

## Compile and Install 

```
make
sudo systemctl enable systemd-time-wait-sync
sudo ./install
```

### Raspberry Pi Modifications for fast & graphically-clean boot

`sudo systemctl disable getty@tty1.service`

#### /boot/config.txt:

* Comment out the line: `dtoverlay=vc4-fkms-v3d`
* Append the following lines:
```
disable_splash=1
boot_delay=0
dtoverlay=pi3-disable-bt
```
* Optional, requires RPi4(?) and UHS-1 SD Card, append the line: `dtoverlay=sdtweak,overclock_50=100`

#### /boot/cmdline.txt

* Remove `console=tty1`
* Append `vt.global_cursor_default=0 logo.nologo`

## Usage

Reboot to apply the boot modifications and let the application start automatically.

### Waiting for Network on boot

The application will wait for network-online target to be reached before starting the main app.

<p float="left">
  <img src="/img/waitfornetwork.png" width="49%" />
</p>


### Reference

```bash
curl -k -s -S -i -u username:password -H 'Accept: application/json' \
 'http://<host>/icingaweb2/monitoring/list/services?service_problem=1&service_notifications_enabled=1&service_acknowledged=0&limit=10&sort=service_last_state_change'
```

## Licensing

Unless otherwise specified at the top of the relevant file, all materials of this project are licensed under the BSD 3-clause License (New BSD License).

Copyright (c) Phil Crump 2022. All rights reserved.
