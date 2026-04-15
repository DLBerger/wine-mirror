# RFC: Win16 Support in Wine (wow16loader)

**Status:** Draft / Prototype  
**Date:** 2026-04-15  
**Author:** DLBerger  
**Target branch:** master

---

## 1. Problem Statement and Motivation

Wine has a long history of Win16 support rooted in the 32-bit x86 era, when the host CPU
could enter VM86 mode and execute 16-bit segmented code natively.  Modern 64-bit-only
Linux/macOS hosts have lost that safety net: the VM86 syscall is gone on x86-64 kernels,
and non-x86 architectures never had it.

At the same time there is a real, ongoing need to run 16-bit Windows applications:

* **Legacy business / industrial software** originally shipped for Windows 3.1 and never
  updated, still in active use on "frozen" Windows XP SP3 workstations.
* **Educational / historical computing** – running software from the early 1990s for
  research, preservation, or teaching.
* **Wine regression / conformance** – Wine's own test suite exercises Win16 behaviour;
  keeping that exercisable cross-platform is valuable.

Projects such as [otvdm](https://github.com/otya128/winevdm) (and its derivative
**DLBerger/otvdm**) demonstrate that a working Win16 personality can be built on top of
Win32/Win64 host APIs.  However otvdm re-implements large parts of what Wine already
provides (loader, thunking, USER/GDI dispatch, wineserver IPC patterns, etc.), leading to
duplicated effort and divergent quality.

This RFC proposes a path to add **first-class, maintained Win16 support to Wine itself**,
reusing existing Wine components wherever possible, following Wine coding conventions, and
buildable on every platform Wine supports.

---

## 2. Goals

* **G1 – Run Win 3.1 NE executables** under Wine on 64-bit Linux/macOS/ARM.
* **G2 – Run typical Win16 applications** that were historically used on Windows XP SP3
  (16-bit helpers, installers, legacy tools).
* **G3 – Reuse existing Wine components** – loader, wineserver, ntdll, USER16/GDI16,
  krnl386, thunking infrastructure – rather than re-implementing them.
* **G4 – Cross-platform** – buildable wherever Wine builds (Linux x86-64, aarch64, macOS
  x86-64/arm64).
* **G5 – Wine coding conventions** – LGPL licence, winebuild `.spec` files, Makefile.in
  build system, Wine test framework.
* **G6 – Non-invasive** – the feature is optional; existing Wine builds are unaffected
  when it is not enabled.
* **G7 – Future-extensible** – design must not block broader Win16 support, additional
  emulation backends, or Win95/Win98 16-bit compatibility work.

---

## 3. Non-Goals

* Full DOS/DPMI emulation (already handled by DOSBox integration in `programs/winevdm`).
* Porting large code blobs from otvdm wholesale.
* Win16 network stack or protected-mode 16-bit device drivers.
* Any change to Wine's existing Win32/Win64 execution paths.
* Shipping a redistributable binary blob of a CPU emulator.

---

## 4. Compatibility Targets

| Target | Format | Typical CPU requirement | Example apps |
|--------|--------|------------------------|--------------|
| Windows 3.1 GUI apps | NE (.exe/.dll) | 80286 real/protected mode | Cardfile, Solitaire, Write |
| Windows 3.11 for Workgroups | NE | 80386 | Most Win3.x shareware |
| Win16 apps on Windows XP SP3 | NE (running under NTVDM) | 80386 | Legacy industrial HMI, CAD viewers, tax software |
| Win16 installers | NE | 80286/386 | InstallShield 1.x, SETUP.EXE patterns |

The primary acceptance criterion for each milestone is: **the application launches, presents
its main window (or completes its task), and exits without crashing Wine**.

---

## 5. Survey of Existing Wine Win16 Capabilities

Wine already contains substantial Win16 infrastructure that this proposal intends to reuse:

### 5.1 krnl386.exe16

`dlls/krnl386.exe16/` is the 16-bit kernel emulation layer.  It contains:

