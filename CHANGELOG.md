# Changelog

## 2026-11-08

- **PRs #2–#7 merged** (Jusii, tested on an A4000 with a Z3660 in 68060 mode).
  - **Probe (#2):** automatic detection is A4000-only; the Gayle ID register `0xDE1000` is read only when
    `ide_board_force` selects the A1200 (2) or A600 (3). An empty port no longer spins the reset budget.
  - **Request queue (#3):** duplicate submits of an active `sdcom` are ignored; SYNCHRONIZE CACHE is queued
    behind transfers in interrupt mode; a timed-out command resets the drive (SRST).
  - **SCSI command fixes (#4):** VERIFY(10) runs READ VERIFY SECTOR(S); DF is a hardware error; LBA range
    check no longer wraps; LUN ≠ 0 rejected; a full completion FIFO delivers instead of dropping.
  - **Docs (#5):** README and design record aligned with the driver; VPOSR id 0x23 confirmed on a real PAL A4000.
  - **68040/68060 build (#6):** `-DIDE_CACHE=1 -DIDE_NBPP=4096` adds cache push/invalidate around PIO and a
    DTT check of the register window; that line is polled only.
  - **ATAPI CD-ROM (#7):** PACKET passthrough; `ide_slave_enable 2` attaches the slave only if it is ATAPI.
- `docs/interfaces.md` added: what other drivers may rely on (units, board table, Gayle INTREQ/INTENA sharing
  rules on the A1200/A600, tunables and counters).
- Host harness: the `printf` stub in `test/host/kstubs.c` is now non-variadic, matching the driver's
  `extern int printf();`; on arm64 (Apple Silicon) the variadic stub read garbage and crashed.

## 2026-10-13

- **`support.c.patch` dropped** (PR #1, Jusii). The phantom-A3000-SCSI removal now comes only from
  `amix-a4091`, which the amix-kerntools hub applies to every kernel; building by hand needs that
  copy. `kernel.c.patch` hunk header regenerated (applies at zero fuzz, content unchanged).
- Open: `amix-a4091`'s copy still uses the `& 0x7F) < 0x22` AGA test, which misclassifies NTSC ECS
  (`docs/vposr-aga-gate.md`). A4000 is unaffected; reported to the maintainer.

## 2026-09-28

- **First release.** License: MIT (`LICENSE`). Repository shaped after the
  amix-kerntools driver-author contract (`driver.conf`, `src/`, `src/kernel-patches/` + `NOTICE`,
  `docs/`, `test/host/`).

## 2026-09-27

- **Real hardware: Amix boots from IDE on an Amiga 4000/030** (Commodore A4000/030 card, 68882,
  Kickstart 3.1, SanDisk 1 GB CF on a 44-pin adapter). Kickstart autoboots the `UNIX_Boot` partition,
  the kernel mounts root `c6d0s1` and swap `c6d0s2` through this driver, root login works. IDENTIFY
  model string and sector count decode correctly, confirming the DATA-register byte-order model.
- **Phase 2 — interrupts and board table.** `ideintr()` registered in `int2_tbl[]` via
  `kernel.c.patch`; one interrupt per sector, `timeout()` watchdog, polled fallback when the hook is
  absent. `struct ideboard` confines every board-specific address and interrupt convention (A4000
  entry active; A1200/A600 entries prepared). In the Amiberry A4000/AGA profile: 107k interrupts,
  2 spurious, 0 timeouts; sys time for a 10 MB raw read 3.4 s → 0.7 s. Host harness 439 checks.
- **Phase 1 — polled PIO pseudo SCSI host adapter.** `idepresent()` probe (AGA gate via VPOSR, task-file
  write/readback), `idequeue()` translating READ/WRITE(6)/(10), REQUEST SENSE, INQUIRY, TEST UNIT READY,
  READ CAPACITY(10), MODE SENSE(6), START/STOP, SYNCHRONIZE CACHE to ATA-2 commands with synthesised
  sense data. Compiled by the native Amix `cc` without diagnostics on the first attempt; linked through
  amix-kerntools; boots on an A3000 profile with the probe correctly silent; root on IDE boots in the
  Amiberry A4000/AGA profile. IDE master presented as SCSI unit 6 so a stock A3000-installed image
  boots unchanged.
- `support.c.patch` carried with the AGA gate corrected to the low nibble of the VPOSR id
  (`docs/vposr-aga-gate.md`): `id >= 0x22` misclassifies NTSC ECS machines (0x30) as AGA.
- Architecture decided: pseudo SCSI host adapter (Option A), `docs/decision-hba-vs-block.md`.
