# Edge Device Core System Update Support

This directory contains sample files for supporting edge device core package updates,
which implement a blue-green deployment strategy.

## Files

### `edc_system_update.sh`
The main installation script that performs zero-downtime updates of the edge-device-core system using a blue-green deployment strategy.

### `edge-device-core.service`
Updated systemd service file that works with the blue-green deployment structure.

## Overview

The update system implements a blue-green deployment strategy where:
- **Blue deployment**: `/opt/edcA/`
- **Green deployment**: `/opt/edcB/`
- **Active deployment**: `/opt/edc` (symbolic link pointing to current active deployment)
- **Senscord access**: `/opt/senscord` (symbolic link pointing to senscord content in active deployment)

## Installation

### 1. Run the Update Script to Install the Latest EDC

```bash
# Copy the script to a system location
sudo cp samples/ota/edc_system_update.sh /sbin/

# Make it executable (if not already)
sudo chmod +x /sbin/edc_system_update.sh

# Run the update
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

# Enable the EDC service to start automatically when system is booted.
sudo systemctl enable edge-device-core.service
```


## Usage

### Basic Usage of Installation Script
```bash
# Perform a standard update
sudo edc_system_update.sh

# Show help
sudo edc_system_update.sh --help

# Show version
sudo edc_system_update.sh --version

# Skip OS updates (only update EDC and senscord packages)
sudo edc_system_update.sh --skip-os

# Force update even if versions appear current
sudo edc_system_update.sh --force
```

### What the Script Does

1. **Download Phase**:
   - Fetches latest `edge-device-core` binary and `libparameter_storage_manager.so` from GitHub releases
   - Downloads latest `senscord-edc-rpi-*_arm64.deb` package

2. **OS Update Phase** (optional):
   - Updates Raspberry Pi OS packages using apt (can be skipped with `--skip-os`)
   - Performs `apt update` and `apt upgrade` operations

3. **Deployment Detection**:
   - Detects current active deployment (`/opt/edcA` or `/opt/edcB`)
   - Selects alternate directory for new installation

4. **Installation Phase**:
   - Creates directory structure: `bin/`, `lib/`
   - Extracts and installs packages to target directory
   - Senscord content is extracted directly to deployment root
   - Validates installation integrity

5. **Activation Phase**:
   - Updates both `/opt/edc` and `/opt/senscord` symbolic links atomically
   - Points to new deployment after successful installation (provides rollback protection)

### Directory Structure After Installation

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

## Service Configuration

The updated `edge-device-core.service` file includes:

- **Dynamic Path**: Uses `/opt/edc/bin/edge-device-core` (follows symbolic link)
- **Environment Variables**: All environment settings embedded in service file
- **Library Path**: Sets `LD_LIBRARY_PATH` to include deployment-specific libraries
- **Senscord Integration**: Uses hardcoded `/opt/senscord` path for compatibility
- **Root Privileges**: Runs as root user/group for system access

### Key Service File Changes

```ini
# Uses symbolic link for blue-green deployment
ExecStart=/opt/edc/bin/edge-device-core

# Using symbolic deployment directory to set up environment variables embedded in service file
Environment="EDGE_DEVICE_CORE_DB_PATH=/var/lib/edge-device-core/db.sqlite3"
Environment="LD_LIBRARY_PATH=/opt/edc/lib:/opt/edc/opt/senscord/lib:/opt/edc/opt/senscord/lib/senscord/utility:/opt/edc/opt/senscord/lib/aarch64-linux-gnu:/usr/lib/aarch64-linux-gnu"
Environment="SENSCORD_INSTALL_PATH=/opt/edc/opt/senscord"
Environment="SENSCORD_FILE_PATH=/opt/edc/opt/senscord/share/senscord/config:/opt/edc/opt/senscord/lib:/opt/edc/opt/senscord/lib/senscord/component:/opt/edc/opt/senscord/lib/senscord/allocator:/opt/edc/opt/senscord/lib/senscord/connection:/opt/edc/opt/senscord/lib/senscord/converter:/opt/edc/opt/senscord/lib/senscord/recoder:/opt/edc/opt/senscord/lib/senscord/extension"
Environment="LD_PRELOAD=/opt/edc/opt/senscord/lib/rpicam_app_mod.so"
Environment="PATH=/opt/edc/opt/senscord/bin:/bin:/usr/bin:/usr/local/bin"

```