* **NE loader** (`ne_module.c`, `ne_segment.c`) – parses NE headers, builds the module
  database, resolves imports, performs segment fixups.
* **Global/local heap** (`global.c`, `local.c`) – the 16-bit heap manager operating on
  the LDT-based selector model.
* **Task management** (`task.c`) – 16-bit task table, task switching hooks.
* **Thunking** (`thunk.c`, `wowthunk.c`, `utthunk.c`) – 16→32 and 32→16 call frames,
  WOWCallback16Ex.
* **Interrupt emulation** (`interrupts.c`, `int21.c`, …) – DOS/BIOS interrupt stubs.
* **Selector management** (`selector.c`) – LDT manipulation via the kernel.

### 5.2 Win16 DLLs

`dlls/` contains Win16 variants of many system libraries:

| DLL | Purpose |
|-----|---------|
| `user.exe16` | Win16 USER – window management, message loop |
| `gdi.exe16` | Win16 GDI – drawing primitives |
| `kernel.exe16` (krnl386 alias) | Memory, file, module management |
| `commdlg.dll16` | Common dialogs |
| `shell.dll16` | Shell APIs |
| `ver.dll16` | Version information |
| `toolhelp.dll16` | Task/module enumeration |
| `wow32.dll` | WOW thunking from 16-bit into 32-bit |

### 5.3 programs/winevdm

`programs/winevdm/` is a 32-bit launcher that can invoke `krnl386.exe16` to run DOS and
simple Win16 executables.  It is the natural home for a Win16-specific loader front-end.

### 5.4 tools/winebuild

`winebuild` generates NE-format stubs for Wine's 16-bit built-in DLLs, demonstrating that
the build toolchain already understands NE format.

### 5.5 Existing gaps (64-bit hosts)

* `WINE_VM86_*` / VM86 mode syscalls are absent on 64-bit kernels → 16-bit code cannot
  execute natively.
* `krnl386.exe16` currently requires a 32-bit Wine build; it does not load under a 64-bit
  Wine without WoW64 support.
* The LDT is not fully accessible from 64-bit user space on all OSes.

---

## 6. Survey of otvdm Approach (Conceptual)

otvdm (and DLBerger/otvdm) takes the following approach at a conceptual level – **no code
is being imported**:

1. **CPU emulation backend** – a software emulator (based on a modified NTVDM/x86
   emulator) interprets 8086/286/386 instructions in a tight loop.  The emulator exposes a
   flat memory model that the Win16 runtime maps segment:offset pairs onto.

2. **Win16 personality layer** – WIN16 API calls are intercepted at the INT or far-call
   boundary and dispatched to equivalent Win32 APIs on the host, with argument conversion
   (handle mapping, pointer fixups, string encoding).

3. **NE loader** – loads NE segments into the emulator's flat address space and fixes up
   relocations.

4. **Message loop bridge** – a special thread runs the 16-bit message pump and forwards
   WM_* messages to the 32-bit window created for the emulated app.

**Key insight from otvdm:** the hard part is not the CPU emulator itself but the *Win16
API personality* – correctly mapping thousands of 16-bit API call semantics to 32-bit
equivalents, especially around the message loop, GDI coordinate system, handle lifetimes,
and task model.

**Where otvdm diverges from Wine:** it reimplements the WIN16 API layer from scratch on
top of raw Win32 rather than leveraging Wine's existing krnl386/user16/gdi16 chain.  This
RFC proposes using Wine's existing layer instead.

---

## 7. Proposed Architecture Options

### Option A – "Embedded emulator, thunk into existing Wine Win16 layer" (Recommended)

```
NE binary
    │
    ▼
wow16loader (new program)
    │  loads NE, sets up address space
    │
    ▼
x86 emulator library (libx86emu / similar, as optional dep)
    │  interprets 16-bit code
    │
    ▼
krnl386.exe16 / user.exe16 / gdi.exe16   ← existing Wine 16-bit DLLs
    │  existing 16→32 thunk layer
    │
    ▼
Wine 32-bit / 64-bit layer (ntdll, win32u, wineserver, …)
```

