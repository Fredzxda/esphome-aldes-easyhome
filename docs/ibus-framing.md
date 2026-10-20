# iBUS receive framing — working assumptions

Based on the previously developed Raspberry Pi Pico W EasyHome decoder:

- UART: 2400 baud, 8 data bits, even parity, one stop bit.
- Pico TX inversion was a hardware-specific compensation, not evidence of an inverted iBUS protocol.
- Expected VMC response prefix: `87 00`.
- Byte index 2 appears to specify total frame length minus one (checksum included).
- Checksum: two's complement of sum of all preceding bytes, so sum of entire frame modulo 256 is zero.
- The prior code named index 3 `StartPattern` and index 26 `EndPattern`, but did not establish or validate fixed values.
- The example starting `33 FF` is not treated as a VMC response until independently confirmed.

Receive-only milestone: parse and validate frames; expose short raw frames as text_sensor and publish all frames in logs. Resynchronize byte-by-byte on invalid candidates. Avoid interpreting checksum-valid data as necessarily semantically correct.

Future: identify long frames, polling cadence, sensor fields, and commands before enabling TX.
