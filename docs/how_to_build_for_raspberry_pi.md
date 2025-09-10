# How to build aitrios-edge-device-core for Raspberry Pi

## Cross Compile for Raspberry Pi (from amd64, x86_64)

### Prerequisites
- Docker
- Devcontainer CLI or Visual Studio Code with the Remote - Containers extension

### 1. Clone the repository and update submodules

```bash
git clone https://github.com/aitrios/aitrios-edge-device-core.git
cd aitrios-edge-device-core
git submodule update --init
```

### 2. Download the senscord-edc-rpi .deb package

Download the latest `senscord-edc-rpi_X.X.X_arm64.deb` from the [Releases page](https://github.com/aitrios/aitrios-edge-device-sensor/releases)

Place the downloaded `.deb` file in the project root directory.

### 3. Start and enter the devcontainer (recommended)

```bash
devcontainer up --config .devcontainer/raspi/devcontainer.json --workspace-folder .
devcontainer exec --config .devcontainer/raspi/devcontainer.json --workspace-folder . bash
```

Alternatively, you can open the source directory in VS Code and select "Reopen in Container".

### 4. Clean previous builds (optional, but recommended if you built for a different architecture before)

```bash
rm -rf builddir
meson subprojects purge --confirm
```

### 5. Extract the senscord-edc-rpi package

```bash
mkdir -p extracted
dpkg-deb -x senscord-edc-rpi*_arm64.deb extracted/
sudo cp -r extracted/opt/senscord /opt/senscord
```

### 6. Configure and build the project

```bash
meson setup --cross-file=/usr/share/meson/arm64-cross builddir
ninja -C builddir
```

### 7. Create the .deb package

```bash
cd builddir
meson compile deb
```

The resulting `.deb` file will be available at `builddir/edge-device-core-X.X.X_arm64.deb`.
Transfer it to your Raspberry Pi and go to [How to use for Raspberry Pi](./how_to_use_for_raspberry_pi.md) to install and run the package.

---

## Native Compile on Raspberry Pi (aarch64, arm64)

### 1. Install dependencies

```bash
sudo apt install meson python3-kconfiglib libbsd-dev cmake libjpeg-dev \
                 sqlite3 libsqlite3-dev libnm-dev
```

### 2. Download and install the senscord-edc-rpi package

Download the latest `senscord-edc-rpi_X.X.X_arm64.deb` from the [Releases page](https://github.com/aitrios/aitrios-edge-device-sensor/releases)

Place the downloaded `.deb` file in the project root directory.

```bash
sudo rm -f /var/cache/apt/archives/senscord-edc-rpi*_arm64.deb
sudo mv senscord-edc-rpi*_arm64.deb /var/cache/apt/archives/
cd /var/cache/apt/archives/
sudo apt install ./senscord-edc-rpi*_arm64.deb
```

### 3. Build the project

```bash
git clone https://github.com/aitrios/aitrios-edge-device-core.git
cd aitrios-edge-device-core
git submodule update --init
meson setup builddir
ninja -C builddir
```
