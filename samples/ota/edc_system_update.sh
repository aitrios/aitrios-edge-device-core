#!/bin/bash

# Edge Device Core System Update Script
# Version: 1.0
# Description: Blue-green deployment script for updating edge-device-core system
# Author: Generated for AITRIOS Edge Device Core

set -euo pipefail

# Script configuration
readonly SCRIPT_NAME="edc_system_update.sh"
readonly SCRIPT_VERSION="1.0"
readonly LOG_FILE="/var/log/edc_system_update.log"

# Deployment directories
readonly EDC_DIR_A="/opt/edcA"
readonly EDC_DIR_B="/opt/edcB"
readonly EDC_SYMLINK="/opt/edc"
readonly SENSCORD_SYMLINK="/opt/senscord"
readonly TEMP_DIR="/tmp/edc_update_$$"

# GitHub repositories
readonly CORE_REPO="aitrios/aitrios-edge-device-core"
readonly SENSOR_REPO="aitrios/aitrios-edge-device-sensor"

# Downloaded files
CORE_PACKAGE=""
SENSOR_PACKAGE=""
CORE_BINARY=""

# Version information
CORE_VERSION=""
SENSOR_VERSION=""

# Command-line options
FORCE_UPDATE=false
SKIP_OS_UPDATE=false

# Color codes for output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m' # No Color

# Initialize logging
init_logging() {
    exec 1> >(tee -a "${LOG_FILE}")
    exec 2> >(tee -a "${LOG_FILE}" >&2)
    log_info "=== Edge Device Core Update Started at $(date) ==="
}

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1" >&1
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1" >&2
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

log_debug() {
    echo -e "${BLUE}[DEBUG]${NC} $1" >&1
}

# Error handling
error_exit() {
    log_error "$1"
    cleanup
    exit 1
}

# Cleanup function
cleanup() {
    log_info "Cleaning up temporary files..."
    if [[ -d "${TEMP_DIR}" ]]; then
        rm -rf "${TEMP_DIR}"
        log_debug "Removed temporary directory: ${TEMP_DIR}"
    fi
}

# Trap for cleanup on exit
trap cleanup EXIT

# Check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        error_exit "This script must be run as root"
    fi
}

# Check system requirements
check_requirements() {
    log_info "Checking system requirements..."
    
    local required_commands=("curl" "dpkg-deb" "jq" "tar" "mkdir" "ln" "chmod")
    for cmd in "${required_commands[@]}"; do
        if ! command -v "$cmd" &> /dev/null; then
            if [[ "$cmd" == "jq" ]]; then
                error_exit "Required command not found: $cmd. Install with: sudo apt install -y jq"
            else
                error_exit "Required command not found: $cmd"
            fi
        fi
    done
    
    # Check available space (need at least 1GB free in /opt)
    local available_space
    available_space=$(df /opt | awk 'NR==2 {print $4}')
    if [[ $available_space -lt 1048576 ]]; then # 1GB in KB
        error_exit "Insufficient disk space in /opt (need at least 1GB free)"
    fi
    
    log_info "System requirements check passed"
}

# Get latest release info from GitHub
get_latest_release() {
    local repo="$1"
    local release_info
    
    # Don't log here to avoid mixing with JSON output
    
    if ! release_info=$(curl -s "https://api.github.com/repos/${repo}/releases/latest"); then
        log_error "Failed to fetch release information for ${repo}"
        return 1
    fi
    
    # Check if response is empty
    if [[ -z "$release_info" ]]; then
        log_error "Empty response from GitHub API for ${repo}"
        return 1
    fi
    
    # Validate JSON response
    if ! echo "$release_info" | jq . > /dev/null 2>&1; then
        log_error "Invalid JSON response from GitHub API for ${repo}"
        log_debug "Response length: $(echo "$release_info" | wc -c) characters"
        log_debug "Response start: $(echo "$release_info" | head -c 200)"
        log_debug "Response end: $(echo "$release_info" | tail -c 200)"
        return 1
    fi
    
    echo "$release_info"
}

