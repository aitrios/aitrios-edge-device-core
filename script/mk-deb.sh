#!/bin/sh

# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

set -e

arch=amd64
version=0.0.0

usage()
{
    echo usage: mk-deb[-a arch][-V version] >&2
    exit 1
}

while test $# -gt 0; do
    case "$1" in
    -a)
        arch=${2?`usage`}
        shift 2
        ;;
    -V)
        version=${2?`usage`}
        shift 2
        ;;
    -d)
        buildroot=${2?`usage`}
        shift 2
        ;;
    *)
        usage
        ;;
    esac
done

cd "${MESON_SOURCE_ROOT}"

rm -rf "${buildroot}/dist"
trap "rm -rf ${buildroot}/dist $$.tmp" EXIT HUP INT TERM

case $arch in
aarch64)
    debarch=arm64
    ;;
x86_64)
    debarch=amd64
    ;;
esac

# create deb package
mkdir -p "${buildroot}/dist/DEBIAN"

cat > "${buildroot}/dist/DEBIAN/control" <<EOF
Package: edge-device-core
Section: contrib/misc
Version: $version
Priority: optional
Architecture: $debarch
Maintainer: Shinsuke Tashiro <Shinsuke.Tashiro@sony.com>
Depends: libc6 (>= 2.35), ca-certificates, libjpeg62-turbo, sqlite3, libsqlite3-dev, libnm-dev, libopencv-dev
Description: AITRIOS Edge Device Core
 This package provides the Sony AITRIOS Edge Device Core
EOF

# copy main executable
mkdir -p "${buildroot}/dist/usr/bin"
cp "${buildroot}/edge_device_core" "${buildroot}/dist/usr/bin/"

# copy shared library
case "$debarch" in
    arm64)   LIBDIR="/usr/lib/aarch64-linux-gnu" ;;
    amd64)   LIBDIR="/usr/lib/x86_64-linux-gnu" ;;
    *) echo "Unsupported arch: $debarch" >&2; exit 1 ;;
esac
mkdir -p "${buildroot}/dist${LIBDIR}"
cp "${buildroot}/libparameter_storage_manager.so" "${buildroot}/dist${LIBDIR}/"

# copy systemd service file
mkdir -p "${buildroot}/dist/lib/systemd/system"
cp "${MESON_SOURCE_ROOT}/systemd/edge-device-core.service" "${buildroot}/dist/lib/systemd/system/"

# copy systemd environment file
mkdir -p "${buildroot}/dist/etc/default"
cp "${MESON_SOURCE_ROOT}/systemd/edge-device-core.env" "${buildroot}/dist/etc/default/"

# copy update_psm_db.py script to package
mkdir -p "${buildroot}/dist/var/lib/edge-device-core"
cp "${MESON_SOURCE_ROOT}/script/update_psm_db.py" "${buildroot}/dist/var/lib/edge-device-core/"

# Create postinst script
cat > "${buildroot}/dist/DEBIAN/postinst" <<'EOF'
#!/bin/bash
set -e

# Create required directories
mkdir -p /var/lib/edge-device-core
mkdir -p /evp_data
mkdir -p /etc/evp

# Set appropriate permissions
chmod 755 /var/lib/edge-device-core
chmod 755 /evp_data
chmod 755 /etc/evp

touch /etc/evp/00000000-0000-0000-0000-000000000000_cert.pem
touch /etc/evp/00000000-0000-0000-0000-000000000000_key.pem

# Setup database and initial configuration
if [ -f /var/lib/edge-device-core/update_psm_db.py ]; then
    chmod +x /var/lib/edge-device-core/update_psm_db.py
    
    # Create database with proper table structure
    sqlite3 "/var/lib/edge-device-core/db.sqlite3" "CREATE TABLE IF NOT EXISTS psm_data (key INTEGER PRIMARY KEY UNIQUE, value TEXT);"

    # Define key-value pairs for database initialization
    db_entries="PlStorageDataEvpHubURL=127.0.0.1 PlStorageDataEvpHubPort=1883 PlStorageDataEvpIotPlatform=TB PlStorageDataEvpTls=1"
    
    # Iterate over key-value pairs and execute python3 command
    for entry in $db_entries; do
        key=$(echo $entry | cut -d= -f1)
        value=$(echo $entry | cut -d= -f2)
        python3 /var/lib/edge-device-core/update_psm_db.py --key "$key" --value "$value" --db-file /var/lib/edge-device-core/db.sqlite3 || echo "Warning: Failed to set $key"
    done
    
    # Set appropriate permissions for the database
    chmod 644 /var/lib/edge-device-core/db.sqlite3
    chown root:root /var/lib/edge-device-core/db.sqlite3
else
    echo "Warning: update_psm_db.py not found, skipping database initialization" >&2
fi

# Reload systemd daemon and enable service
if [ -d /run/systemd/system ]; then
    systemctl daemon-reload
    
    echo "edge-device-core service files installed."
    echo ""
    echo "To set up and start the service:"
    echo "1. Configure database and certificates as described in the documentation"
    echo "2. Enable the service: sudo systemctl enable edge-device-core.service"
    echo "3. Start the service: sudo systemctl start edge-device-core.service"
    echo ""
    echo "The service will automatically start on boot after enabling."
fi

exit 0
EOF

chmod +x "${buildroot}/dist/DEBIAN/postinst"

# Create prerm script
cat > "${buildroot}/dist/DEBIAN/prerm" <<'EOF'
#!/bin/bash
set -e

if [ -d /run/systemd/system ]; then
    # Stop the service if it's running
    if systemctl is-active --quiet edge-device-core.service 2>/dev/null; then
        echo "Stopping edge-device-core service..."
        systemctl stop edge-device-core.service
    fi
    
    # Disable the service to prevent automatic startup
    if systemctl is-enabled --quiet edge-device-core.service 2>/dev/null; then
        echo "Disabling edge-device-core service..."
        systemctl disable edge-device-core.service
    fi
fi

exit 0
EOF

chmod +x "${buildroot}/dist/DEBIAN/prerm"

# Create postrm script  
cat > "${buildroot}/dist/DEBIAN/postrm" <<'EOF'
#!/bin/bash
set -e

if [ -d /run/systemd/system ]; then
    systemctl daemon-reload
fi

# Remove data directory on purge
if [ "$1" = "purge" ]; then
    echo "Removing edge-device-core data directory..."
    rm -rf /var/lib/edge-device-core
    rm -rf /evp_data
    rm -rf /etc/evp
fi

exit 0
EOF

chmod +x "${buildroot}/dist/DEBIAN/postrm"

dpkg-deb --build "${buildroot}/dist" "${buildroot}/edge-device-core-${version}_$debarch.deb"
