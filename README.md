# 2027-Embedded-Code

Firmware projects for the Kennesaw State University AUV team.

## Projects

| Project | Location | Description |
|---|---|---|
| Arm Controller Unit | [`ArmControllerUnit/ArmControllerUnit`](./ArmControllerUnit/ArmControllerUnit) | Arm Controller firmware |
| ESC Controller | [`ESCController`](./ESCController) | ESC Controller firmware |
| Environment Monitoring Unit | [`EnvironmentMonitoringUnit`](./EnvironmentMonitoringUnit) | Environment Monitoring Unit firmware |
| Display Controller | [`DisplayController`](./DisplayController) | Display Controller firmware |

The controller projects use [PlatformIO](https://platformio.org/). Open the relevant project directory in VS Code with the PlatformIO extension, then build or upload using the PlatformIO toolbar. Each project has its own `platformio.ini`; this repository root is not a single PlatformIO project.

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