# Download file from URL
download_file() {
    local url="$1"
    local output_file="$2"
    
    log_info "Downloading $(basename "$output_file")..."
    
    if ! curl -L -o "$output_file" "$url"; then
        error_exit "Failed to download $url"
    fi
    
    if [[ ! -f "$output_file" ]]; then
        error_exit "Downloaded file not found: $output_file"
    fi
    
    log_debug "Successfully downloaded: $output_file"
}

# Download edge-device-core packages
download_core_packages() {
    log_info "Downloading edge-device-core packages..."
    
    local release_info
    log_debug "Fetching latest release info for ${CORE_REPO}..."
    
    if ! release_info=$(get_latest_release "$CORE_REPO"); then
        error_exit "Failed to get release information for ${CORE_REPO}"
    fi
    
    local download_url_binary
    
    # Extract download URL for binary package with better error handling
    log_debug "Extracting download URL from release info..."
    
    # Use a more robust approach - save to temporary file first
    local temp_json="${TEMP_DIR}/release_info.json"
    echo "$release_info" > "$temp_json"
    
    # Verify the temp file was created and has content
    if [[ ! -f "$temp_json" ]]; then
        error_exit "Failed to create temporary JSON file"
    fi
    
    local file_size=$(wc -c < "$temp_json")
    log_debug "Temporary JSON file size: $file_size bytes"
    
    # Test if jq can parse the file
    if ! jq . "$temp_json" > /dev/null 2>&1; then
        log_error "Cannot parse JSON in temporary file"
        log_debug "File content preview: $(head -5 "$temp_json")"
        error_exit "Invalid JSON in temporary file"
    fi
    
    # Extract URL for the .deb package (contains both binary and library)
    download_url_binary=$(jq -r '.assets[] | select(.name | test("edge-device-core-[0-9.]+.*_arm64\\.deb$")) | .browser_download_url' "$temp_json" 2>/dev/null || echo "")
    
    # Debug: show all available assets
    log_debug "All available assets:"
    jq -r '.assets[].name' "$temp_json" 2>/dev/null | while read -r asset; do
        log_debug "  - $asset"
    done
    
    if [[ -z "$download_url_binary" ]]; then
        error_exit "Could not find edge-device-core .deb package in latest release"
    fi
    
    log_debug "Binary URL: $download_url_binary"
    
    # Download the .deb package (contains both binary and library)
    CORE_PACKAGE="${TEMP_DIR}/edge-device-core.deb"
    
    download_file "$download_url_binary" "$CORE_PACKAGE"
    
    log_info "Edge-device-core package downloaded successfully"
}

# Download sensor package
download_sensor_package() {
    log_info "Downloading sensor package..."
    
    local release_info
    log_debug "Fetching latest release info for ${SENSOR_REPO}..."
    
    if ! release_info=$(get_latest_release "$SENSOR_REPO"); then
        error_exit "Failed to get release information for ${SENSOR_REPO}"
    fi
    
    local download_url
    log_debug "Extracting sensor package download URL..."
    
    # Use a more robust approach - save to temporary file first
    local temp_json="${TEMP_DIR}/sensor_release_info.json"
    echo "$release_info" > "$temp_json"
    
    download_url=$(jq -r '.assets[] | select(.name | test("senscord-edc-rpi-[0-9.]+.*_arm64\\.deb$")) | .browser_download_url' "$temp_json" 2>/dev/null || echo "")
    
    if [[ -z "$download_url" ]]; then
        log_debug "Available assets:"
        jq -r '.assets[].name' "$temp_json" 2>/dev/null || log_debug "Could not parse assets list"
        error_exit "Could not find senscord-edc-rpi package in latest release"
    fi
    
    log_debug "Sensor package URL: $download_url"
    
    SENSOR_PACKAGE="${TEMP_DIR}/senscord-edc-rpi.deb"
    download_file "$download_url" "$SENSOR_PACKAGE"
    
    log_info "Sensor package downloaded successfully"
}

# Detect current deployment
detect_current_deployment() {
    if [[ ! -L "$EDC_SYMLINK" ]]; then
        log_warn "Symbolic link $EDC_SYMLINK not found, assuming first installation"
        echo ""
        return
    fi
    
    local current_target
    current_target=$(readlink "$EDC_SYMLINK")
    
    # Don't log here to avoid mixing with return value
    echo "$current_target"
}

