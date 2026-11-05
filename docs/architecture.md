# ENPY OS Architecture

## Vision and scope
ENPY OS is intended to become an original Linux-based desktop distribution and experience. This repository currently contains a UI prototype and planning documents; it is **not** a complete or bootable OS.

## Proposed layers
- **Base system:** a maintained Linux distribution and kernel; avoid reinventing kernel, hardware, and security infrastructure.
- **Display/session:** a Wayland compositor and session manager selected after a technical prototype.
- **Shell:** original ENPY desktop shell (panel, launcher, workspaces, notifications, and window interactions).
- **System applications:** settings and file browsing, integrating mature Linux components where appropriate.
- **Apps and updates:** use established package formats and signed repositories; do not invent an unsafe privileged installer.
- **Installer/image:** reproducible image build pipeline, added after hardware and upgrade testing.

## Prototype boundaries
`ui/` is a static HTML/CSS/JavaScript mockup. The sign-in screen is a visual demo: it has no authentication, credential storage, or OS session integration. Never enter a real password into it. The app tiles are illustrative and do not launch native applications.

## Security principles
- Delegate authentication to the chosen Linux session stack; never store user passwords in the shell.
- Keep privileged operations in trusted system services with least privilege.
- Prefer signed updates and reproducible builds.
- Document data collection; default to minimal telemetry (ideally none).
- Test accessibility, localization, updates, and recovery paths.

## Milestones
1. Improve the interactive prototype and define user journeys.
2. Choose a base distribution, toolkit, and license strategy for dependencies.
3. Build a minimal shell proof-of-concept on a development VM.
4. Integrate existing login/session and system components.
5. Add CI, security review, hardware testing, and installation documentation.
6. Publish an experimental image only when it can be safely installed and updated.
