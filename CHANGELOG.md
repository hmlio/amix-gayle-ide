# Changelog

## 2026-09-28

- **First release** (private repo, shared with the amigaux.org maintainers). License decided: MIT (`LICENSE`). Repository shaped after the
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