# Determine target deployment directory
get_target_deployment() {
    local current_deployment="$1"
    
    case "$current_deployment" in
        "$EDC_DIR_A")
            echo "$EDC_DIR_B"
            ;;
        "$EDC_DIR_B")
            echo "$EDC_DIR_A"
            ;;
        "")
            # First installation, use edcA
            echo "$EDC_DIR_A"
            ;;
        *)
            log_warn "Unknown current deployment: $current_deployment, using edcA"
            echo "$EDC_DIR_A"
            ;;
    esac
}

# Create deployment directory structure
create_deployment_structure() {
    local target_dir="$1"
    
    log_info "Creating deployment structure in $target_dir..."
    
    # Remove existing directory if it exists
    if [[ -d "$target_dir" ]]; then
        log_debug "Removing existing deployment directory: $target_dir"
        rm -rf "$target_dir"
    fi
    
    # Create directory structure (no senscord subfolder - content goes directly in root)
    mkdir -p "$target_dir"/{bin,lib}
    
    log_debug "Created deployment directory structure in $target_dir"
}

# Extract and install core packages
install_core_packages() {
    local target_dir="$1"
    
    log_info "Installing core packages to $target_dir..."
    log_info "Installing edge-device-core version: $CORE_VERSION"
    
    # Extract core .deb package and find the binary and library
    local extract_dir="${TEMP_DIR}/core_extract"
    mkdir -p "$extract_dir"
    
    if ! dpkg-deb -x "$CORE_PACKAGE" "$extract_dir"; then
        error_exit "Failed to extract core .deb package"
    fi
    
    # Find the edge_device_core binary (typically in usr/bin)
    local binary_path
    binary_path=$(find "$extract_dir" -name "edge_device_core" -type f | head -1)
    
    if [[ -z "$binary_path" ]]; then
        log_debug "Searching for any executable files..."
        find "$extract_dir" -type f -executable | head -10 | while read -r file; do
            log_debug "Found executable: $file"
        done
        error_exit "Could not find edge_device_core binary in extracted package"
    fi
    
    log_debug "Found binary at: $binary_path"
    
    # Find the libparameter_storage_manager.so library in the extracted package
    local library_path
    library_path=$(find "$extract_dir" -name "libparameter_storage_manager.so" -type f | head -1)
    
    if [[ -z "$library_path" ]]; then
        log_debug "Searching for library files..."
        find "$extract_dir" -name "*.so" | head -10 | while read -r file; do
            log_debug "Found library: $file"
        done
        error_exit "Could not find libparameter_storage_manager.so in extracted package"
    fi
    
    log_debug "Found library at: $library_path"
    
    # Copy binary to target (rename if necessary)
    cp "$binary_path" "$target_dir/bin/edge_device_core"
    chmod +x "$target_dir/bin/edge_device_core"
    
    # Copy library to target (extracted from the same .deb package)
    cp "$library_path" "$target_dir/lib/libparameter_storage_manager.so"
    chmod 644 "$target_dir/lib/libparameter_storage_manager.so"
    
    # Create version file in /tmp then move to target
    local temp_version_file="/tmp/version_edc_$$.txt"
    echo "Version: $CORE_VERSION" > "$temp_version_file"
    chmod 644 "$temp_version_file"
    mv "$temp_version_file" "$target_dir/version_edc.txt"
    log_info "Created EDC version file: $target_dir/version_edc.txt (Version: $CORE_VERSION)"
    
    log_debug "Core packages installed successfully"
}

# Extract and install sensor package
install_sensor_package() {
    local target_dir="$1"
    
    log_info "Installing sensor package to $target_dir..."
    
    # Extract version from .deb package
    SENSOR_VERSION=$(dpkg-deb -I "$SENSOR_PACKAGE" | grep '^ Version:' | awk '{print $2}')
    if [[ -z "$SENSOR_VERSION" ]]; then
        error_exit "Could not extract version from sensor .deb package"
    fi
    log_info "Installing senscord version: $SENSOR_VERSION"
    
    # Extract .deb package directly to target directory (not to senscord subdirectory)
    if ! dpkg-deb -x "$SENSOR_PACKAGE" "$target_dir"; then
        error_exit "Failed to extract sensor package"
    fi
    
    # Create version file in senscord directory
    local senscord_dir="$target_dir/opt/senscord"
    if [[ ! -d "$senscord_dir" ]]; then
        error_exit "Senscord directory not found after extraction: $senscord_dir"
    fi
    
    # Create version file in /tmp then move to target
    local temp_version_file="/tmp/version_senscord_$$.txt"
    echo "Version: $SENSOR_VERSION" > "$temp_version_file"
    chmod 644 "$temp_version_file"
    mv "$temp_version_file" "$senscord_dir/version_senscord.txt"
    log_info "Created senscord version file: $senscord_dir/version_senscord.txt (Version: $SENSOR_VERSION)"
    
    log_debug "Sensor package installed successfully (content extracted directly to deployment root)"
}

