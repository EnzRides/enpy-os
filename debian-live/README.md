# ENPY OS — Debian live image starter

This directory is the beginning of the **Debian-based ENPY OS distribution**. It builds Debian Live with XFCE, ENPY identity, a custom wallpaper asset, and a Fastfetch preset. It does not replace the experimental freestanding kernel in `kernel/`; this distribution boots the Debian Linux kernel.

## Build

Use a Debian Trixie amd64 build host (or a Debian Trixie amd64 VM), install the live-build tools, then run:

```sh
sudo apt update
sudo apt install live-build debootstrap squashfs-tools xorriso
cd debian-live
bash build.sh
```

The build can take a while and needs internet access and several gigabytes of free disk space. The generated ISO is produced by live-build in this directory. Test it in QEMU before using it on physical hardware. Use a disposable VM disk for installation tests; the live installer can modify disks.

## Current branding

- ENPY name and project metadata in the live system's `/etc/os-release`, `/etc/issue`, and `/etc/issue.net`.
- Custom ENPY wallpaper artwork at `/usr/share/backgrounds/enpy/enpy-wallpaper.svg`.
- Fastfetch logo and preset at `/usr/share/fastfetch/`.
- XFCE desktop, Firefox ESR, networking tools, and the Debian Live installer are selected in the live-build config/package list.

The image is a Debian remaster, not an OS built from scratch: Debian supplies the Linux kernel, base system, package manager, drivers, and installer framework. This starter does not yet replace Debian installer artwork or provide a custom installer workflow. Fastfetch can be run with `fastfetch --config /usr/share/fastfetch/presets/enpy.jsonc`.

## Branding roadmap

1. Test the live ISO and installation in QEMU for every supported build host.
2. Add and test XFCE panel, theme, icons, login screen, and default wallpaper settings.
3. Add a custom boot menu and installer artwork using live-build-supported overlays/hooks.
4. Add a branded Plymouth boot splash and first-boot welcome flow.
5. Publish release metadata, checksums, source/build instructions, and update policy.

Do not ship an installer configuration that automatically partitions or formats disks. Any unattended install needs explicit user review and a disposable-VM test first.
