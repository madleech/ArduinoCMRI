# Test on Real Hardware

This is a tiny sketch to test the current library code on a real device. It
emulates a default SMINI node against a JMRI host, mirroring output bit 0 on the
onboard LED and reporting pin 12 (with pull-up) as input bit 0.

Build + upload + monitor:
```
$ pio run -d extras/hardware_test -e uno -t upload
$ pio device monitor -b 9600
```

Meaningfully exercising the protocol needs a JMRI host polling the node over a
C/MRI serial connection (9600 baud, SERIAL_8N2). Without one, this still serves
as a compile/upload/boot smoke test against the local library source.
```
$ pio run -d extras/hardware_test -e uno
...
SUCCESS
```