# Validate installation
validate_installation() {
    local target_dir="$1"
    
    log_info "Validating installation in $target_dir..."
    
    # Check required files exist
    local required_files=(
        "$target_dir/bin/edge_device_core"
        "$target_dir/lib/libparameter_storage_manager.so"
    )
    
    for file in "${required_files[@]}"; do
        if [[ ! -f "$file" ]]; then
            error_exit "Required file missing: $file"
        fi
    done
    
    # Check senscord content exists and specifically validate opt/senscord directory
    local senscord_target="$target_dir/opt/senscord"
    if [[ ! -d "$senscord_target" ]]; then
        log_debug "Looking for any extracted content in target directory..."
        ls -la "$target_dir" || true
        error_exit "Required senscord directory not found: $senscord_target"
    fi
    
    log_debug "Found senscord content at: $senscord_target"
    
    # Check binary is executable
    if [[ ! -x "$target_dir/bin/edge_device_core" ]]; then
        error_exit "edge_device_core binary is not executable"
    fi
    
    log_info "Installation validation passed"
}

# Update symbolic link
update_symbolic_link() {
    local target_dir="$1"
    
    log_info "Updating symbolic links to point to $target_dir..."
    
    # Remove existing EDC symbolic link if it exists
    if [[ -L "$EDC_SYMLINK" ]]; then
        rm "$EDC_SYMLINK"
    elif [[ -e "$EDC_SYMLINK" ]]; then
        error_exit "$EDC_SYMLINK exists but is not a symbolic link"
    fi
    
    # Remove existing senscord symbolic link if it exists
    if [[ -L "$SENSCORD_SYMLINK" ]]; then
        rm "$SENSCORD_SYMLINK"
    elif [[ -e "$SENSCORD_SYMLINK" ]]; then
        error_exit "$SENSCORD_SYMLINK exists but is not a symbolic link"
    fi
    
    # Create new EDC symbolic link
    if ! ln -s "$target_dir" "$EDC_SYMLINK"; then
        error_exit "Failed to create EDC symbolic link"
    fi
    
    # Create new senscord symbolic link pointing to senscord content within deployment
    local senscord_target="$target_dir/opt/senscord"
    if [[ ! -d "$senscord_target" ]]; then
        error_exit "Senscord directory not found at $senscord_target"
    fi
    
    if ! ln -s "$senscord_target" "$SENSCORD_SYMLINK"; then
        error_exit "Failed to create senscord symbolic link"
    fi
    
    log_info "Symbolic links updated successfully"
    log_debug "EDC link: $EDC_SYMLINK -> $target_dir"
    log_debug "Senscord link: $SENSCORD_SYMLINK -> $senscord_target"
}

