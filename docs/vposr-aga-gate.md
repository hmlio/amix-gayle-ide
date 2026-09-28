# Detecting AGA from VPOSR: use the low nibble, not `id >= 0x22`

Finding from the Amix Gayle IDE driver work (2026-09-27). Relevant to any Amix kernel
patch that gates hardware probes on the chipset, including the `support.c` patch in
`amix-a4091` (phantom A3000 SCSI removal).

VPOSR (`0xDFF004`) bits 14..8 carry the Agnus/Alice identification. The values as
implemented by WinUAE/Amiberry (`custom.cpp: VPOSR()`), which follow the hardware:

| Chip | PAL | NTSC |
|---|---|---|
| OCS Agnus | 0x00 | 0x10 |
| ECS Agnus (8372/8375) | 0x20 | 0x30 |
| AGA Alice | 0x23 (0x22 rev A) | 0x33 (0x32) |

Bit 4 (`0x10`) is the video standard, the low nibble identifies Alice. A test of the
form `id >= 0x22` therefore classifies an **NTSC ECS** machine (0x30) as AGA. In the
`amix-a4091` `support.c` patch that would skip the WD33C93 probe on an NTSC A3000 and
lose its internal SCSI; in an IDE probe it would touch `0xDD2020` on an NTSC A3000.

Robust test: `((vposr >> 8) & 0x0F) >= 2` → AGA. Verified against the emulator model on
the A3000 profile (reads 0x2000 / 0xA000). Real-hardware confirmation on the A4000
(expected 0x23 / 0xA300 for PAL) is pending.
