# Security policy

A keyboard sees everything a person types, so security reports have top priority.

## Reporting a vulnerability

**Do not open a public issue.** Report it privately through
[GitHub security advisories](https://github.com/promahbubul/july_bangla_keyboard/security/advisories/new).
Include:

- the affected version and platform
- the steps to reproduce
- the impact, as you understand it

We aim to acknowledge reports within 7 days and to publish a fix and an advisory as soon
as possible after that. We credit reporters unless they prefer not to be named.

## Supported versions

Only the latest release receives security fixes.

## What we promise

- No network access, telemetry, analytics or ads in the keyboard.
- No global keyboard hooks, no injected keystrokes, and no persistence beyond what is
  documented.
- The Android app requests no permissions.

The design and threat model are in [docs/SECURITY.md](docs/SECURITY.md).