# Main update function
perform_update() {
    log_info "Starting edge-device-core system update..."
    
    # Create temporary directory
    mkdir -p "$TEMP_DIR"
    log_debug "Created temporary directory: $TEMP_DIR"
    
    # Download packages
    download_core_packages
    download_sensor_package
    
    # Extract version from downloaded package
    CORE_VERSION=$(dpkg-deb -I "$CORE_PACKAGE" | grep '^ Version:' | awk '{print $2}')
    if [[ -z "$CORE_VERSION" ]]; then
        error_exit "Could not extract version from downloaded .deb package"
    fi
    log_info "Downloaded edge-device-core version: $CORE_VERSION"
    
    # Check if update is needed (unless --force is specified)
    if [[ "$FORCE_UPDATE" == true ]]; then
        log_info "Force update enabled, skipping version check"
    else
        local current_version_file="$EDC_SYMLINK/version_edc.txt"
        if [[ -f "$current_version_file" ]]; then
            local current_version
            current_version=$(grep '^Version:' "$current_version_file" | awk '{print $2}')
            
            if [[ -n "$current_version" && "$current_version" == "$CORE_VERSION" ]]; then
                log_info "Already on latest version: $current_version"
                log_info "Skipping update. Use --force to update anyway."
                return 0
            fi
            
            log_info "Current version: ${current_version:-unknown}, Latest version: $CORE_VERSION"
        else
            log_info "No current version file found, proceeding with installation"
        fi
    fi
    
    # Determine deployment strategy
    local current_deployment target_deployment
    current_deployment=$(detect_current_deployment)
    target_deployment=$(get_target_deployment "$current_deployment")
    
    log_debug "Current deployment target: $current_deployment"
    log_info "Current deployment: ${current_deployment:-"(none)"}"
    log_info "Target deployment: $target_deployment"
    
    # Create and populate target deployment
    create_deployment_structure "$target_deployment"
    install_core_packages "$target_deployment"
    install_sensor_package "$target_deployment"
    
    # Validate installation before updating symbolic link
    validate_installation "$target_deployment"
    
    # Update symbolic link (this activates the new deployment)
    update_symbolic_link "$target_deployment"
    
    log_info "Update completed successfully! New version will be active after next reboot."
    log_info "Current deployment: $current_deployment"
    log_info "New deployment: $target_deployment"
}

# Print usage information
usage() {
    cat << EOF
Usage: $SCRIPT_NAME [OPTIONS]

Edge Device Core System Update Script v$SCRIPT_VERSION

This script performs a blue-green deployment update of the edge-device-core system.
It downloads the latest packages and installs them to an alternate directory,
enabling zero-downtime updates.

OPTIONS:
    -h, --help      Show this help message
    -v, --version   Show version information
    --force         Force update even if current version appears to be latest
    --skip-os       Skip OS update (only update edge-device-core packages)

EXAMPLES:
    $SCRIPT_NAME                    # Perform standard update (includes OS update)
    $SCRIPT_NAME --force            # Force update regardless of version
    $SCRIPT_NAME --skip-os          # Update only edge-device-core packages

DEPLOYMENT STRUCTURE:
    /opt/edc -> /opt/edcA (or /opt/edcB)    # Symbolic link to active deployment
    /opt/edcA/                              # Blue deployment
    /opt/edcB/                              # Green deployment

REQUIREMENTS:
    - Root privileges
    - Internet connectivity
    - Minimum 1GB free space in /opt
    - Required utilities: curl, dpkg-deb, jq, tar

For more information, see the documentation in the samples/ota/ directory.
EOF
}

# Print version information
version() {
    echo "$SCRIPT_NAME version $SCRIPT_VERSION"
    echo "Edge Device Core System Update Script"
    echo "Generated for AITRIOS Edge Device Core"
}

# OS Update
os_update() {
    log_info "Performing OS update..."
    
    if ! apt update -y; then
        log_error "Failed to update package lists"
        return 1
    fi
    
    if ! apt upgrade -y; then
        log_error "Failed to upgrade packages"
        return 1
    fi
    
    log_info "OS update completed successfully"
}

# Main function
main() {
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                usage
                exit 0
                ;;
            -v|--version)
                version
                exit 0
                ;;
            --force)
                FORCE_UPDATE=true
                shift
                ;;
            --skip-os)
                SKIP_OS_UPDATE=true
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    done
    
    # Initialize
    init_logging
    
    log_info "Edge Device Core System Update Script v$SCRIPT_VERSION"
    log_info "Started at: $(date)"
    
    # Perform checks
    check_root
    check_requirements

    # Execute OS update (unless skipped)
    if [[ "$SKIP_OS_UPDATE" != true ]]; then
        if ! os_update; then
            log_warn "OS update failed, but continuing with edge-device-core update"
        fi
    else
        log_info "Skipping OS update as requested"
    fi
    
    # Perform update
    perform_update
    
    log_info "=== Edge Device Core Update Completed at $(date) ==="
    log_info "Please reboot the device to activate the new version."
}

# Run main function with all arguments
main "$@"
