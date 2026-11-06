#!/usr/bin/env bash
set -euo pipefail

# Build the Debian-based ENPY OS live image from this directory.
# Recommended build host: Debian Trixie amd64. Run as a regular user;
# live-build will request privileges where required.
cd "$(dirname "$(realpath "$0")")"

command -v lb >/dev/null || {
    echo "Missing live-build. Install it with: sudo apt install live-build" >&2
    exit 1
}

DEBIAN_KEYRING=/usr/share/keyrings/debian-archive-keyring.gpg
if [[ ! -r "$DEBIAN_KEYRING" ]]; then
    echo "Missing Debian archive keyring: $DEBIAN_KEYRING" >&2
    echo "On Debian/Ubuntu, install it with: sudo apt install debian-archive-keyring" >&2
    exit 1
fi

# Explicit Debian mode is essential when live-build runs on Ubuntu: without it,
# live-build selects Ubuntu's mode and parent mirrors from the host defaults.
# Explicit mirrors prevent host defaults from being reused for Debian Trixie.
# live-build has no separate bootstrap-security mirror option; security mirrors
# apply to chroot and binary stages.
lb config \
    --mode debian \
    --distribution trixie \
    --architectures amd64 \
    --binary-images iso-hybrid \
    --archive-areas "main contrib non-free non-free-firmware" \
    --mirror-bootstrap https://deb.debian.org/debian \
    --mirror-chroot https://deb.debian.org/debian \
    --mirror-binary https://deb.debian.org/debian \
    --mirror-chroot-security https://security.debian.org/debian-security \
    --mirror-binary-security https://security.debian.org/debian-security \
    --debian-installer live

lb build
