#!/usr/bin/env bash
set -euo pipefail

# Build the Debian-based ENPY OS live image from this directory.
# Run as a regular user; live-build will request privileges where required.
cd "$(dirname "$(realpath "$0")")"

command -v lb >/dev/null || {
    echo "Missing live-build. Install it with: sudo apt install live-build" >&2
    exit 1
}

lb config \
    --distribution trixie \
    --architectures amd64 \
    --binary-images iso-hybrid \
    --archive-areas "main contrib non-free non-free-firmware" \
    --debian-installer live

lb build
