# Aleste LX — A Virtual 8-Bit Platform and BIOS Development Kit

**Aleste LX** is a design-first 8-bit computer platform. It does not yet exist as physical hardware — and that is intentional. The project is built around a simple observation:

> You cannot boot a machine without firmware, and you cannot develop firmware for a machine that does not yet exist.

Aleste LX breaks this deadlock by providing a **virtual hardware platform, BIOS, driver model, and debugger**, all integrated into a self-contained emulator. You can design the hardware, declare drivers in assembly, build a ROM image, and run it — before flashing a single FPGA.

Aleste LX is a successor to the original *Aleste EX* architecture. It retains what worked while addressing several of the original design's shortcomings: a cleaner memory map, a proper driver dispatch mechanism, well-defined error handling, and a stable ABI for applications.

---

## Why This Project Might Be Useful to You

This repository is published in an unfinished, frozen state. Its primary value lies in the **architecture and development workflow**, rather than in a complete feature set.

It demonstrates a practical approach to developing software for hardware that does not yet exist — or for hardware that lacks a usable emulator.

You may find it useful if you want to:

- Write Z80 software for a computer that has no existing emulator.
- Prototype a retro-style FPGA-based system and develop its firmware before the hardware is ready.
- Design a BIOS and driver model with a stable call table and test it entirely in software.
- Study an example of **hardware abstraction implemented through virtualization**: the emulator does more than execute Z80 instructions; it also implements the video modes, sound interfaces, and memory mapping planned for the eventual hardware.

In short, Aleste LX is a **forkable foundation for bootstrapping your own platform**.

---

## The Core Idea

The project is built around three layers developed in parallel.

### 1. Virtual Hardware

The emulator lives in `emulator/`. It contains a Z80 CPU core, provided by the bundled `z80ex` dependency, along with virtual implementations of the Aleste LX peripherals:

- CRTC
- Palette
- Video buffer
- MMU
- AY-8910 audio
- Debugger overlay

### 2. BIOS and Drivers

The BIOS and drivers are written in Z80 assembly and live under `bios/`, `drivers/`, and `fonts/`.

Drivers are registered in a **driver table** with a defined method layout. Applications can invoke driver functionality through a stable interface rather than relying on hard-coded hardware addresses.

### 3. Applications

Applications can be written in C or assembly. See `main.c` for an example.

They access the BIOS interface through the headers and definitions provided in `include/` and `inc/`.

### The Include Directory Split

The project separates public interfaces from internal implementation details:

- `include/` — public C headers for application developers.
- `inc/` — corresponding assembly includes, constants, and equates.
- `drivers/` — internal driver implementations, built into the BIOS image.

This separation keeps the application-facing ABI independent of driver implementation details.

---

## Repository Layout

```text
aleste-xi-bios/
├── asm/                 CRT0 and linker sections for the BIOS image
├── bios/                BIOS entry points and error handling
├── drivers/             Driver implementations (assembly)
│   ├── audio/           AY-8910 driver
│   ├── crtc/            CRTC driver and video modes
│   ├── mmu/             Memory management unit driver
│   ├── palette/         12-bit palette driver
│   ├── vbuf/            Video buffer driver
│   └── video/           Video composition
├── fonts/               Bitmap fonts (Windows-1252)
├── inc/                 Assembly includes (bios/, drivers/, hardware/)
├── include/             C headers (bios/, drivers/, hardware/)
├── src/drivers/         C-side driver wrappers and stubs
├── services/            Higher-level services (console, filesystem, players) — planned
├── emulator/            Virtual platform and debugger
│   ├── src/             CPU, MMU, CRTC, palette, ports, screen, debugger
│   └── test/            Minimal test ROM and build script
├── deps/z80ex/          Bundled Z80 CPU emulation library
├── output/aleste.bin    Assembled 64 KB BIOS image
└── main.c               Example application
```

---

## Installation (Ubuntu)

The toolchain requires a **native Z88DK build with the SDCC compiler engine enabled (`BUILD_SDCC=1`)**, rather than a prebuilt package that lacks the required compiler.

The following steps describe the intended setup.

### 1. Install Prerequisites

```bash
sudo apt update
sudo apt install -y bison flex subversion
sudo apt install -y libsdl2-dev libreadline-dev libgmp-dev libboost-graph-dev
```

### 2. Build Z88DK from Source

Download and build Z88DK with the SDCC compiler engine enabled:

```bash
cd ~
wget https://github.com/z88dk/z88dk/releases/download/v2.4/z88dk-src-2.4.tgz
tar -xvzf z88dk-src-2.4.tgz
cd z88dk
make BUILD_SDCC=1
sudo make install
```

The `BUILD_SDCC=1` option is required because the Aleste BIOS build depends on the SDCC-based `zsdcc` compiler. A default Z88DK build may not provide this compiler.

### 3. Configure the Environment

Set the environment variables for the current shell session:

```bash
export Z88DK_PATH="$HOME/z88dk"
export PATH="$Z88DK_PATH/bin:$PATH"
export ZCCCFG="$Z88DK_PATH/lib/config/"
```

