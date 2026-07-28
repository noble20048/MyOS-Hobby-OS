#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// Reads a 512-byte sector from the hard drive at the specified LBA address into the buffer.
void ata_read_sector(uint32_t lba, uint16_t *buf);

// Writes a 512-byte sector to the hard drive at the specified LBA address from the buffer.
void ata_write_sector(uint32_t lba, uint16_t *buf);

#endif