## System Requirements

### Prerequisites
- **Operating System**: Raspberry Pi OS (64-bit, Debian Bookworm)
- **Privileges**: Root access required
- **Disk Space**: Minimum 1GB free space in `/opt/`
- **Network**: Internet connectivity for GitHub downloads

### Required Utilities
- `curl` - For downloading packages
- `dpkg-deb` - For extracting .deb packages  
- `jq` - For parsing GitHub API responses
- `tar` - For extracting compressed archives
- Standard utilities: `mkdir`, `ln`, `chmod`, `find`

### Installation of Requirements
```bash
# Install required packages
sudo apt update
sudo apt install -y curl jq

# dpkg-deb is part of dpkg (usually pre-installed)
# tar and other utilities are typically pre-installed
```

## Safety Features

### Rollback Protection
- Installation to alternate directory ensures current deployment remains intact
- Both symbolic links (`/opt/edc` and `/opt/senscord`) only updated after successful installation and validation
- On failure, current deployment continues to work

### Error Handling
- Comprehensive error checking at each step
- Automatic cleanup of temporary files
- Detailed logging to `/var/log/edc_system_update.log`
- Graceful failure without affecting running system

### Validation
- Checks for required files after installation
- Verifies binary executability
- Ensures senscord content is properly extracted to `target_dir/opt/senscord`
- Validates available disk space before starting

## Troubleshooting

### Common Issues

#### Download Failures
```bash
# Check internet connectivity
ping -c 4 api.github.com

# Check GitHub API access
curl -s https://api.github.com/repos/aitrios/aitrios-edge-device-core/releases/latest
```

#### Permission Issues
```bash
# Ensure running as root
sudo whoami

# Check /opt permissions
ls -la /opt/
```

#### Disk Space Issues
```bash
# Check available space
df -h /opt/

# Clean up old deployments manually if needed
sudo rm -rf /opt/edcA  # Only if not currently active
sudo rm -rf /opt/edcB  # Only if not currently active
```

#### Service Issues
```bash
# Check service status
sudo systemctl status edge-device-core.service

# Check service logs
sudo journalctl -u edge-device-core.service -f

# Verify symbolic link
ls -la /opt/edc
```

### Log Files
- **Update logs**: `/var/log/edc_system_update.log`
- **Service logs**: `sudo journalctl -u edge-device-core.service`

### Recovery

#### Manual Rollback
If the new deployment has issues after reboot:
```bash
# Check current deployment
readlink /opt/edc

# Switch back to previous deployment (both links must be updated together)
sudo rm /opt/edc /opt/senscord
sudo ln -s /opt/edcA /opt/edc  # or /opt/edcB
sudo ln -s /opt/edcA/opt/senscord /opt/senscord  # or /opt/edcB/opt/senscord

# Restart service
sudo systemctl restart edge-device-core.service
```

#### Complete Reset
To return to package-based installation:
```bash
# Stop service
sudo systemctl stop edge-device-core.service

# Remove blue-green deployments and dual symbolic links
sudo rm -rf /opt/edc /opt/senscord /opt/edcA /opt/edcB

# Restore original service file
sudo cp /etc/systemd/system/edge-device-core.service.backup /etc/systemd/system/edge-device-core.service

# Reload systemd
sudo systemctl daemon-reload

# Reinstall packages normally (see docs/how_to_use_for_raspberry_pi.md)
```

## Integration

### Automated Updates
The script can be integrated with:
- **Cron jobs**: For scheduled updates
- **System services**: For triggered updates
- **Monitoring systems**: For remote update management

### Example Cron Job
```bash
# Update daily at 2 AM
0 2 * * * /sbin/edc_system_update.sh >> /var/log/edc_cron_update.log 2>&1
```

## Security Considerations

- Script requires root privileges for system modifications
- Downloads are performed over HTTPS from trusted GitHub repositories
- No authentication required as repositories are public
- Temporary files are cleaned up after installation
- Original deployments are preserved until the next update cycle

## Support

For issues or questions:
1. Check the log files for detailed error information
2. Refer to the main documentation: `docs/how_to_use_for_raspberry_pi.md`
3. Verify system requirements and prerequisites
4. Test with `--dry-run` mode when available

---

**Note**: This is a sample implementation. Test thoroughly in your environment before production use.
