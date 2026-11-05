# ENPY OS

An original, open-source desktop OS project built on a Linux foundation. ENPY aims to provide a clear, accessible desktop experience with its own identity—not a copy of Windows or another proprietary OS.

> **Status:** Early prototype. This repository is not yet a bootable operating system. The `ui/` directory is a browser-based interface prototype.

## Prototype
Open `ui/index.html` in a browser to try the login-to-desktop flow. The demo has no real authentication and must not be used to protect data or accounts.

## Project roadmap
1. Validate the original ENPY desktop experience with the web prototype.
2. Select a Linux base and desktop technology (for example, a Wayland shell using Qt/QML).
3. Implement a real session/login integration using existing Linux components rather than custom password handling.
4. Add settings, file manager integration, accessibility, localization, and application discovery.
5. Create reproducible builds and an installer only after the desktop is functional and tested.

## Contributing
See [`docs/architecture.md`](docs/architecture.md) for the initial architecture and roadmap. Contributions should preserve user privacy, accessibility, security, and the project's original visual identity.

## License
MIT. See [`LICENSE`](LICENSE).
