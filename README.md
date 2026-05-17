# Tuya-ESP Daemon

A daemon that connects to the **Tuya IoT cloud** via MQTT and forwards incoming action commands to the `espd` daemon via UBUS, which then controls ESP8266/ESP32 devices over serial.

```
Tuya Cloud ──MQTT──▶  tuya_espd  ──UBUS──▶  espd  ──Serial──▶  ESP device
```

## Requirements

- OpenWRT 19.07+ or RutOS 7.x+
- [Tuya IoT SDK](https://github.com/tuya/tuya-iot-link-sdk-embedded-c) (linked at build time)
- [`espd`](https://github.com/Domantux/tuya-daemon) running on the same device
- Valid Tuya cloud credentials (`device_id`, `product_id`, `device_secret`)

## Build

```sh
make
sudo make install
```

## Configuration

Credentials are stored in UCI at `/etc/config/tuya_espd`:

```
config tuya_espd 'main'
    option enabled       '1'
    option device_id     'your_device_id'
    option product_id    'your_product_id'
    option device_secret 'your_device_secret'
```

Set `enabled` to `1` and fill in your Tuya cloud credentials.

## Usage

Via init script (recommended):

```sh
/etc/init.d/tuya-espd enable
/etc/init.d/tuya-espd start
```

Or manually:

```sh
tuya_espd -d <device_id> -s <device_secret> -p <product_id>   # foreground
tuya_espd -d <device_id> -s <device_secret> -p <product_id> -D  # daemonize
```

The init script reads credentials from UCI and uses `procd` with automatic respawn.

## Supported Actions

These Tuya cloud action identifiers map directly to `espd` UBUS calls:

| Action | Description |
|--------|-------------|
| `devices` | List connected ESP devices |
| `on` | Set GPIO pin HIGH |
| `off` | Set GPIO pin LOW |
| `get` | Read DHT sensor data |

## Files

| Path | Description |
|------|-------------|
| `src/main.c` | Entry point, MQTT loop, reconnection logic |
| `src/mqtt_callbacks.c/h` | Tuya cloud event handlers (connect, disconnect, action) |
| `src/action.c/h` | Parses cloud actions and invokes UBUS calls |
| `src/ubus_invoke.c/h` | UBUS interface to `espd` |
| `src/daemon.c/h` | Daemonization and PID file management |
| `src/argp_parser.c/h` | CLI argument parsing |
| `src/config.h` | Build-time constants (MQTT host, timeouts, action names) |
| `files/etc/init.d/tuya-espd` | Procd init script |
| `files/etc/config/tuya_espd` | UCI config template |

## License

MIT
