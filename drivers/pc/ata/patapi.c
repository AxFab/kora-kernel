#include "ata.h"
#include <kernel/arch.h>
#include <errno.h>

splock_t patapi_lock = INIT_SPLOCK;

static int ata_prepare_patapi_pio(ata_drive_t *drive, size_t lba, int count, int words, int cmd)
{
    outb(drive->base + ATA_REG_CONTROL, 0x0);

    // Setup SCSI Packet:
    uint8_t packet[12];
    packet[ 0] = cmd;
    packet[ 1] = 0x0;
    packet[ 2] = (lba >> 24) & 0xFF;
    packet[ 3] = (lba >> 16) & 0xFF;
    packet[ 4] = (lba >> 8) & 0xFF;
    packet[ 5] = (lba >> 0) & 0xFF;
    packet[ 6] = 0x0;
    packet[ 7] = 0x0;
    packet[ 8] = 0x0;
    packet[ 9] = count;
    packet[10] = 0x0;
    packet[11] = 0x0;

    // Select the drive
    outb(drive->base + ATA_REG_HDDEVSEL, (drive->slave ? 0xb0 : 0xa0));
    ata_wait(drive);

    // Inform the Controller
    outb(drive->base + ATA_REG_FEATURES, 0); // Use PIO mode
    // Sector size
    outb(drive->base + ATA_REG_LBA1, (words * 2) & 0xFF);
    outb(drive->base + ATA_REG_LBA2, (words * 2) >> 8);
    // Send command
    outb(drive->base + ATA_REG_COMMAND, ATA_CMD_PACKET);

    int err = ata_poll(drive, true);
    if (err != 0)
        return EIO;

    // Sending the packet data
    outsw(drive->base + ATA_REG_DATA, (uint16_t *)packet, 6);
    return 0;
}

int ata_read_patapi_pio(inode_t *ino, char *buf, size_t length, xoff_t offset, int flags)
{
    ata_drive_t *drive = ino->drv_data;
    size_t words = ino->dev->block / 2;
    size_t lba = offset / ino->dev->block;
    int count = length / ino->dev->block;

    splock_lock(&patapi_lock);
    int err = ata_prepare_patapi_pio(drive, lba, count, words, ATAPI_CMD_READ);
    if (err) {
        splock_unlock(&patapi_lock);
        return EIO;
    }

    // Receiving Data
    for (int i = 0; i < count; i++) {
        err = ata_poll(drive, true); // Or wait for an IRQ.
        if (err != 0) {
            // splock
            return EIO;
        }

        insw(drive->base + ATA_REG_DATA, (uint16_t *)&buf[ino->dev->block * i], words);
    }

    ata_poll(drive, false); // Or wait for an IRQ.
    splock_unlock(&patapi_lock);
    return 0;
}

int ata_write_patapi_pio(inode_t *ino, const char *buf, size_t length, xoff_t offset, int flags)
{
    return EROFS;
}

ino_ops_t ata_ops_patapi = {
    .read = ata_read_patapi_pio,
    .write = ata_write_patapi_pio,
    //    .ioctl = ata_ioctl,
};
