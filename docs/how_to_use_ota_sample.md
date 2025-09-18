# How to use the OTA sample functionality

## Overview
This guide provides step-by-step instructions for using the Over-The-Air
(OTA) update sample functionality in the Edge Device Core (EDC) project.

Since OTA functionality is a system-level feature, it is not included in the EDC
module itself. Instead, we provide a sample implementation to demonstrate how to
integrate OTA functionality with EDC.

EDC provides an implementation of the AITRIOS Cloud Edge Interface protocol, which
defines an interface for OTA updates.

```json
PRIVATE_deploy_firmware {
    "targets": [
        {
            "component": 1,
            "process_state": "<done | failed>",
            "chip": "main_chip"
        }
        ...
    ]
}
```

Where:
* **component**: An integer enum value where `1` represents that the target
  of OTA is firmware. 
* **chip**: A string enum value, where `main_chip` represents the system
  firmware running on the main application processor.
* **process_state**: A string enum value that indicates the result of the
  OTA operation and is returned from the edge device to the console.

When the edge device receives the above configuration, it executes the shell
script `/sbin/edc_system_update.sh` and returns the script execution result as
either success or failure to the console through the **process_state** field.

Also, since EDC is a software module that implements the AITRIOS Cloud Edge
Interface protocol, there is no entry point defined for EDC to update itself. Therefore,
OTA updates of EDC are considered as part of the system firmware update.

In the following sections, we will demonstrate how to set up a sample environment to
implement OTA functionality with EDC.

## Sample OTA Functionality

The `samples/ota` directory of the EDC project contains a sample implementation
for OTA functionality.

* **edc_system_update.sh**: A sample shell script that executes the OTA update.
  - Executes Raspberry Pi OS package updates using the apt utility (optional, can be skipped with `--skip-os`).
  - Downloads and installs the latest EDC package (including sensor package) from
    the EDC GitHub Releases page. Since this script is executed from EDC itself, a blue-green
    deployment architecture (see below) is implemented in this script.
  - Implements dual symbolic link management for both `/opt/edc` and `/opt/senscord`.

* **edge-device-core.service**: A systemd service file that starts the EDC
  daemon based on the blue-green deployment architecture.

### Blue-Green Deployment Architecture

Since the OTA script is executed from EDC, it cannot update EDC itself.
Therefore, a blue-green deployment architecture is required to update EDC.

The EDC package is installed in the system as follows:

```
/opt/
├── edc -> edcA                 # Symbolic link to active deployment
├── senscord -> edcA/opt/senscord # Symbolic link to active senscord installation
├── edcA/                       # Blue deployment
│   ├── bin/
│   │   └── edge-device-core
│   ├── lib/
│   │   └── libparameter_storage_manager.so
│   └── opt/                    # (from senscord deb package)
│       └── senscord/
└── edcB/                       # Green deployment
    ├── bin/
    │   └── edge-device-core
    ├── lib/
    │   └── libparameter_storage_manager.so
    └── opt/                    # (from senscord deb package)
        └── senscord/
```

When the current EDC is running from `edcA` (blue deployment), the OTA script will
install the new version of EDC to `edcB` (green deployment), and vice versa.

The symbolic link `/opt/edc` points to the currently active deployment and will
be updated to point to the new deployment after a successful update. Additionally,
`/opt/senscord` points to the senscord content within the active deployment to
support applications that expect senscord at a fixed path.

`edge-device-core.service` will always start the EDC daemon from `/opt/edc/`.

## Installation

### 1. Run the Update Script to Install the Latest EDC

```bash
# Copy the script to a system location
sudo cp samples/ota/edc_system_update.sh /sbin/

# Make it executable (if not already)
sudo chmod +x /sbin/edc_system_update.sh

# Clean up files for create symbolic link
sudo rm -fr /opt/senscord
sudo rm -fr /opt/edc

# Run the update
# This will also create the initial blue-green deployment structure
sudo /sbin/edc_system_update.sh
```

### 2. Deploy the Updated Service File

```bash
# Stop the current service if it exists
sudo systemctl stop edge-device-core.service

# Copy the new service file
sudo cp samples/ota/edge-device-core.service /etc/systemd/system/

# Reload systemd configuration
sudo systemctl daemon-reload

# Start the EDC service
sudo systemctl start edge-device-core.service

# Enable the EDC service to start automatically when the system boots.
sudo systemctl enable edge-device-core.service
```

## Usage
After the installation, you can use the OTA functionality as follows:

1. Ensure EDC is running and enrolled with AITRIOS Cloud.
2. Execute the OTA update configuration from the AITRIOS Cloud Console. Note that
   only the `component` and `chip` fields are required. Other fields are ignored
   by EDC.
3. Confirm the OTA update succeeds by checking the state of the **process_state** field
   in the **PRIVATE_deploy_firmware** object. It should be `done`.
4. Issue a `reboot` direct command to reboot the edge device. After the reboot,
   the blue-green deployment will be switched to the new deployment.

Note that an explicit reboot is required to switch to the new deployment. EDC
will NOT automatically reboot the device.