**How it works:**  
`wow16loader` is a new Wine program (`programs/wow16loader/`) that:
1. Parses the NE header to determine the entry point and import table.
2. Maps NE segments into a memory region managed by an embedded x86 emulator.
3. Starts the emulator, letting it execute 16-bit code.
4. When the 16-bit code issues a far call to a system DLL entry point, the emulator's
   "undefined opcode / INT" intercept fires, and `wow16loader` routes the call through
   Wine's existing `krnl386`/`user.exe16` ABI (already compiled as 32-bit built-ins).

**Pros:**
* Reuses krnl386/user16/gdi16 – maximum leverage of existing, tested Wine code.
* CPU emulator is an isolated library dependency – can be swapped or improved.
* Follows the Wine model of layered DLL dispatch.
* Separates "how to run 16-bit CPU instructions" from "what Win16 API calls do".

**Cons:**
* Requires a suitable x86 emulator that can be used as a library (e.g., libx86emu,
  unicorn, or a vendored minimal core).
* Performance will be limited to emulator speed for CPU-bound code.
* Dependency on an optional external library – needs careful build system integration.

---

### Option B – "WoW64-style: run 32-bit Wine in a 32-bit subprocess"

Wine already supports running 32-bit processes via its WoW64 mechanism on 64-bit hosts.
The existing `krnl386.exe16` and associated Win16 DLLs work under a 32-bit Wine build.

**How it works:**  
On hosts that support it (Linux x86-64 with 32-bit multilib), arrange for a 32-bit Wine
wine-preloader to handle NE binaries, using `krnl386` directly (as in Wine 5.x behaviour).

**Pros:**
* Near-zero new code – reuses the existing 32-bit Wine Win16 path.
* Very high compatibility with the existing krnl386 code.

**Cons:**
* Requires 32-bit multilib support on the host – not available on aarch64, macOS arm64,
  or stripped-down 64-bit Linux installations.
* Not cross-platform in the G4 sense.
* Does not address VM86 gap on 64-bit kernels for actual 16-bit code execution.

---

### Option C – "NE-to-PE binary translation (static lift)"

Parse the NE binary, identify code segments, and statically translate 16-bit x86
instructions to 32/64-bit equivalents, generating a PE image that Wine's normal loader
can execute.

**How it works:**  
A translation tool converts the NE binary at install/run time into a temporary PE.

**Pros:**
* No CPU emulator dependency.
* Translated code runs at native speed.
* Works on non-x86 architectures natively.

**Cons:**
* Fundamental correctness problem: self-modifying code, computed jumps, segment arithmetic,
  and undocumented 16-bit behaviours are very hard to translate statically.
* The API shim layer still needs to exist (same problem as Options A/B).
* Likely to stall at moderate complexity apps.

---

### Recommendation

**Start with Option A.** It provides the best balance of:
* Leveraging existing Wine infrastructure (krnl386, user16, gdi16).
* Cross-platform reach (any host where the emulator library builds).
* Incremental deliverability (stub loader → minimal CPU → more API coverage).
* Future-proofing (emulator backend can be swapped; translation could be layered later).

Option B can be activated opportunistically on x86 Linux hosts as a fast path when 32-bit
Wine is available.  Option C is deferred as a long-term research direction.

---

## 8. Proposed Component Boundaries and Naming

```
programs/wow16loader/          – New Wine program (launcher + NE parser + emulator glue)
    Makefile.in
    README.md                  – Points to this RFC; marks directory as stub
    wow16loader.c              – (stub) main entry point
    wow16loader.spec           – (stub) winebuild spec

dlls/wow16support/             – (future) helper library shared between wow16loader and
                                  existing krnl386 thunking; currently empty
    Makefile.in
    README.md

dlls/krnl386.exe16/            – Existing; extend where needed (64-bit clean-ups,
                                  emulator callback hooks)

dlls/user.exe16/               – Existing; expose hooks for emulator interrupt intercept
dlls/gdi.exe16/                – Existing; no changes planned initially
```

