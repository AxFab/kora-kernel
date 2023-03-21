#include "ata.h"
#include <kernel/arch.h>
#include <errno.h>

static void ata_prepare_pata_pio(ata_drive_t *drive, size_t lba, int count, bool wait)
{
    // Select mode from LBA28, LBA48 or CHS
    uint8_t lba_io[6] = { 0 };
    int head, cyl, sect;

    // Wait for BSY to be cleared
    while (inb(drive->base + ATA_REG_STATUS) & ATA_STATUS_BSY);


    if (drive->mode == ATA_MODE_LBA48) {
        lba_io[0] = (lba >> 0) & 0xff;
        lba_io[1] = (lba >> 8) & 0xff;
        lba_io[2] = (lba >> 16) & 0xff;
        lba_io[3] = (lba >> 24) & 0xff;
        outb(drive->base + ATA_REG_HDDEVSEL, 0xE0 | (drive->slave ? 0xb0 : 0xa0));

    } else if (drive->mode == ATA_MODE_LBA28) {
        lba_io[0] = (lba >> 0) & 0xff;
        lba_io[1] = (lba >> 8) & 0xff;
        lba_io[2] = (lba >> 16) & 0xff;
        head = (lba >> 24) & 0x0f;
        outb(drive->base + ATA_REG_HDDEVSEL, (drive->slave ? 0xb0 : 0xa0) | head);

    } else if (drive->mode == ATA_MODE_CHS) {
        // Head number is written to HDDEVSEL lower 4-bits.
        sect = (lba % 63) + 1;
        cyl = (lba + 1  - sect) / (16 * 63);
        head = (lba + 1  - sect) % (16 * 63) / (63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        outb(drive->base + ATA_REG_HDDEVSEL, (drive->slave ? 0xb0 : 0xa0) | head);
    }

    if (wait)
        ata_wait(drive);

    // Write Parameters
    if (drive->mode == ATA_MODE_LBA48) {
        outb(drive->base + ATA_REG_SECCOUNT1, (count >> 8) & 0xff);
        outb(drive->base + ATA_REG_LBA3,   lba_io[3]);
        outb(drive->base + ATA_REG_LBA4,   lba_io[4]);
        outb(drive->base + ATA_REG_LBA5,   lba_io[5]);
    }
    outb(drive->base + ATA_REG_SECCOUNT0, count & 0xff);
    outb(drive->base + ATA_REG_LBA0, lba_io[0]);
    outb(drive->base + ATA_REG_LBA1, lba_io[1]);
    outb(drive->base + ATA_REG_LBA2, lba_io[2]);

    if (wait)
        ata_wait(drive);

    /* Send read command */
    int cmd = drive->mode == ATA_MODE_LBA48 ? ATA_CMD_READ_PIO_EXT : ATA_CMD_READ_PIO;
    outb(drive->base + ATA_REG_COMMAND, cmd);

    if (wait)
        ata_wait(drive);
}

int ata_read_pata_pio(inode_t *ino, char *buf, size_t length, xoff_t offset, int flags)
{
    ata_drive_t *drive = ino->drv_data;
    int lba = offset / 512;
    int count = length / 512;
    for (int r = 0; r < 3; ++r) {
        // Grab IDE mutex ?
        ata_prepare_pata_pio(drive, lba, count, r != 0);

        // Do reading
        for (int i = 0; i < count; i++) {
            int err = ata_poll(drive, true);
            if (err != 0) {
                for (int j =0; j < 5; ++j)
                    ata_wait(drive);
                continue;
            }

            insw(drive->base + ATA_REG_DATA, (uint16_t *)&buf[i * 512], 256);
        }

        // Release IDE mutex ?
        return 0;
    }

    // Release IDE mutex ?
    return EIO;
}

int ata_write_pata_pio(inode_t *ino, const char *buf, size_t length, xoff_t offset, int flags)
{
    ata_drive_t *drive = ino->drv_data;
    int cmd = drive->mode == ATA_MODE_LBA48 ? ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH;
    int lba = offset / 512;
    int count = length / 512;
    for (int r = 0; r < 2; ++r) {
        // Grab IDE mutex ?
        ata_prepare_pata_pio(drive, lba, count, r != 0);

        // Do writing
        for (int i = 0; i < count; i++) {
            int err = ata_poll(drive, true);
            if (err != 0) {
                outb(drive->base + ATA_REG_COMMAND, cmd);
                ata_poll(drive, false);
                for (int j =0; j < 5; ++j)
                    ata_wait(drive);
                continue;
            }

            outsw(drive->base + ATA_REG_DATA, (uint16_t *)&buf[i * 512], 256);
        }

        outb(drive->base + ATA_REG_COMMAND, cmd);
        ata_poll(drive, false);
        // Release IDE mutex ?
        return 0;
    }

    // Release IDE mutex ?
    return EIO;
}

ino_ops_t ata_ops_pata = {
    .read = ata_read_pata_pio,
    .write = ata_write_pata_pio,
    //    .ioctl = ata_ioctl,
};
