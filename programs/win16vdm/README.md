# win16vdm — STUB (not yet wired into build)

This directory is a **placeholder** for the `win16vdm` component proposed in the
project RFC:

> [`documentation/rfc-win16-support.md`](../../documentation/rfc-win16-support.md)

## What is win16vdm?

`win16vdm` is planned as an optional Wine helper process for Win16 (Windows 3.1 / NE
format) applications. The current prototype direction is one `win16vdm` process per
Win16 app, with per-app manifest discovery from the executable directory:

* Manifest path: `<exe-name>.win16vdm.xml` next to the Win16 executable.
* Default shared cache: `%LOCALAPPDATA%\\win16vdm\\cache`.
* Manifest may override the cache root to a fixed directory.

## Current status

**Milestone 0 – Scaffolding only.**  No functional code exists yet.

See the RFC document for the full design, proposed architecture, build system notes,
testing strategy, and staged implementation plan.

## How to help

Read the RFC, then discuss on the Wine mailing list (`wine-devel@winehq.org`) or open an
issue on this mirror repository.  Patches must follow Wine coding conventions.

## Why is this not wired into the build?

The `Makefile.in` in this directory is intentionally left as a minimal stub and is **not**
referenced from the top-level `Makefile.in` or `configure.ac`.  It will only be wired in
once the prototype is ready for functional implementation review.