**Naming rationale:**
* `wow16loader` mirrors Wine's `winevdm` naming pattern (functional description, not a
  Windows internal name).
* Placing new code under `programs/` follows the pattern of `winevdm`, `winedbg`, etc.
* `wow16support` is reserved in `dlls/` for future shared logic; not wired into the build
  until meaningful code exists.

---

## 9. Build System Considerations

### 9.1 Wine build system (autotools + Makefile.in)

* New directories follow the existing pattern: a `Makefile.in` in each subdirectory,
  referenced from the top-level `configure.ac` / `Makefile.in`.
* The component is **optional**: add a `--enable-wow16` / `--disable-wow16` autoconf
  option (defaulting to `auto`, which enables if the emulator library is detected).
* No changes to existing `Makefile.in` files until the component is ready to build.

### 9.2 External emulator dependency

Candidate libraries (evaluated for licence, portability, and API surface):
* **libx86emu** – MIT licence, small, C, no external deps.
* **Unicorn** – LGPL, multi-arch, Python-friendly, slightly heavier.
* **QEMU TCG as library** – powerful but very heavy; likely overkill for stage 1.

The dependency should be:
* Optional (detected by `configure` via `pkg-config`).
* Vendorable (small enough to ship as a `libs/` subtree if needed, like Wine's own
  `libs/port/`).

### 9.3 MSVC friendliness

Wine is primarily a GCC/Clang codebase.  MSVC support is partial.  For `wow16loader`:
* Use only C99/C11 constructs found elsewhere in Wine (no GCC extensions beyond those
  already used in `krnl386`).
* Avoid `__attribute__((packed))` in favour of `#pragma pack` (already used in
  `winevdm.c`).
* Mark any assembly stubs as `#ifdef __GNUC__` with MSVC stubs returning `FIXME`.
* The emulator library dependency may be the limiting factor for MSVC; document clearly.

---

## 10. Testing Strategy

### 10.1 Wine test framework

All new code follows Wine's existing test conventions:
* Unit tests live alongside the component in a `tests/` subdirectory.
* Tests use `wine/test.h` macros (`ok`, `todo_wine`, `skip`, etc.).
* Tests are submitted as part of each feature PR.

### 10.2 NE loader tests

Priority tests for the initial milestone:
* Parse a known NE header and verify field extraction (module name, segment count, entry
  table).
* Verify that a minimal NE binary with no code (just a module header) loads without error.
* Verify correct handling of malformed NE headers (truncated, wrong magic).

### 10.3 Sample application smoke tests

The following classes of applications are target smoke-test candidates:

| Category | Example |
|----------|---------|
| Win 3.1 built-in apps | `notepad.exe` (16-bit original), `calc.exe` |
| Win 3.1 SDK demos | "Hello World" NE binary built with MASM/C 7 |
| Simple Win16 shareware | Small freeware utilities from the era |
| Win16 installer | InstallShield 1.x style SETUP.EXE |
| XP SP3 16-bit helper | Embedded 16-bit SETUP stub from legacy MSI |

Smoke-test criterion: **application reaches main window / completes task / exits with
expected exit code**.

### 10.4 Regression baseline

Before wiring `wow16loader` into the build, run `make test` against the existing Win16
test suite (`dlls/krnl386.exe16/tests/`) to establish a baseline.  No regressions are
acceptable in that suite.

---

## 11. Security Considerations

16-bit code emulation introduces a meaningful attack surface:

### 11.1 Memory isolation

* The emulator runs in the same process as the Wine loader but operates on a dedicated
  memory arena.  The NE segment table must be validated before mapping.
* Segment sizes and offsets must be bounds-checked against the file size before use to
  prevent heap overflows in the loader.

### 11.2 Privilege separation

* `wow16loader` is a normal Wine user-space process; it does not require elevated
  privileges.
* The emulator should not be permitted to issue system calls directly; all OS interaction
  must flow through the Wine thunk chain.

### 11.3 Denial of service

* Infinite loops in emulated code are mitigated by: instruction count limits (configurable
  via `WINEDEBUG`), or running the emulator on a dedicated thread with a watchdog.
