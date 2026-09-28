# Decision: implement the A4000 IDE driver as a pseudo SCSI host adapter (Option A)

Date: 2026-09-27. Basis: reading the Amix 2.1 kernel relink kit (`/usr/sys`, from the install
tape) and the amigaux.org drivers/build tooling. No Commodore/AT&T code is reproduced here; the facts
below are interface facts also documented publicly by the upstream projects.

## Why A
1. **The SCSI stack is small and the HBA contract is narrow.** The only SCSI disk driver (block major 18)
   issues exactly three commands to a host adapter: READ(10), WRITE(10) and REQUEST SENSE. The request
   record carries a physical buffer address, byte count, card/unit and a completion callback; the HBA
   reports `okay` + SCSI status. Everything above it — RDB partitioning, slices `c<N>d0s<P>`, raw and
   block devices, `mkfs`/`fsck`, the installer's disk tools — keeps working unchanged.
2. **Registration of a fixed-address controller is a solved problem.** `amix-kerntools` generates the
   host-adapter table from `driver.conf` and supports a `probe=` function for boards that are not
   AutoConfig (used by the Z3660 SCSI driver). The upstream A4091 `support.c` patch removes the
   "phantom A3000 SCSI" on AGA machines, so on an A4000 the IDE controller becomes **card 0** and the
   IDE master is `c0d0` — a root-device configuration (`c0s1unix`) that the stock tree already ships.
3. **Interrupt and memory primitives exist and are simple.** Level-2 handlers are called from a
   NULL-terminated table for every INT2 (the dispatcher clears the PORTS bit, handlers check their own
   device). Kernel virtual == physical for RAM, so a PIO driver copies straight to/from the physical
   address it is given; no page mapping, no cache maintenance.
4. **Worked examples to mirror**: the stock A3000 driver (queue per unit, interrupt state machine,
   lazy `initialize()`), upstream `amix-a4091` and `amix-z3660scsi` (polled/synchronous completion,
   probe function, host-side unit tests with a stubbed `sd.h`).

## Why not B (native block driver with its own major)
It would need its own `bdevsw` entry and device nodes, its own partition code (or a re-use of the RDB
code through its strategy), its own raw-I/O breakup, and changes to installer and admin tools that
assume SCSI-style `c<N>d0s<P>` devices — all to avoid a SCSI→ATA translation of three commands. There is
no performance argument either: both paths end in PIO word copies.

## Consequences / design constraints
- Driver exports `idequeue(con, cp)` and `ideintr()`, plus `idepresent(&base)` for `driver.conf`.
- ATA translation: READ/WRITE(10) → LBA28 READ/WRITE SECTORS (multi-sector, 256 max per command,
  split as needed); REQUEST SENSE → synthesised sense from the last ATA error; INQUIRY, TEST UNIT READY,
  READ CAPACITY(10), MODE SENSE(6), START/STOP, SYNCHRONIZE CACHE answered locally (needed by the
  `GSIO` passthrough used by userland tools); everything else → CHECK CONDITION / ILLEGAL REQUEST.
- Units: unit 0 = master, unit 1 = slave (slave disabled by default — FlashAir safety rule).
- Completion model: start with synchronous polled completion inside `idequeue` (like the Z3660 driver);
  move to INT2-driven completion in Phase 2.
- Multi-page requests: same physical-contiguity assumption as the stock DMA host adapters; add a
  `cmn_err` guard if a request crosses a 2048-byte page in a way we cannot serve.
- Probe safety: gate on the AGA chipset (VPOSR) before touching 0xDD2020, then a register readback
  test; never register on an A3000.
