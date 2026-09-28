# amix-gayle-ide — native Amix IDE driver for the Amiga 4000's onboard Gayle port

A native **Amiga UNIX (Amix 2.1, Commodore's SVR4.0 port to the 68030 Amiga)** driver for the
**Gayle IDE controller** built into the Amiga 4000 (and, through a board table, the A1200 and A600),
so that Amix runs on an **unmodified A4000 with a 68030 CPU card** — no SCSI host adapter needed,
root on a CF card. Amix shipped with no IDE driver at all; this repo closes that gap. It follows
the framework, build environment and gotcha catalog of [`amix-a4091`](https://github.com/jusii/amix-a4091)
and [`amix-z3660scsi`](https://github.com/jusii/amix-z3660scsi).

The driver is a **pseudo SCSI host adapter**: the stock Amix SCSI disk layer (`dd.c`, block major 18)
hands it SCSI CDBs, and it executes them as ATA-2 PIO commands on the Gayle task file. Everything
above — RDB partitioning, `c<N>d<U>s<P>` slices, `mkfs`/`fsck`, the installer — is untouched.
Design record: [`docs/decision-hba-vs-block.md`](docs/decision-hba-vs-block.md).

## Scope / responsibility

This repo is **the Amix IDE driver for the Gayle port, and nothing else.** Its siblings in the
[amigaux.org](https://amigaux.org/) ecosystem:

- **Build harness** (kernel assembly, `driver.conf` discovery, clean-gate, host↔box bridge) →
  [`amix-kerntools`](https://github.com/jusii/amix-kerntools)
- **A4091 SCSI driver**, origin of the `support.c` phantom-A3000-SCSI patch →
  [`amix-a4091`](https://github.com/jusii/amix-a4091)
- **Documentation** of the Amix kernel and its quirks → [`grimoire-amix`](https://github.com/jusii/grimoire-amix)

Compilation and kernel integration go through `amix-kerntools`, which reads this repo's
[`driver.conf`](driver.conf) by relative path. This repo carries only the driver source, its
`driver.conf`, the kernel patches, a host-side test harness and the design docs.

## Status (2026-09)

**✅ Boots Amix with root on IDE on a real Amiga 4000/030** (2026-09-27): Commodore A4000/030 card
(68030 + MMU, 68882), Kickstart 3.1, SanDisk 1 GB CF on a 44-pin adapter. Kickstart autoboots the
`UNIX_Boot` partition, the kernel mounts root `c6d0s1` and swap `c6d0s2` through this driver, root
login works. To my knowledge the first Amix boot on an A4000 with root on IDE.

- ✅ **Probe** `idepresent()` — AGA chipset gate via VPOSR, then a task-file write/readback; silent on
  an A3000 without touching the Gayle addresses.
- ✅ **Command execution** `idequeue()` — READ/WRITE(6)/(10), REQUEST SENSE, INQUIRY, TEST UNIT READY,
  READ CAPACITY(10), MODE SENSE(6), START/STOP UNIT, SYNCHRONIZE CACHE → ATA READ/WRITE SECTOR(S),
  IDENTIFY DEVICE, FLUSH CACHE, with synthesised sense data. PIO only, LBA28.
- ✅ **Interrupt-driven I/O** `ideintr()` — level-2 handler hooked into `int2_tbl[]` by
  `kernel.c.patch`, one interrupt per sector, `timeout()` watchdog; falls back to polled I/O on an
  unpatched kernel. Emulation: 107k interrupts, 2 spurious, 0 timeouts; 10 MB raw read 3.4 s → 0.7 s sys.
- ✅ **Native build** — compiles with the Amix K&R `cc` without diagnostics, links `checkunix`-clean
  through amix-kerntools, boots on an A3000 profile (probe silent) and the Amiberry A4000/AGA profile.
- ✅ **Host harness** — the unmodified driver source against a mock Gayle/ATA model, 439 checks.
- 🟡 **Board table** — A4000 entry proven; A1200/A600 entries (task file `0xDA0000`, latched INTREQ,
  Gayle ID register) follow NetBSD/Amiberry and are untested. Amix itself has further A1200/A600
  blockers (fast RAM expected at `0x07000000`).
- 🟡 **Open on real hardware** — stress and power-cycle `fsck` runs, spin-count calibration of the
  bounded polled waits, the installer route. The **IDE slave** stays disabled unless `ide_slave_enable`
  is set.

The IDE master is presented as **SCSI unit 6** (`ide_master_unit`) so that the stock root configuration
`c6d0s1`, `/etc/vfstab` and the installer's assumptions of an A3000-installed system apply unchanged.
`support.c.patch` removes the stock kernel's phantom A3000 SCSI controller on AGA machines, which makes
the IDE port **card 0**.

## Layout

```
driver.conf               0xFFFF1DE0 idequeue "A4000 IDE" ide.c probe=idepresent
src/ide.c                 the driver: register map, board table, probe, SCSI→ATA translation,
                          level-2 handler (K&R C, single file as amix-kerntools expects)
src/kernel-patches/       kernel.c.patch  -- ideintr into int2_tbl[] (master.d/kernel.c)
                          support.c.patch -- phantom A3000 SCSI removal (amiga/kernel/support.c),
                                             derived from amix-a4091, AGA gate corrected
                          NOTICE          -- diffs only, not Amix source
test/host/                host harness: mock Gayle/ATA, kernel stubs, tests (make check)
docs/                     decision record, VPOSR AGA-gate finding
CHANGELOG.md
```

The `sd.c` `scsicard[]` rows and the alien `Makefile` `OBJ` entry are **not** here — amix-kerntools
generates them from `driver.conf`.

## Building

This repo is **source only**. With [`amix-kerntools`](https://github.com/jusii/amix-kerntools)
checked out alongside it and an Amix 2.1 box reachable (real hardware or an
Amiberry/WinUAE profile), the single entry point is:

```sh
(cd ../amix-kerntools && ./amix-build gayle-ide --install)
```

The harness splices `src/ide.c` into `/usr/sys/amiga/alien/`, generates the controller rows from
`driver.conf`, applies the two kernel patches with `patch -N`, relinks and clean-gates the kernel.
When combining with `amix-a4091` in one kernel, apply only **one** of the two `support.c` patches
(ours differs from upstream's only in the AGA test; see `docs/vposr-aga-gate.md`).

On an A4000 the resulting kernel boots straight from the CF card: Kickstart 3.1 autoboots the
`UNIX_Boot` partition of the IDE disk, and the kernel mounts root over this driver. No SCSI card,
no `unix_boot` patches (68030 = stock `unix_boot` 1.1c).

The host harness needs only a C compiler:

```sh
make -C test/host check      # 439 checks, exit 0 == all pass
```

## Hardware notes

- Gayle task file at `0xDD2020` (A4000/A4000T), byte registers spaced by 4, control block at `+0x101A`;
  IRQ mirror at `0xDD3020` bit 7. INT2 is shared with CIA-A, so the handler checks that bit and
  returns "not mine" otherwise. No explicit interrupt acknowledge on the A4000 (reading STATUS drops it).
- `0xDD2020` lies in the identity-mapped low 1 GB (`phystokv(p) == p`), so no `sptalloc()`; the 68030
  cache inhibit comes from the transparent-translation register, not from a per-page bit.
- A 16-bit DATA read delivers ATA words with the bytes in natural order on the 68k, so sector data is
  copied as is and only IDENTIFY words are byte-swapped — confirmed on the real machine.
- The clock is a level-2 interrupt and `sdspl == spl2`, so `lbolt` is frozen inside the queue routine:
  polled waits are bounded spin counts, the interrupt path uses a `timeout()` watchdog.

## License

The original work in this repo is released under the **MIT license** (see [`LICENSE`](LICENSE)):
[`src/ide.c`](src/ide.c), [`driver.conf`](driver.conf), the test harness and this repository's
documentation.

The patches in [`src/kernel-patches/`](src/kernel-patches/) are unified diffs against stock Amix
files: the added lines are MIT, the few quoted stock context lines remain under the original
Commodore SVR4 copyright. `support.c.patch` is derived from `amix-a4091` (MIT, Jussi Alanärä). See
[`src/kernel-patches/NOTICE`](src/kernel-patches/NOTICE).

Amix itself (SVR4.0), its disk images, Kickstart ROMs and manuals are proprietary Commodore software
and are **not** included — you need your own Amix 2.1 system. Nothing from Commodore/AT&T or from
GPL code is in this repo: the ATA behaviour follows the ATA-2 standard, and NetBSD's `wdc`/`gayle`
sources (BSD) and Linux `gayle.c` (GPL) were read as documentation only.

## Credits

By Emre Bastuz (hmlio). AI tooling — **Claude Code** — was used throughout to investigate, build and verify this
work. Built on the framework, build harness and kernel documentation of the amigaux.org community
(Jussi Alanärä, asokero, isoriano1968) and the NetBSD `wdc` driver as the ATA reference.