* Large NE files or pathological relocation tables must be rejected gracefully.

### 11.4 API surface reduction

* Only Win16 APIs required by the target application set are enabled; all others return
  `ERROR_CALL_NOT_IMPLEMENTED`.
* Existing Wine security hardening (ASLR, stack canaries, etc.) applies to the loader
  process itself.

---

## 12. Staged Implementation Plan

### Milestone 0 – Scaffolding (this PR)

* [x] Add `programs/wow16loader/README.md` (stub, points here).
* [x] Add `programs/wow16loader/Makefile.in` (stub, not wired into build).
* [x] Add `documentation/rfc-win16-support.md` (this document).

### Milestone 1 – NE Parser

* [ ] Implement NE header / segment table / import name table parser in
      `programs/wow16loader/ne_parse.c`.
* [ ] Add unit tests for the parser.
* [ ] Wire `wow16loader` into the build system as an optional target
      (`--enable-wow16`).

### Milestone 2 – Emulator Integration

* [ ] Select and integrate an x86 emulator library (libx86emu or Unicorn).
* [ ] Implement address-space setup: map NE segments into emulator flat memory.
* [ ] Stub out INT/far-call intercepts → return `ERROR_CALL_NOT_IMPLEMENTED`.
* [ ] Run a trivial NE binary (no API calls) to completion.

### Milestone 3 – Minimal Win16 API Bridge

* [ ] Route WIN16 API far calls to `krnl386.exe16` / `user.exe16` thunk chain.
* [ ] Implement handle translation table (HWND16 ↔ HWND32, etc.).
* [ ] Run "Hello World" NE binary (MessageBox16).

### Milestone 4 – Basic GUI App Support

* [ ] Full message loop bridge.
* [ ] GDI primitives (TextOut, DrawText, basic painting).
* [ ] Common dialogs.
* [ ] Run Win 3.1 Notepad and Calculator without crashes.

### Milestone 5 – Broader Compatibility

* [ ] OLE/DDE basics.
* [ ] File I/O (OpenFile16, _lread16, …).
* [ ] Resource loading (LoadBitmap16, LoadIcon16, …).
* [ ] Run target XP SP3 legacy applications.

### Milestone 6 – Upstreaming Preparation

* [ ] Code review against Wine coding conventions.
* [ ] Test suite coverage ≥ 80% of implemented APIs.
* [ ] CI integration.
* [ ] Submit patches to wine-devel mailing list.

---

## 13. Open Questions for Maintainers

1. **Emulator library policy:** Is Wine willing to accept an optional dependency on an
   external x86 emulator (e.g., libx86emu)?  Alternatively, is a minimal vendored CPU
   core under `libs/` acceptable?

2. **krnl386.exe16 on 64-bit:** What is the long-term plan for krnl386 on 64-bit Wine?
   Should `wow16loader` extend krnl386 or maintain a parallel Win16 personality?

3. **LDT access on 64-bit Linux:** The LDT is still accessible via `modify_ldt(2)` on
   Linux x86-64 for user-space code.  Is it acceptable to rely on this for segment
   simulation, or should the design avoid LDT use entirely?

4. **Process model:** Should each Win16 task run as a separate Wine process (like modern
   winevdm/winedbg), or should multiple Win16 tasks share a process (matching original
   Windows 3.1 cooperative multitasking)?

5. **WoW64 interaction:** On x86-64 Linux with 32-bit multilib, should `wow16loader`
   detect that 32-bit Wine is available and delegate to the existing krnl386 path
   (Option B as a fast path)?

6. **Naming:** The Wine project may have preferences for naming (`wow16loader` vs.
   `ntvdm16` vs. `win16host`); what is preferred?

7. **Licence for emulator backend:** Unicorn is LGPL; libx86emu is MIT.  Does Wine's
   licence policy accept either?

8. **Build system:** Should the optional component use autoconf `--enable-wow16` or a
   separate `configure` sub-invocation (like `wine64` / `wine32` split today)?

---

*End of RFC draft.*
