#include "ata.h"

/*
 * Low-level I/O Helper Functions
 * 
 * In x86, CPU communicates with hardware controllers (like ATA hard disk controller)
 * through dedicated I/O ports. We use inline assembly to execute the CPU instructions 
 * 'in' and 'out' to read and write bytes (8-bit) and words (16-bit) to/from these ports.
 */

// Read a single byte (8-bit) from an I/O port
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Write a single byte (8-bit) to an I/O port
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Read a single word (16-bit) from an I/O port (used to stream data from disk)
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Write a single word (16-bit) to an I/O port (used to stream data to disk)
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__("outw %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * ATA PIO Controller Registers:
 * - 0x1F0: Data Port (Read/Write data)
 * - 0x1F1: Error / Features Register
 * - 0x1F2: Sector Count Register (Number of sectors to read/write)
 * - 0x1F3: Sector Number / LBA Lo (Bits 0-7 of sector address)
 * - 0x1F4: Cylinder Low / LBA Mid (Bits 8-15 of sector address)
 * - 0x1F5: Cylinder High / LBA High (Bits 16-23 of sector address)
 * - 0x1F6: Drive / Head Register (Bits 24-27 of sector address + Master/Slave select)
 * - 0x1F7: Status Register (Read) / Command Register (Write)
 */

void ata_read_sector(uint32_t lba, uint16_t *buf) {
    // 1. Select the Master Drive on Primary IDE Bus & send bits 24-27 of the LBA address
    // 0xE0 represents: 11100000 binary. Bit 6 (0x40) enables LBA mode, Bit 4 (0x10) selects Master drive.
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    // 2. Specify the number of sectors we want to read (1 sector = 512 bytes)
    outb(0x1F2, 1);

    // 3. Send LBA address bits 0-7, 8-15, and 16-23 to their respective registers
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));

    // 4. Send Command 0x20 (Read Sectors with retry) to the Command Register
    outb(0x1F7, 0x20);

    // 5. Poll the status register (0x1F7) until the disk controller is ready.
    // - BSY (Busy, bit 7) must be cleared (0)
    // - DRQ (Data Request, bit 3) must be set (1), signaling the controller has read sector data into its internal buffer
    while ((inb(0x1F7) & 0x80) != 0); // Loop while Busy
    while ((inb(0x1F7) & 0x08) == 0); // Loop until Data Request is set

    // 6. Read 256 words (512 bytes total) from the Data Port (0x1F0) into our buffer
    for (int i = 0; i < 256; i++) {
        buf[i] = inw(0x1F0);
    }
}

void ata_write_sector(uint32_t lba, uint16_t *buf) {
    // 1. Select the Master Drive and send bits 24-27 of LBA
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    // 2. Specify we want to write 1 sector (512 bytes)
    outb(0x1F2, 1);

    // 3. Send LBA address bits 0-7, 8-15, and 16-23
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));

    // 4. Send Command 0x30 (Write Sectors with retry) to the Command Register
    outb(0x1F7, 0x30);

    // 5. Poll the status register until the drive is ready to receive data.
    // - BSY (Busy, bit 7) must be 0
    // - DRQ (Data Request, bit 3) must be 1, signaling the drive's buffer is ready to receive our data
    while ((inb(0x1F7) & 0x80) != 0);
    while ((inb(0x1F7) & 0x08) == 0);

    // 6. Write 256 words (512 bytes total) to the Data Port (0x1F0) from our buffer
    for (int i = 0; i < 256; i++) {
        outw(0x1F0, buf[i]);
    }

    // 7. Send Command 0xE7 (Cache Flush) to tell the controller to commit the write from internal cache to physical disk.
    outb(0x1F7, 0xE7);

    // Wait for BSY to clear, confirming the write has successfully flushed to disk.
    while ((inb(0x1F7) & 0x80) != 0);
}
