# Running aitrios-edge-device-core as a systemd service

This document provides instructions for setting up aitrios-edge-device-core to run automatically as a systemd service on Raspberry Pi.

## Prerequisites

- Complete the installation and configuration steps from [How to use for Raspberry Pi](../docs/how_to_use_for_raspberry_pi.md)
- SQLite database created with proper root permissions
- Database configured with MQTT broker settings

**Note**: When you install edge-device-core via `apt install`, the systemd service files are automatically installed, but the service must be manually enabled.

## Setup systemd service

### Enable the service for automatic startup

```bash
# Enable the service for automatic startup
sudo systemctl enable edge-device-core.service
```

### Start the service

```bash
# Start the service
sudo systemctl start edge-device-core.service
```

### Check service status

```bash
sudo systemctl status edge-device-core.service
```

You should see output indicating that the service is active and running:
```
$ sudo systemctl status edge-device-core.service
● edge-device-core.service - Edge Device Core Application
     Loaded: loaded (/lib/systemd/system/edge-device-core.service; enabled; preset: enabled)
     Active: active (running) since ...
```

## Managing the service

### View service logs

```bash
# View recent logs
sudo journalctl -u edge-device-core.service

# Follow logs in real-time
sudo journalctl -u edge-device-core.service -f
```

### Stop the service

```bash
sudo systemctl stop edge-device-core.service
```

### Restart the service

```bash
sudo systemctl restart edge-device-core.service
```

### Disable automatic startup (if needed)

```bash
sudo systemctl disable edge-device-core.service
```

## Troubleshooting

### Check service configuration

Verify that the service file is correctly configured:

```bash
sudo systemctl cat edge-device-core.service
```

### Check environment variables

Verify that the environment file is correctly loaded:

```bash
sudo systemctl show edge-device-core.service --property=Environment
```

### Manual testing

Before starting the systemd service, you can test the configuration manually as root:

```bash
sudo bash -c 'source /etc/default/edge-device-core.env && /usr/bin/edge_device_core'
```

## Service configuration details

The systemd service is configured with the following features:

- **Automatic restart**: The service will restart automatically if it crashes
- **Network dependency**: Waits for network connectivity before starting
- **User context**: Runs as root user for system-level operations
- **Environment file**: Loads configuration from `/etc/default/edge-device-core.env`
- **Data directories**: Uses `/var/lib/edge-device-core` for application data and `/evp_data` for MQTT broker data exchange
- **Logging**: Logs are managed by `journalctl` and can be viewed with `sudo journalctl -u edge-device-core.service`
