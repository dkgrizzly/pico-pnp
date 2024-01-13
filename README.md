Simple single purpose addons for OpenPnP based on the RP2040 and some breakout boards.

I am building a Pick & Place machine based loosely on parts from Opulo's LumenPnP line.
Instead of using their custom marlin board, I've built my machine around a Duet3D Duet 2 Ethernet board.  Since this board doesn't have vacuum sensors or enough outputs to ideally run the pumps and valves, I've cobbled together some breakout boards I had on hand.
This also meant I didn't have the WS2812 based LED rings for the cameras, so I designed my own using the same board outline but APA102 DotStar white addressable LEDs instead.
Feel free to use these addons as a jumping off point for your own OpenPnP build.


# Pico-Vac
Addon controller to talk to two Honeywell MPRLS0025PA00001A breakout boards https://www.sparkfun.com/products/16476, and the Quad Relay board https://www.sparkfun.com/products/16566 from sparkfun.
This could be a starting place for making your own pnumatic control board for your OpenPnP build.
Supports the following commands:

| Command               |                                      |
| --------------------- | ------------------------------------ |
| M42 P(relay) S(value) | Turn (relay) on (1) or off (0)       |
| M112                  | Emergency Stop, turns all relays off |
| M115                  | Firmware Inquiry                     |
| M308 S(sensor)        | Request (sensor) reading             |

Pin Usage:
| Pin | Function | Purpose                      |
|:---:| -------- | ---------------------------- |
|GP0  | UART TX  | Serial GCode Interface       |
|GP1  | UART RX  |                              |
|GP4  | I2C0 SDA | Pressure Sensor 0 and Relays |
|GP5  | I2C0 SCL |                              |
|GP6  | I2C1 SDA | Pressure Sensor 1            |
|GP7  | I2C1 SCL |                              |

# Pico-Ring
Addon controller for APA102 DotStar LEDs.
Supports the following commands:

| Command                  |                                                                                      |
| ------------------------ | ------------------------------------------------------------------------------------ |
| M112                     | Emergency Stop, turns all LEDs off                                                   |
| M115                     | Firmware Inquiry                                                                     |
| M150 P(value) Q(segment) | Set ring brightness (0.0-1.0 or 0-765), optionally set a bitmask of LEDs to turn on. |

Pin Usage:
| Pin | Function | Purpose                      |
|:---:| -------- | ---------------------------- |
|GP0  | UART TX  | Serial GCode Interface       |
|GP1  | UART RX  |                              |
|GP2  | SPI0 CLK | LED Data Interface           |
|GP3  | SPI0 TX  |                              |

# Pico-Combo
Combines LED Ring and Pnumatics on one MCU.

| Command                  |                                                                                      |
| ------------------------ | ------------------------------------------------------------------------------------ |
| M42 P(relay) S(value)    | Turn (relay) on (1) or off (0)                                                       |
| M112                     | Emergency Stop, turns all relays off                                                 |
| M115                     | Firmware Inquiry                                                                     |
| M150 P(value) Q(segment) | Set ring brightness (0.0-1.0 or 0-765), optionally set a bitmask of LEDs to turn on. |
| M308 S(sensor)           | Request (sensor) reading                                                             |

Pin Usage:
| Pin | Function | Purpose                      |
|:---:| -------- | ---------------------------- |
|GP0  | UART TX  | Serial GCode Interface       |
|GP1  | UART RX  |                              |
|GP2  | SPI0 CLK | LED Data Interface           |
|GP3  | SPI0 TX  |                              |
|GP4  | I2C0 SDA | Pressure Sensor 0 and Relays |
|GP5  | I2C0 SCL |                              |
|GP6  | I2C1 SDA | Pressure Sensor 1            |
|GP7  | I2C1 SCL |                              |
