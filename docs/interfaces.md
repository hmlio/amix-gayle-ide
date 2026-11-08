# Interfaces: what other drivers and projects may rely on

This note lists what `amix-gayle-ide` provides to the rest of an Amix system and the rules other code must
follow when it shares hardware with it (Gayle interrupt registers, INT2). Everything here is taken from
`src/ide.c`, `src/kernel-patches/kernel.c.patch` and `driver.conf`; if the source and this note ever disagree,
the source wins.

## Disks

- The IDE master appears as **SCSI unit 6** (`ide_master_unit`), so a system installed on an A3000 (root at
  target 6) boots unchanged from IDE: root `c6d0s1`, swap `c6d0s2` (verified on a real A4000/030, 2026-09-27).
- The slave is unit 5 (`ide_slave_unit`) and is **not attached or addressed** unless `ide_slave_enable` is set
  (1 = any device, 2 = only an ATAPI device such as a CD-ROM). One exception: after a command timeout the
  driver issues a software reset (SRST), which by the ATA standard resets both devices on the cable.
- The driver registers with the stock `sd` layer as a pseudo SCSI host adapter (`driver.conf`: queue function
  `idequeue`, probe `idepresent`). RDB partitioning, `dd`/`sd` minors and raw devices work as for SCSI disks.

## Boards

| Board | Task file | INTRQ status (bit 7) | Gayle INTREQ ack | Gayle INTENA | Gayle ID | Chipset |
|---|---|---|---|---|---|---|
| A4000 | `0xDD2020` | `0xDD3020` | none | none | – | AGA |
| A1200 | `0xDA0000` | `0xDA9000` | `0xDA9000` | `0xDAA000` | `0xD1` | AGA |
| A600 | `0xDA0000` | `0xDA9000` | `0xDA9000` | `0xDAA000` | `0xD0` | ECS |

The Gayle ID is read bit-serially from `0xDE1000` (a write resets it, eight reads return the bits MSB first in
bit 7). Automatic detection finds **only the A4000** (AGA chipset + task-file readback at `0xDD2020`) and never
touches `0xDE1000`. An A1200 or A600 is selected with `ide_board_force` = 2 or 3; only then does the driver
read the Gayle ID, once at probe time, to confirm the board.

## Interrupts (INT2)

- With `kernel.c.patch` applied, `ideintr` is an entry in the kernel's `int2_tbl[]` (`master.d/kernel.c`).
  The kernel calls **every** entry on **every** level-2 interrupt. Without the patch the driver runs polled.
- `ideintr` returns at once unless its own INTRQ bit (bit 7 of the board's status register) is set. It never
  touches other interrupt sources.
- It runs at IPL 2; the request path (`idequeue`) holds `spl2()`, so the two never interleave.

### Rules for other drivers on A1200/A600 Gayle (e.g. a PCMCIA driver)

- **INTREQ `0xDA9000` is latched; writing 0 to a bit clears it, writing 1 leaves it.** The IDE driver
  acknowledges with `0x7C | (old & 0x03)`: bit 7 cleared, bits 2–6 written as 1 (PCMCIA sources untouched),
  bits 0–1 preserved. Another driver must do the same for its own bit: **write bit 7 as 1**, or it will drop
  a pending IDE interrupt.
- **INTENA `0xDAA000`:** the driver sets bit 7 by read-modify-write. Other drivers must also use
  read-modify-write and **keep bit 7 set**, or IDE falls back to watchdog timeouts.
- **Gayle ID `0xDE1000`:** reading it resets/advances the serial ID counter. Do not read it while another
  driver may be probing; reading it once at boot is fine. The IDE driver reads it only when
  `ide_board_force` selects the A1200 or A600.
- Your INT2 handler must also check its own source and return quietly otherwise; `ideintr` relies on the same.

## Tunables and counters (patchable/readable with `adb -k /stand/unix /dev/kmem`)

| Symbol | Default | Meaning |
|---|---|---|
| `ide_intr_enable` | 1 | 0 forces polled I/O even if hooked |
| `ide_board_force` | 0 | 0 auto-detect (A4000 only), 1 = A4000, 2 = A1200, 3 = A600, -1 = never probe |
| `ide_slave_enable` | 0 | 1 allows the slave (unit 5), 2 only if it is an ATAPI device |
| `ide_timeout_secs` | 20 | watchdog for an interrupt-driven command |
| `ide_debug` | 0 | 1 attach/errors, 2 every command |
| `ide_intr_count`, `ide_intr_spurious`, `ide_intr_timeouts` | – | statistics (read with `sym/D`) |
| `ide_resets`, `ide_dup_submits`, `ide_intr_busy`, `ide_pagecross_n` | – | resets after timeouts, ignored duplicate submits, interrupts while BSY, raw requests crossing a page |
| `ide_atapi_cmds`, `ide_atapi_overrun`, `ide_atapi_irlate` | – | ATAPI statistics |
