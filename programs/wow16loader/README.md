# wow16loader — STUB (not yet wired into build)

This directory is a **placeholder** for the `wow16loader` component proposed in the
project RFC:

> [`documentation/rfc-win16-support.md`](../../documentation/rfc-win16-support.md)

## What is wow16loader?

`wow16loader` will be an optional Wine program that provides Win16 (Windows 3.1 / NE
format) application support on modern 64-bit hosts, leveraging Wine's existing
`krnl386.exe16`, `user.exe16`, and `gdi.exe16` layers rather than re-implementing them.

## Current status

**Milestone 0 – Scaffolding only.**  No functional code exists yet.

See the RFC document for the full design, proposed architecture, build system notes,
testing strategy, and staged implementation plan.

## How to help

Read the RFC, then discuss on the Wine mailing list (`wine-devel@winehq.org`) or open an
issue on this mirror repository.  Patches must follow Wine coding conventions.

## Why is this not wired into the build?

The `Makefile.in` in this directory is intentionally left as a minimal stub and is **not**
referenced from the top-level `Makefile.in` or `configure.ac`.  It will be wired in once
Milestone 1 (NE parser) is complete and ready for review.
