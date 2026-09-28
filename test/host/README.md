# Host test harness for `src/ide.c`

Compiles the **real** driver source with the host C compiler against a mock of the
A4000 Gayle IDE port and two ATA-2 drives, and checks the SCSI→ATA translation byte
for byte. No Amiga, no emulator, sub-second turnaround.

```sh
make -C test/host check     # build + run; exit 0 == every assertion passed
make -C test/host clean
```

## How the driver is compiled here

`ide.c` is K&R C and is built with `-std=gnu89` in a kernel-only include world:
`-Istubs` shadows `sys/types.h`, `sys/errno.h`, `sys/inline.h`, `rico.h` and `sd.h`
with minimal shims (the `struct sdcom` layout is the kernel ABI, copied field for
field). `-DHOST_TEST -include mock_regs.h` routes the driver's register seam
(`IDE_RB/IDE_WB/IDE_RW/IDE_WW/IDE_IRQSTAT/IDE_VPOSR`) into `mock_gayle.c`. Without
`HOST_TEST` the driver source is unchanged and the macros are plain volatile
dereferences of the identity-mapped addresses.

`kstubs.c` supplies `spl2()/splx()` (counted, so the harness can assert the
bracket is balanced) and a `printf()` that records the driver's console output.

## What the mock models

- Register map as defined at the top of `ide.c` (offsets relative to `0xDD2020`, control block at
  `0x101A`); unknown offsets abort the test.
- Two drives with an in-memory disk image, IDENTIFY DEVICE (model string, LBA
  capability, LBA28 sector count, FLUSH CACHE support), READ/WRITE SECTOR(S) with
  the DRQ-per-sector state machine, FLUSH CACHE, ABRT for anything else, IDNF for
  out-of-range LBAs, an injectable UNC at a chosen LBA, a "stuck BSY" knob, an
  absent master (open bus, `0xFF`) and an absent slave.
- **Byte order:** a 16-bit DATA read returns `(stream[0] << 8) | stream[1]` where
  `stream` is the byte sequence the drive presents in little-endian word order, the
  same model WinUAE/Amiberry use for Gayle. On the 68k that puts sector bytes in
  natural order and IDENTIFY words byte-swapped (which the driver undoes once).
  On a little-endian host the driver's integer word stores land the two bytes
  swapped, so `disk_equals()` in `ide_test.c` compares against a pair-swapped
  image there. The driver source is identical in both worlds.
- `VPOSR` (chipset id) so the probe's AGA gate can be tested on an "A3000".

## Gating assertions (`ide_test.c`)

- Probe: on an ECS chipset no IDE register is touched; on AGA without a drive the
  probe reports absent without issuing a command; with a drive it reports the base,
  disables drive interrupts, is cached, never selects the slave.
- Attach: one IDENTIFY per unit, model string in order, sector count decoded.
- INQUIRY / READ CAPACITY(10) / MODE SENSE(6) shapes, `nbyte` respected.
- READ(10) single sector, 16 sectors (a UFS block), short `nbyte` (drive drained),
  300 sectors (split into two ATA commands), READ(6); WRITE(10) and read-back.
- Errors: LBA out of range (rejected before the drive), UNC, unknown opcode,
  stuck BSY (bounded spin, no hang) → CHECK CONDITION and the expected
  REQUEST SENSE key/ASC, consumed on read; `okay == TRUE` for all of them.
- Slave: never selected while `ide_slave_enable == 0`; works when enabled; an
  enabled but absent slave does not disturb the master.
- Completion: delivered exactly once per request, iteratively (a request issued from
  inside the completion does not nest), spl bracket balanced, FIFO never overflows.
- CHS-only drives are refused with a console message.
