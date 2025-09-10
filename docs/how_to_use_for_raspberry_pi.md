# How to use aitrios-edge-device-core for Raspberry Pi

## Required Hardware
- Raspberry Pi 4 or higher (**recommended**), Raspberry Pi Zero 2 W
- microSD card (minimum 8GB, recommended 16GB or more)
- Power adapter (5V 2.5A or higher)
- Raspberry Pi AI camera module (IMX500)

## Prerequisites
- Raspberry Pi 4 or higher (**recommended**), Raspberry Pi Zero 2 W with Raspberry Pi OS (64-bit, Debian Bookworm)
    - Please make sure your username belongs to the `video` group to access the camera module.
    - You can check this with the `groups $(whoami)` command.
- Established internet connection with a wired connection

## Installation

### Install imx500 firmware

The IMX500 firmware is required for the Raspberry Pi AI camera module to work properly. It is not included in the edge-device-core package. You need to install it separately.
Please follow the instructions in the [official Raspberry Pi documentation](https://www.raspberrypi.com/documentation/accessories/ai-camera.html#install-the-imx500-firmware) to install the IMX500 firmware.

You can install it using the following commands:

```bash
sudo apt update && sudo apt full-upgrade
sudo apt install -y imx500-all
sudo reboot
```

### Remove previous edge-device-core and senscord-edc-rpi (optional)

```bash
# Remove previous edge-device-core
sudo apt purge -y edge-device-core
sudo apt clean
sudo rm -rf /usr/bin/edge_device_core

# Remove previous senscord-edc-rpi
sudo apt purge -y senscord-edc-rpi
sudo apt clean
sudo rm -rf /opt/senscord
```

### Install edge-device-core and senscord-edc-rpi

Download the required packages from their respective release pages:

1. **edge-device-core package**
Download the latest `edge-device-core-X.X.X_arm64.deb` from the [edge-device-core releases page](https://github.com/aitrios/aitrios-edge-device-core/releases).

> **Note:**
> If you prefer to build the package from source, please see the instructions in [How to build for Raspberry Pi](./how_to_build_for_raspberry_pi.md).

2. **senscord package**<br>
Download the latest `senscord-edc-rpi-X.X.X_arm64.deb` from the [edge-device-core-sensor releases page](https://github.com/aitrios/aitrios-edge-device-sensor/releases)

Place both downloaded files in your working directory.

Then, run the following commands to install the packages:

```bash
# Install latest version of senscord-edc-rpi
sudo apt install -y ./senscord-edc-rpi*_arm64.deb

# Install latest version of edge-device-core
sudo apt install -y ./edge-device-core*_arm64.deb
```

3. **parameter storage manager package**<br>
The "libparameter_storage_manager.so" file will be available at builddir/libparameter_storage_manager.so.
Place the "libparameter_storage_manager.so" at "/usr/local/lib/".
```bash
sudo cp builddir/libparameter_storage_manager.so /usr/local/lib
sudo ldconfig
```

### Create sqlite3 database

The edge-device-core package requires a SQLite database to store configuration. The package installation automatically creates the database at `/var/lib/edge-device-core/db.sqlite3`.

```bash
export EDGE_DEVICE_CORE_DB_PATH=/var/lib/edge-device-core/db.sqlite3
```

**Note**:
- The SQLite database and required directories are automatically created during package installation with default local MQTT broker settings. You only need to install the sqlite3 packages if they are not already present.
- The required directories (`/var/lib/edge-device-core`, `/evp_data` and `/etc/evp`) are automatically created during package installation. The `/evp_data` directory is used for data exchange between edge-device-core and the MQTT broker.

## Configuration

### Set parameters for edge-device-core

**Preparation:**
Choose your configuration method:
- **Option A: AITRIOS Online Console** - Requires project credentials and device certificates
- **Option B: Local Console (Local MQTT Broker)** - without cloud connectivity

#### Option A: Using AITRIOS Online Console

AITRIOS Online Console is a cloud-based device management platform that allows you to manage your edge devices remotely through MQTT over TLS.

**Required files (prepare before running the commands below):**
- Project ID from your AITRIOS device management portal
- Console endpoint URL provided by AITRIOS
- Device certificates (`*_cert.pem` and `*_key.pem` files)

> **Note:**
> Please refer to the [AITRIOS Portal](https://www.aitrios.sony-semicon.com/) to obtain device certificates.

**Installation and configuration:**
```bash
# 1. Remove default certificate files
rm -f /etc/evp/00000000-0000-0000-0000-000000000000_cert.pem
rm -f /etc/evp/00000000-0000-0000-0000-000000000000_key.pem

# 2. Please replace *_cert.pem and *_key.pem with your actual certificate and key file names
sudo cp *_cert.pem /etc/evp/ # Device certificate file
sudo cp *_key.pem /etc/evp/  # Device private key file

# 3. Configure database parameters (replace with your actual values)
PROJECT_ID="your_actual_project_id"           # Example: "1234567890abcdefghij"
CONSOLE_URL="your_console_endpoint_url"       # Example: "mqtt.console.aitrios.com"

# 4. Set MQTT and platform parameters
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataProjectID --value "${PROJECT_ID}" --db-file ${EDGE_DEVICE_CORE_DB_PATH}
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataEvpHubURL --value "${CONSOLE_URL}" --db-file ${EDGE_DEVICE_CORE_DB_PATH}
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataEvpHubPort --value 8883 --db-file ${EDGE_DEVICE_CORE_DB_PATH}
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataEvpIotPlatform --value TB --db-file ${EDGE_DEVICE_CORE_DB_PATH}
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataEvpTls --value 0 --db-file "${EDGE_DEVICE_CORE_DB_PATH}"

# 5. Set root certificate
ROOT_CA_PATH="/etc/ssl/certs/ca-certificates.crt"
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataPkiRootCerts --value-file "${ROOT_CA_PATH}" --db-file ${EDGE_DEVICE_CORE_DB_PATH}
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataPkiRootCertsHash --value "$(sha256sum "${ROOT_CA_PATH}" | cut -d' ' -f1)" --db-file ${EDGE_DEVICE_CORE_DB_PATH}
```

#### Option B: Using Local Console (Local MQTT Broker)

This option allows you to test edge-device-core without cloud connectivity and without using certificates. You can set the following parameters in the SQLite database to use a local MQTT broker.

**Note**: The package installation automatically configures the database with local MQTT broker settings. You may skip this step if the default configuration is suitable for your environment. Please make sure you have a local MQTT broker (e.g., Mosquitto) installed and running on your Raspberry Pi or local network.

```bash
# Configure for local MQTT broker
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataEvpHubURL --value 127.0.0.1 --db-file "${EDGE_DEVICE_CORE_DB_PATH}" # Your local MQTT broker address
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataEvpHubPort --value 1883 --db-file "${EDGE_DEVICE_CORE_DB_PATH}" # Your local MQTT broker port
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataEvpIotPlatform --value TB --db-file "${EDGE_DEVICE_CORE_DB_PATH}"
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataEvpTls --value 1 --db-file "${EDGE_DEVICE_CORE_DB_PATH}" # Non-TLS mode

# Set root certificate
ROOT_CA_PATH="/etc/ssl/certs/ca-certificates.crt"
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataPkiRootCerts --value-file "${ROOT_CA_PATH}" --db-file ${EDGE_DEVICE_CORE_DB_PATH}
sudo python3 /var/lib/edge-device-core/update_psm_db.py --key PlStorageDataPkiRootCertsHash --value "$(sha256sum "${ROOT_CA_PATH}" | cut -d' ' -f1)" --db-file ${EDGE_DEVICE_CORE_DB_PATH}
```

### Verify configuration (optional)
You can verify your configuration has been applied correctly:

```bash
# Show current database settings
sudo python3 /var/lib/edge-device-core/update_psm_db.py --show-db --db-file "${EDGE_DEVICE_CORE_DB_PATH}"

# Show current database settings with keys
sudo python3 /var/lib/edge-device-core/update_psm_db.py --show-keys
```

The keys are defined in the `psm_data` table of the SQLite database. You can use these keys to set parameters for the edge-device-core.

You can specify keys in any pattern:
- `PlStorageDataEvpHubURL`
- EvpHubURL (omit `PlStorageData`)
- 30 (enum of `PlStorageDataEvpHubURL`)

## Running
### Run edge-device-core

You can run the edge-device-core application in two ways:

#### Option A: Run as systemd service (recommended)
See the [systemd setup manual](../systemd/README.md) for details on how to set up edge-device-core as a systemd service.

#### Option B: Run in terminal
```bash
export EDGE_DEVICE_CORE_DB_PATH=/var/lib/edge-device-core/db.sqlite3
cd /opt/senscord/share/senscord
sudo --preserve-env=EDGE_DEVICE_CORE_DB_PATH ./setup_env.sh edge_device_core
```

**Expected output:**

When edge-device-core starts successfully, you should see log messages like:

```
system_app_main.c ... [INF] Host[127.0.0.1]     # Your configured MQTT host
system_app_main.c ... [INF] Port[1883]          # Your configured MQTT port
system_app_main.c ... [INF] ProjectId:          # Your project ID (if configured)
system_app_main.c ... [INF] RegiToken:          # Registration token
system_app_main.c ... [INF] Ether connected.    # Network connection established
system_app_main.c ... [INF] NTP sync done.      # Time synchronization completed
```
**Connection status:**
- If you see `MQTT_ERROR_RECONNECT_FAILED`: This indicates that edge-device-core is unable to connect to the MQTT broker. Check your MQTT broker configuration and network connectivity.
- If you can see `agent connected to hub`: This indicates that edge-device-core is successfully connected to the MQTT broker.

### Stop edge-device-core

**For Option A (systemd service):**
See the [systemd setup manual](../systemd/README.md) for service management commands.

**For Option B (terminal execution):**
```bash
# If running in foreground: Press Ctrl+C
# If running in background:
sudo pkill -f edge_device_core
```

## Running wasi-nn Applications with Edge Device Core

Edge Device Core supports wasi-nn (WebAssembly System Interface for Neural Networks) for running AI inference using WebAssembly applications. This guide explains how to set up and run wasi-nn applications, using License Plate Detection (LPD) + License Plate Recognition (LPR) as an example.

**Note**: You need to prepare your own .rpk model and custom TFLite model for CPU inference for now. Sample .rpk files are available in the `/usr/share/imx500-models` directory.

### Prerequisites for wasi-nn

Before running wasi-nn applications, install the following required components:

```bash
# Install TensorFlow Lite (if needed)
# Note: TensorFlow Lite is already included in senscord
# Install manually only if required:
wget https://github.com/prepkg/tensorflow-lite-raspberrypi/releases/download/2.20.0/tensorflow-lite_64.deb
sudo apt install ./tensorflow-lite_64.deb -y
```

### Sample Application: License Plate Recognition

This example demonstrates how to set up LPD (License Plate Detection) + LPR (License Plate Recognition) using TensorFlow Lite and the wasi-nn backend.

You need to prepare your own custom TFLite model for CPU inference.

#### 1. Download Sample Applications and Models

Pre-built WebAssembly applications are available from the AITRIOS SDK Edge App repository:
- [AITRIOS SDK Edge App Releases](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/releases/latest)

#### 2. Replace the Default LPD Model on IMX500

Update the camera configuration to use the custom LPD model:

```bash
# Update the AI model configuration file
sudo sed -i 's|/usr/share/rpi-camera-assets/imx500_mobilenet_ssd.json|/opt/senscord/share/rpi-camera-assets/custom.json|' /opt/senscord/share/senscord/config_cam/senscord.xml

# Copy the custom .rpk model file
sudo cp your_custom_network.rpk /opt/senscord/share/imx500-models/
```

#### 3. Deploy Your Custom TFLite Model

Place your custom TensorFlow Lite model for CPU inference:

```bash
# Create the target directory
sudo mkdir -p /opt/senscord/share/tflite_models

# Copy your custom TFLite model (replace with your own model file)
sudo cp your_custom_model.tflite /opt/senscord/share/tflite_models/
```

#### 4. Run the Edge Device Core and Application

- **Ensure Edge Device Core is running** (see the [Running](#running) section above)

### File Locations Summary

| Item                     | Path                                                              |
|--------------------------|-------------------------------------------------------------------|
| LPD configuration        | `/opt/senscord/share/senscord/config_cam/senscord.xml`           |
| Custom model JSON        | `/opt/senscord/share/rpi-camera-assets/custom.json`              |
| LPD model (.rpk)         | `/opt/senscord/share/imx500-models/your_custom_network.rpk`                  |
| LPR model (.tflite)      | `/opt/senscord/share/tflite_models/your_custom_model.tflite`     |
| WebAssembly apps         | Any location (specify path when running)                         |
| Configuration files      | Application-specific                                              |
| TensorFlow Lite          | Included in `senscord-libcamera` or install manually if required |

> **Note**: Restart the application or device after making model deployment changes.

## Troubleshooting
### Connection Issues
- **Cannot connect to MQTT broker**:
  - Check your MQTT broker configuration and ensure it is running
  - Verify that the MQTT broker address and port are correctly set in the SQLite database

- **MQTT connection errors**:
  - Reset MQTT broker data exchange:
    ```bash
    sudo systemctl stop edge-device-core.service
    sudo rm -rf /evp_data/*
    sudo systemctl start edge-device-core.service
    ```

### Certificate Issues (AITRIOS Console only)
- **Certificate problems**:
  - Verify certificate files exist: `ls -la /etc/evp/`
  - Check certificate permissions: `sudo chmod 644 /etc/evp/*.pem`
