# 2027-Embedded-Code

Firmware projects for the Kennesaw State University AUV team.

## Projects

| Project | Location | Description |
|---|---|---|
| Arm Controller Unit | [`ArmControllerUnit`](./ArmControllerUnit) | Arm Controller firmware |
| ESC Controller | [`ESCController`](./ESCController) | ESC Controller firmware |
| Environment Monitoring Unit | [`EnvironmentMonitoringUnit`](./EnvironmentMonitoringUnit) | Environment Monitoring Unit firmware |
| Display Controller | [`DisplayController`](./DisplayController) | Display Controller firmware |

The projects use [PlatformIO](https://platformio.org/). Open the relevant project directory in VS Code with the PlatformIO extension, then build or upload using the PlatformIO toolbar. Each project has its own `platformio.ini`; this repository root is not a single PlatformIO project.

For Environment Monitoring Unit setup, hardware, and protocol details, see its [usage guide](./EnvironmentMonitoringUnit/README_USAGE.md) and [`docs/`](./EnvironmentMonitoringUnit/docs).

## I2C Addresses

| # | Address | Device Name |
|---|:---:|---|
| 1 | `0x4A` | Power Safety |
| 2 | `0x4B` | IMU |
| 3 | `0x4C` | ESC Controller |
| 4 | `0x4D` | Display Controller |
| 5 | `0x4E` | Hydrophone Controller |
| 6 | `0x4F` | Torpedo Controller |
| 7 | `0x50` | Arm Controller |

## Environment Monitor

The Environment Monitor consists of an STM32G431KBT6, BNO086, and BME280. On boot, the blue LED flashes four times. It stays on for one second if IMU communication succeeds, then flashes on each I2C interrupt. The I2C register is updated every 10 ms (100 Hz).

### I2C Packet

| Byte | Field | Type | Encoding |
|---|:---:|---|---|
| 0 | index | `u8` | Rolling 0–255 frame counter |
| 1–2 | yaw | `i16 LE` | 1/100° (−18000..18000) |
| 3–4 | pitch | `i16 LE` | 1/100° |
| 5–6 | roll | `i16 LE` | 1/100° |
| 7–8 | accel_x | `i16 LE` | 1/100 m/s² |
| 9–10 | accel_y | `i16 LE` | 1/100 m/s² |
| 11–12 | accel_z | `i16 LE` | 1/100 m/s² |
| 13 | accuracy | `u8` | 0 = unreliable; 3 = high |

## ESC Controller

### I2C Packet

| Byte | Field | Type | Encoding |
|---|:---:|---|---|
| 0 | cmd | `u8` | Framing/validation byte; must equal `0x00` or the packet is dropped |
| 1 | ESC1 | `u8` | 0–255 |
| 2 | ESC2 | `u8` | 0–255 |
| 3 | ESC3 | `u8` | 0–255 |
| 4 | ESC4 | `u8` | 0–255 |
| 5 | ESC5 | `u8` | 0–255 |
| 6 | ESC6 | `u8` | 0–255 |
| 7 | ESC7 | `u8` | 0–255 |
| 8 | ESC8 | `u8` | 0–255 |
## Display Controller Firmware

The Display Controller runs on an STM32G431 and drives an e-paper display. It shows vehicle telemetry including battery voltages, attitude and movement, heading, thruster outputs, environmental readings, kill-switch state, and device presence. Test mode simulates telemetry and can be toggled with the board button.

### I2C Packet

The controller receives a 31-byte telemetry frame. Multi-byte values are little-endian.

| Byte | Field | Type | Encoding |
|---|:---:|---|---|
| 0 | magic | `u8` | `0xA5` frame marker |
| 1 | version | `u8` | Protocol version `2` |
| 2–7 | battery voltages | 3 × `u16 LE` | Millivolts, one value per battery |
| 8 | flags | `u8` | Bit 0: kill switch on |
| 9 | pitch | `i8` | Degrees |
| 10 | roll | `i8` | Degrees |
| 11 | yaw | `i8` | Degrees |
| 12 | surge | `i8` | m/s² |
| 13 | sway | `i8` | m/s² |
| 14 | heave | `i8` | m/s² |
| 15–16 | compass heading | `u16 LE` | Degrees |
| 17–24 | thruster outputs | 8 × `u8` | One value per thruster |
| 25–26 | temperature | `i16 LE` | Centi-degrees Celsius |
| 27–28 | humidity | `u16 LE` | Centi-percent |
| 29 | device presence | `u8` | Bit N indicates presence of device N |
| 30 | CRC-8/ATM | `u8` | Checksum over bytes 0–29; polynomial `0x07`, initial value `0x00` |
