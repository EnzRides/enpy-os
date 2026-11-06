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

# Explicitly set Debian mirrors so host defaults (e.g. Ubuntu's archive URL)
# cannot be reused for the Debian Trixie bootstrap. live-build has no separate
# bootstrap-security mirror option; security mirrors apply to chroot and binary.
lb config \
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
