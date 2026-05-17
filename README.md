# ESPD - ESP Device Control Daemon

A UBUS daemon for controlling ESP8266/ESP32 devices over USB serial on OpenWRT/RutOS. Bridges UBUS method calls to JSON commands sent over serial.

```
User/CLI  ──UBUS──▶  espd  ──Serial──▶  ESP8266/ESP32
```

## Requirements

- OpenWRT 19.07+ or RutOS 7.x+
- Runtime: `libubus`, `libubox`, `libblobmsg-json`, `libserialport`
- Hardware: ESP device with a CP210x USB-to-serial chip (VID `0x10C4`, PID `0xEA60`)
- ESP must run firmware that speaks the JSON protocol below

## Build & Install

```sh
make
sudo make install
```

Install the hotplug script so devices are detected automatically on plug/unplug:

```sh
cp files/etc/hotplug.d/usb/20-espd /etc/hotplug.d/usb/20-espd
chmod +x /etc/hotplug.d/usb/20-espd
```

To run at boot, enable the included init script:

```sh
/etc/init.d/tuya-espd enable
/etc/init.d/tuya-espd start
```

## Usage

```sh
espd          # foreground (debugging)
espd -D       # daemonize
```

### UBUS API

| Method | Arguments | Description |
|--------|-----------|-------------|
| `devices` | — | List connected ESP devices |
| `on` | `port`, `pin` | Set GPIO pin HIGH |
| `off` | `port`, `pin` | Set GPIO pin LOW |
| `get` | `port`, `pin`, `model`, `sensor` | Read DHT sensor |
| `scan` | — | Rescan for devices |

```sh
ubus call espd devices
ubus call espd on  '{"port":"/dev/ttyUSB0","pin":12}'
ubus call espd off '{"port":"/dev/ttyUSB0","pin":12}'
ubus call espd get '{"port":"/dev/ttyUSB0","pin":4,"model":"dht22","sensor":"dht"}'
```

Valid sensor models: `dht11`, `dht22`, `dht21`, `am2302`. Pin range: 0–20.

## ESP Firmware Protocol

Serial settings: **9600 8N1**, no flow control.

Commands are single-line JSON; the ESP must reply with JSON before the next newline.

```json
{"action":"on","pin":12}
{"action":"off","pin":12}
{"action":"get","pin":4,"model":"DHT22","sensor":"temperature"}
```

Success response: `{"rc":0,"msg":"OK"}`  
Sensor response: `{"temperature":23.5,"humidity":45.2}`  
Error response: `{"rc":-1,"msg":"Pin not found"}`

## Troubleshooting

**Device not detected** — run `lsusb` and confirm the VID/PID is `10c4:ea60`. Trigger a manual rescan with `ubus call espd scan`.

**Serial errors** — verify the ESP baud rate is 9600 and the firmware outputs valid JSON terminated with `\n`.

**Daemon won't start** — check that `ubusd` is running (`ps | grep ubusd`) and that `/var/run/ubus/ubus.sock` exists.

## Files

| Path | Description |
|------|-------------|
| `src/main.c` | Entry point, argument parsing, event loop |
| `src/action.c/h` | UBUS method handlers |
| `src/mqtt_callbacks.c/h` | MQTT event callbacks |
| `src/ubus_invoke.c/h` | UBUS invocation helpers |
| `src/argp_parser.c/h` | CLI argument parsing |
| `src/daemon.c/h` | Daemonization |
| `files/etc/init.d/tuya-espd` | Init script |
| `files/etc/config/tuya_espd` | UCI config |

## License

MIT — Internship project, Teltonika Networks
