# Arrietty controller GPIO diagnostics

This temporary firmware reads the documented button and joystick-switch GPIOs
plus GPIO 16, 17, 27, and 33 as nearby safe candidates. All tested pins remain
inputs with internal pull-ups. Wi-Fi and Bluetooth are never started.

Diagnostic packet format:

```text
D1,sequence,low_mask,j1_gpio35,j1_gpio34,j2_gpio32,j2_gpio25
```

`low_mask` bit order:

```text
bit:   0  1  2  3  4  5  6  7   8  9 10 11
GPIO: 18 19 21 22 23 26 13 14  16 17 27 33
```

This sketch is temporary. Restore the production firmware from
`Hardware/ArriettyController/ArriettyController.ino` after diagnosis.