To persist these settings across terminal sessions, add them to `~/.bashrc`:

```bash
echo 'export Z88DK_PATH="$HOME/z88dk"' >> ~/.bashrc
echo 'export PATH="$Z88DK_PATH/bin:$PATH"' >> ~/.bashrc
echo 'export ZCCCFG="$Z88DK_PATH/lib/config/"' >> ~/.bashrc
```

Reload the configuration or open a new terminal session to apply the changes.

---

## Building

### 1. Build the Emulator Dependency and Binary

First, build the bundled Z80 CPU emulation dependency (`z80ex`), then compile the emulator.

Replace the example project paths below with the actual location of your checkout.

```bash
# Build the local Z80 CPU emulation dependency
cd ~/projects/hww/_z80/aleste-xi-bios/deps/z80ex
make

# Build the emulator
cd ~/projects/hww/_z80/aleste-xi-bios/emulator
make clean && make
```

### 2. Build the Aleste BIOS Image

Navigate to the project root and ensure that the project's `Makefile` points to your local Z88DK installation.

```bash
cd ~/projects/hww/_z80/aleste-xi-bios

# Update Z88DK_PATH in the Makefile to match your installation.
# For example: Z88DK_PATH = /home/your-user/z88dk

# Clean and rebuild the BIOS image
make clean
make
```

The resulting BIOS image is written to `output/aleste.bin`. The expected image size is 64 KB.

---

## Running

Launch the standalone emulator and pass it the assembled BIOS ROM:

```bash
cd ~/projects/hww/_z80/aleste-xi-bios
./bin/z80_emu output/aleste.bin
```

Alternatively, use the emulator's built-in test target:

```bash
cd ~/projects/hww/_z80/aleste-xi-bios/emulator
make run
```

---

## The Driver Model

Drivers are exposed through a **call table**, rather than through a collection of fixed entry-point addresses.

Each driver has a struct-like method layout defined in `inc/drivers/*.inc` and `include/drivers/*.h`. The BIOS dispatches calls through this interface, allowing applications to use driver functionality without accessing hardware ports directly.

Driver methods return error codes defined in `inc/bios/errors.inc`, such as `ERR_SUCCESS`. Unknown methods are routed to `bios/undefined_method.asm`, allowing unsupported calls to fail predictably instead of producing undefined behavior.

This approach provides three important benefits for an 8-bit development platform:

- **Stable ABI** — applications compile against public headers rather than hard-coded addresses.
- **Testability** — drivers can be replaced with alternative implementations or stubs in the emulator.
- **Portability** — application code can potentially run on both the emulator and the eventual FPGA hardware, provided that the driver table and underlying interfaces remain compatible.

---

## The Emulator as a Design Tool

The emulator in `emulator/` is not intended to be a generic Z80 machine. It is a **virtual implementation of Aleste LX**.

It provides:

- The video modes exposed by the CRTC driver.
- The 12-bit palette.
- The video buffer layout planned for the future FPGA implementation.
- An **in-emulator debugger** whose interface is rendered over the emulated screen, allowing you to debug the machine from within its own environment.

The emulator therefore serves as a reference implementation for the planned hardware.

One of the project's goals is to make it possible to boot the same BIOS image on the eventual FPGA implementation without modification. Achieving that will depend on the hardware matching the interfaces and behavior defined by the emulator.

---

## Project Status

**Aleste LX is unfinished and no longer actively developed.**

The following components are available:

- The emulator builds and runs a Z80 image.
- The bundled Z80 CPU emulation dependency builds.
- The BIOS and driver source trees, along with their C and assembly interfaces, are in place.
- The BIOS source assembles into `output/aleste.bin`.

The following areas remain incomplete:

- Some drivers, including `video` and `storage`, are stubs; parts of `services/` are planned or unfinished.
- The debugger is functional but does not implement all intended features.
- The build configuration assumes a specific local Z88DK path, which must be adjusted in the `Makefile`.

The main reason to explore or fork this repository is its **architecture and development workflow**, not its completeness as a finished product.

---

## Forking Aleste LX for Your Own Platform

If you want to use Aleste LX as a starting point for your own machine, the recommended approach is:

1. **Define the memory map and hardware ports** in `inc/hardware/` and `include/hardware/`.
2. **Specify the driver interfaces** in `inc/drivers/` and `include/drivers/`, including method layouts and error codes.
3. **Implement the drivers** in `drivers/` using assembly and the conventions established in `inc/bios/drivers.inc`.
4. **Implement the virtual hardware** in `emulator/src/`, including ports, MMU, video, and audio, so the emulator behaves like the intended hardware.
5. **Write applications** against the public interfaces in `include/`. Once the emulator supports the required functionality and the hardware implements compatible interfaces, the same application code can be used on the physical platform.

You do not need an FPGA to get started. You need a contract — the driver interface — and a reference implementation — the emulator.

That is the foundation Aleste LX provides.

---

## License

See [`LICENSE`](LICENSE) for licensing information.