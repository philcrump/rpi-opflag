# OpFlag

Icinga audio/visual alarm flag for network operations.

This software has been developed with the official Raspberry Pi 7" touchscreen and a Raspberry Pi 4, and Raspberry Pi OS Lite (64-bit) Bookworm.

## Typical screen states

### Nominal (no alarms)

<p float="center">
  <img src="/img/screen-nominal.jpg" width="49%" />
</p>

### Connection Failure (between device and icinga)

<p float="center">
  <img src="/img/screen-connectionfailure.jpg" width="49%" />
</p>

### Active alarms

<p float="center">
  <img src="/img/screen-alarms.gif" width="49%" />
</p>

Tapping the touchscreen will 'acknowledge' this state, switching off the flashing and the buzzer (if connected.) This acknowledgement is internal to this device, and is not an Icinga acknowledgement.

Flag colour in this state is in fact red.

## Dependencies

`sudo apt install git build-essential libcurl4-openssl-dev libjson-c-dev liblgpio-dev`

## Configuration

Copy _config.ini.template_ to _config.ini_ and customise.

## Compile and Install 

```
make
sudo ./install
```

Optionally apply the modifications below.

Reboot and let the application start automatically.

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

## Testing

`./test-http-endpoints/no-alarms.py` - Will simulate a nominal situation.

`./test-http-endpoints/active-alarms.py` - Will simulate a single WARNING and a single CRITICAL alarm.

### Testing of Icinga Polling (for development)

```bash
curl -k -s -S -i -u username:password -H 'Accept: application/json' \
 'http://<host>/icingaweb2/monitoring/list/services?service_problem=1&service_notifications_enabled=1&service_acknowledged=0&limit=10&sort=service_last_state_change'
```

### Waiting for Network on boot

The application will wait for network-online target to be reached before starting the main app.

<p float="left">
  <img src="/img/waitfornetwork.png" width="49%" />
</p>


## Licensing

Unless otherwise specified at the top of the relevant file, all materials of this project are licensed under the BSD 3-clause License (New BSD License).

Copyright (c) Phil Crump 2022. All rights reserved.
