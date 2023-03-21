/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#include "ata.h"
#include <kernel/irq.h>
#include <kernel/mods.h>
#include <kernel/bus/pci.h>
#include <string.h>
#include <errno.h>


ata_drive_t drives[] = {
    { .id = 0, .base = 0x1f0, .ctrl = 0x3f6, .name = "sda", .slave = false },
    { .id = 1, .base = 0x1f0, .ctrl = 0x3f6, .name = "sdb", .slave = true },
    { .id = 2, .base = 0x170, .ctrl = 0x376, .name = "sdc", .slave = false },
    { .id = 3, .base = 0x170, .ctrl = 0x376, .name = "sdd", .slave = true },
};



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

const char *ata_errors[] = {
    "No address mark",
    "Track 0 not found",
    "Command aborted",
    "Media change request",
    "ID mark not found",
    "Media changed",
    "Uncorrectable data",
    "Bad block"
};

static const char *ata_error_string(uint8_t err)
{
    if (!POW2(err) || err == 0)
        return "Unknown";
    int bits = 0;
    while ((err & 1) == 0) {
        bits++;
        err >>= 1;
    }
    return ata_errors[bits];
}

void ata_wait(ata_drive_t *drive);
int ata_poll(ata_drive_t *drive, bool check);
void ata_soft_reset(ata_drive_t *drive);
void ata_probe();

void ata_wait(ata_drive_t *drive)
{
    for (int i = 0; i < 4; ++i)
        inb(drive->base + ATA_REG_ALTSTATUS);
}

int ata_poll(ata_drive_t *drive, bool check)
{
    // Delay 400 nanosecond for BSY to be set
    ata_wait(drive);

    // Wait for BSY to be cleared:
    while (inb(drive->base + ATA_REG_STATUS) & ATA_STATUS_BSY);

    if (!check)
        return 0;

    uint8_t state = inb(drive->base + ATA_REG_STATUS);
    if (state & ATA_STATUS_ERR) {
        ata_wait(drive);
        uint8_t err = inb(drive->base + ATA_REG_ERROR);
        kprintf(-1, "ata: drive error: %d:%s\n", err, ata_error_string(err));
        return -err;
    } else if (state & ATA_STATUS_DF) {
        ata_wait(drive);
        uint8_t err = inb(drive->base + ATA_REG_ERROR);
        kprintf(-1, "ata: drive fault: %d:%s\n", err, ata_error_string(err));
        return -err;
    } else if (!(state & ATA_STATUS_DRQ)) {
        kprintf(-1, "ata: drive error: DRQ not set\n");
        return -1;
    }

    return 0;
}

void ata_soft_reset(ata_drive_t *drive)
{
    outb(drive->ctrl, ATA_CMD_RESET);
    ata_wait(drive);
    outb(drive->ctrl, 0);
}

// static void ata_select_drive(ata_drive_t *drive)
// {
//     static struct ata_drive *last_drive = NULL;
//     static int last_mode = -1;

//     int mode = drive->slave ? 0xb0 : 0xa0;
//     if (drive == last_drive && mode == last_mode)
//         return;

//     outb(drive->base + ATA_REG_HDDEVSEL, mode);
//     ata_wait(drive);
//     last_drive = drive;
//     last_mode = mode;
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

extern ino_ops_t ata_ops_pata;
extern ino_ops_t ata_ops_patapi;

static void ata_detect_drive(ata_drive_t *drive)
{
    // if (!drive->slave) soft_reset + wait
    outb(drive->base + ATA_REG_HDDEVSEL, drive->slave ? 0xb0 : 0xa0);
    // ata_select_drive(drive);
    ata_poll(drive, false);

    int k, status = inb(drive->base + ATA_REG_STATUS);
    if (status == 0)
        return;

    uint8_t mode_lo = inb(drive->base + ATA_REG_LBA1);
    uint8_t mode_hi = inb(drive->base + ATA_REG_LBA2);
    uint16_t mode = (mode_hi << 8) | mode_lo;

    switch (mode) {
    case ATADEV_PATA:
        drive->id_command = ATA_CMD_IDENTIFY;
        drive->block = 512;
        drive->class = "IDE PATA";
        drive->ops = &ata_ops_pata;
        break;
    case ATADEV_SATA:
        drive->id_command = ATA_CMD_IDENTIFY;
        drive->block = 512;
        drive->class = "IDE SATA";
        drive->ops = &ata_ops_pata; // TODO
        break;
    case ATADEV_PATAPI:
        drive->id_command = ATA_CMD_IDENTIFY_PACKET;
        drive->block = 512;
        drive->class = "IDE PATAPI";
        drive->ops = &ata_ops_patapi;
        break;
    case ATADEV_SATAPI:
        drive->id_command = ATA_CMD_IDENTIFY_PACKET;
        drive->block = 2048;
        drive->class = "IDE SATAPI";
        drive->ops = &ata_ops_patapi; // TODO
        break;
    default:
        kprintf(-1, "ata: Unknown IDE drive type %4x\n", mode);
        return;
    }

    outb(drive->base + ATA_REG_COMMAND, drive->id_command);
    ata_poll(drive, true);

    // Read Identification Space of the Device:
    uint8_t ptr[512];
    insl(drive->base + ATA_REG_DATA, (uint32_t *)ptr, 128);

    // Read Device Parameters:
    drive->signature = *((uint16_t *)(ptr + ATA_IDENT_DEVICETYPE));
    drive->capabilities = *((uint16_t *)(ptr + ATA_IDENT_CAPABILITIES));
    drive->command_sets = *((uint32_t *)(ptr + ATA_IDENT_COMMANDSETS));
    if (drive->command_sets & ATA_CAP_LBA48) {
        /* 48-bit LBA */
        uint32_t high = *(uint32_t *) (ptr + ATA_IDENT_MAX_LBA);
        uint32_t low = *(uint32_t *) (ptr + ATA_IDENT_MAX_LBA_EXT);
        drive->max_lba = ((uint64_t) high << 32) | low;
        drive->mode = ATA_MODE_LBA48;
    } else if (drive->command_sets & ATA_CAP_LBA) {
        /* 28-bit LBA */
        drive->max_lba = *(uint32_t *) (ptr + ATA_IDENT_MAX_LBA);
        drive->mode = ATA_MODE_LBA28;
    } else {
        // TODO -- find number of head !?
        drive->max_lba = 0;
        drive->mode = ATA_MODE_CHS;
    }

    // String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
    for (k = 0; k < 40; k += 2) {
        drive->model[k] = ptr[ATA_IDENT_MODEL + k + 1];
        drive->model[k + 1] = ptr[ATA_IDENT_MODEL + k];
    }
    k = 39;
    while (drive->model[k] == ' ' || drive->model[k] == -1)
        drive->model[k--] = '\0';
    drive->model[40] = '\0';

    ata_soft_reset(drive);
    // printk("ata%d: signature: 0x%x\n", drive->id, drive->signature);
    // printk("ata%d: capabilities: 0x%x\n", drive->id, drive->capabilities);
    // printk("ata%d: command Sets: 0x%x\n", drive->id, drive->command_sets);
    // printk("ata%d: max LBA: 0x%x\n", drive->id, drive->max_lba);
    // printk("ata%d: model: %s\n", drive->id, drive->model);

    // Create and register block inode
    inode_t *blk = vfs_inode(drive->id, FL_BLK, NULL, drive->ops);
    blk->length = (xoff_t)drive->max_lba * drive->block;
    blk->lba = drive->id;
    blk->dev->block = drive->block;
    blk->dev->flags = FD_RDONLY;
    blk->dev->model = strdup(drive->model);
    blk->dev->devclass = strdup(drive->class);
    blk->drv_data = drive;
    vfs_mkdev(blk, drive->name);
    vfs_close_inode(blk);
}

void ata_probe()
{
    // pci_scan_device(0x01, 0x01, &ide, 1);
    // class_id >> 8 = 0x0101;
    struct PCI_device *ide = NULL;
    // pci_device_t *ide = NULL;
    if (ide != NULL) {
        if (ide->bar[0].base != 0 && ide->bar[0].base != 1) {
            drives[0].base = ide->bar[0].base;
            drives[1].base = ide->bar[0].base;
        }
        if (ide->bar[1].base != 0) {
            drives[0].ctrl = ide->bar[1].base;
            drives[1].ctrl = ide->bar[1].base;
        }
        if (ide->bar[2].base != 0) {
            drives[2].base = ide->bar[2].base;
            drives[3].base = ide->bar[2].base;
        }
        if (ide->bar[3].base != 0) {
            drives[2].ctrl = ide->bar[3].base;
            drives[3].ctrl = ide->bar[3].base;
        }
    }

    for (int i = 0; i < 4; ++i)
        ata_detect_drive(&drives[i]);
}

void ata_irq_handler(void *data)
{
    // TODO -- use parameter to know wich bus
    // TODO -- Handle query
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define IDE_NODISC     0x00
#define IDE_ATA        0x01
#define IDE_ATAPI      0x02

#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

// Channels:
#define      ATA_PRIMARY      0x00
#define      ATA_SECONDARY    0x01

// Directions:
#define      ATA_READ      0x00
#define      ATA_WRITE     0x01
#define      ATA_DMA       0x02



#define ATA_DELAY do {\
      inb(dr->pbase_ + ATA_REG_ALTSTATUS);  \
      inb(dr->pbase_ + ATA_REG_ALTSTATUS);  \
      inb(dr->pbase_ + ATA_REG_ALTSTATUS);  \
      inb(dr->pbase_ + ATA_REG_ALTSTATUS);  \
    } while (0)


struct ATA_Drive {
    uint8_t   type_; // ATA
    uint16_t  pbase_; // 0x1f0 - 0x170
    uint16_t  pctrl_; // 0x3f6 - 0x376
    uint8_t   disc_;  // 0xa0 - 0xb0
    uint16_t signature_;
    uint16_t capabilities_;
    uint32_t commandsets_;
    uint32_t size_;
    char     model_[44];
};

struct ATA_Drive sdx[4];

const char *sdNames[] = { "sda", "sdb", "sdc", "sdd" };



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


static void ATAPI_Detect(struct ATA_Drive *dr)
{
    uint8_t type_lo = inb(dr->pbase_ + ATA_REG_LBA1);
    uint8_t type_hi = inb(dr->pbase_ + ATA_REG_LBA2);
    uint16_t type    = (type_hi << 8) | type_lo;

    if (type != ATADEV_PATAPI && type != ATADEV_SATAPI)
        return;

    dr->type_ = IDE_ATAPI;
    outb(dr->pbase_ + ATA_REG_COMMAND, (uint8_t)ATA_CMD_IDENTIFY_PACKET);
    ATA_DELAY;
}


static int ATA_Detect(struct ATA_Drive *dr)
{
    int res, k;
    uint8_t ptr[2048];
    // Select Drive:
    outb(dr->pbase_ + ATA_REG_HDDEVSEL, (uint8_t)dr->disc_);
    ATA_DELAY;
    // Send ATA Identify Command:
    outb(dr->pbase_ + ATA_REG_COMMAND, (uint8_t)ATA_CMD_IDENTIFY);
    ATA_DELAY;
    // Polling:
    res = inb(dr->pbase_ + ATA_REG_STATUS);

    if (res == 0) {
        // kprintf ("no disc \n");
        dr->type_ = IDE_NODISC;
        return 0;
    }

    for (;;) {
        res = inb(dr->pbase_ + ATA_REG_STATUS);

        if ((res & ATA_STATUS_ERR)) {
            // Probe for ATAPI Devices:
            dr->type_ = IDE_NODISC;
            ATAPI_Detect(dr);

            if (dr->type_ == IDE_NODISC) {
                // kprintf ("unrecognized disc \n");
                return 0;
            }

            break;

        } // else if (!(res & ATA_STATUS_BSY) && (res & ATA_STATUS_DRQ)) {

        // kprintf ("ATA ");
        dr->type_ = IDE_ATA;

        break;
    }

    // (V) Read Identification Space of the Device:
    insl(dr->pbase_ + ATA_REG_DATA, (uint32_t *)ptr, 128);
    // Read Device Parameters:
    dr->signature_ = *((uint16_t *)(ptr + ATA_IDENT_DEVICETYPE));
    dr->capabilities_ = *((uint16_t *)(ptr + ATA_IDENT_CAPABILITIES));
    dr->commandsets_  = *((uint32_t *)(ptr + ATA_IDENT_COMMANDSETS));
    dr->size_ = (dr->commandsets_ & (1 << 26)) ?
                *((uint32_t *)(ptr + ATA_IDENT_MAX_LBA_EXT)) :
                *((uint32_t *)(ptr + ATA_IDENT_MAX_LBA));
    if (dr->size_ >= 0x80000000)
        dr->size_ = 0;

    // String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
    for (k = 0; k < 40; k += 2) {
        dr->model_[k] = ptr[ATA_IDENT_MODEL + k + 1];
        dr->model_[k + 1] = ptr[ATA_IDENT_MODEL + k];
    }

    k = 39;
    while (dr->model_[k] == ' ' || dr->model_[k] == -1)
        dr->model_[k--] = '\0';
    dr->model_[40] = '\0';// Terminate String.
    // kprintf (" Size: %d Kb, %s \n", dr->size_ / 2, dr->model_);
    return 1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static int ATA_Polling(struct ATA_Drive *dr)
{
    // (I) Delay 400 nanosecond for BSY to be set:
    ATA_DELAY;

    // (II) Wait for BSY to be cleared:
    while (inb(dr->pbase_ + ATA_REG_STATUS) &
           ATA_STATUS_BSY); // Wait for BSY to be zero.

    uint8_t state = inb(dr->pbase_ + ATA_REG_STATUS); // Read Status Register.

    // (III) Check For Errors:
    if (state & ATA_STATUS_ERR) {
        kprintf(KL_ERR, "ATA] device on error\n");
        return 2; // Error.
    }

    // (IV) Check If Device fault:
    if (state & ATA_STATUS_DF) {
        kprintf(KL_ERR, "ATA] device fault\n");
        return 1; // Device Fault.
    }

    // (V) Check DRQ:
    // -------------------------------------------------
    // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
    if ((state & ATA_STATUS_DRQ) == 0) {
        kprintf(KL_ERR, "ATA] DRQ should be set\n");
        return 3; // DRQ should be set
    }

    return 0;
}


static int ATA_Data(int dir, struct ATA_Drive *dr, uint32_t lba,
                    uint8_t sects, uint8_t *buf)
{
    uint8_t lbaIO[6] = { 0 };
    int mode, cmd, i;
    int head, cyl, sect;

    // Select one from LBA28, LBA48 or CHS;
    if (lba >= 0x10000000) {
        if (!(dr->capabilities_ & 0x200)) {
            // kprintf ("Wrong LBA, LBA not supported here");
            return ENOSYS;
        }

        mode  = 2;
        lbaIO[0] = (lba & 0x000000FF) >> 0;
        lbaIO[1] = (lba & 0x0000FF00) >> 8;
        lbaIO[2] = (lba & 0x00FF0000) >> 16;
        lbaIO[3] = (lba & 0xFF000000) >> 24;

    } else if (dr->capabilities_ & 0x200)  {
        mode = 1;
        lbaIO[0] = (lba & 0x00000FF) >> 0;
        lbaIO[1] = (lba & 0x000FF00) >> 8;
        lbaIO[2] = (lba & 0x0FF0000) >> 16;
        head = (lba & 0xF000000) >> 24;

    } else {
        mode = 0;
        sect      = (lba % 63) + 1;
        cyl       = (lba + 1  - sect) / (16 * 63);
        head      = (lba + 1  - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
        lbaIO[0] = sect;
        lbaIO[1] = (cyl >> 0) & 0xFF;
        lbaIO[2] = (cyl >> 8) & 0xFF;
    }

    // Wait the device
    while (inb(dr->pbase_ + ATA_REG_STATUS) & ATA_STATUS_BSY);

    // Select Drive from the controller;
    if (mode == 0) {
        outb(dr->pbase_ + ATA_REG_HDDEVSEL, dr->disc_ | head);    // Drive & CHS.
    } else {
        outb(dr->pbase_ + ATA_REG_HDDEVSEL,
             0xE0 | dr->disc_ | head);    // Drive & LBA
    }

    // Write Parameters;
    if (mode == 2) {
        /*
        outb(channel, ATA_REG_SECCOUNT1,   0);
        outb(channel, ATA_REG_LBA3,   lba_io[3]);
        outb(channel, ATA_REG_LBA4,   lba_io[4]);
        outb(channel, ATA_REG_LBA5,   lba_io[5]);
        */
    }

    outb(dr->pbase_ + ATA_REG_SECCOUNT0, sects);
    outb(dr->pbase_ + ATA_REG_LBA0, lbaIO[0]);
    outb(dr->pbase_ + ATA_REG_LBA1, lbaIO[1]);
    outb(dr->pbase_ + ATA_REG_LBA2, lbaIO[2]);

    // DMA dir |= 0x02
    if (mode < 2) {
        if (dir == 0)
            cmd = ATA_CMD_READ_PIO;
        if (dir == 1)
            cmd = ATA_CMD_WRITE_PIO;
        if (dir == 2)
            cmd = ATA_CMD_READ_DMA;
        if (dir == 3)
            cmd = ATA_CMD_WRITE_DMA;

    } else {
        if (dir == 0)
            cmd = ATA_CMD_READ_PIO_EXT;
        if (dir == 1)
            cmd = ATA_CMD_WRITE_PIO_EXT;
        if (dir == 2)
            cmd = ATA_CMD_READ_DMA_EXT;
        if (dir == 3)
            cmd = ATA_CMD_WRITE_DMA_EXT;
    }

    outb(dr->pbase_ + ATA_REG_COMMAND, cmd);      // Send the Command.

    // Read/Write
    if (dir == 0) {
        for (i = 0; i < sects; i++) {
            if (ATA_Polling(dr)) {
                // Polling, set error and exit if there is.
                return EIO;
            }

            insw(dr->pbase_, (uint16_t *)buf, 256);
            buf += 512;
        }

    } else if (dir == 1) {
        for (i = 0; i < sects; i++) {
            if (ATA_Polling(dr)) {
                // Polling, set error and exit if there is.
                return EIO;
            }

            outsw(dr->pbase_, (uint16_t *)buf, 256);
            buf += 512;
        }

        outb(dr->pbase_ + ATA_REG_COMMAND, (mode == 2) ?
             ATA_CMD_CACHE_FLUSH_EXT :
             ATA_CMD_CACHE_FLUSH);
        ATA_Polling(dr);

    } else
        return EIO;

    return 0;
}

static int ATAPI_Read(struct ATA_Drive *dr, uint32_t lba,  uint8_t sects,
                      uint8_t *buf)
{
    int words =
        1024; // Sector Size. ATAPI drives have a sector size of 2048 bytes.
    int i;
    uint8_t packet[12];

    // kprintf (" - ATAPI] read %d sector at LBA: %d on %x\n", sects, lba, buf);

    irq_disable();
    outb(dr->pbase_ + ATA_REG_CONTROL, 0x0);

    // Setup SCSI Packet:
    packet[ 0] = ATAPI_CMD_READ;
    packet[ 1] = 0x0;
    packet[ 2] = (lba >> 24) & 0xFF;
    packet[ 3] = (lba >> 16) & 0xFF;
    packet[ 4] = (lba >> 8) & 0xFF;
    packet[ 5] = (lba >> 0) & 0xFF;
    packet[ 6] = 0x0;
    packet[ 7] = 0x0;
    packet[ 8] = 0x0;
    packet[ 9] = sects;
    packet[10] = 0x0;
    packet[11] = 0x0;
    // Select the drive:
    outb(dr->pbase_ + ATA_REG_HDDEVSEL, dr->disc_ & 0x10);

    // Delay 400 nanosecond for BSY to be set:
    ATA_DELAY;

    // Inform the Controller:
    outb(dr->pbase_ + ATA_REG_FEATURES, 0);               // Use PIO mode
    outb(dr->pbase_ + ATA_REG_LBA1,
         (words * 2) & 0xFF);  // Lower Byte of Sector Size.
    outb(dr->pbase_ + ATA_REG_LBA2,
         (words * 2) >> 8);    // Upper Byte of Sector Size.
    outb(dr->pbase_ + ATA_REG_COMMAND, ATA_CMD_PACKET);   // Send the Command.

    // Waiting for the driver to finish or return an error code
    if (ATA_Polling(dr)) {
        irq_enable();
        return EIO;
    }

    // Sending the packet data
    outsw(dr->pbase_, (uint16_t *)packet, 6);

    // Receiving Data
    for (i = 0; i < sects; i++) {
        // ATA_WaitIRQ (dr, 14); // Wait for an IRQ.
        if (ATA_Polling(dr)) {
            irq_enable();
            return EIO;
        }

        // kprintf (" - ATAPI] Just get data\n");

        insw(dr->pbase_, (uint16_t *)buf, words);
        buf += (words * 2);
    }

    // Waiting for an IRQ
    // ATA_WaitIRQ (14);
    // kIrq_Wait(0);

    // Waiting for BSY & DRQ to clear
    while (inb(dr->pbase_ + ATA_REG_STATUS) & (ATA_STATUS_BSY | ATA_STATUS_DRQ));

    irq_enable();
    return 0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ata_read(inode_t *ino, char *data, size_t size, xoff_t off, int flags)
{
    struct ATA_Drive *dr = &sdx[ino->lba];
    uint32_t lba = off / ino->dev->block;
    uint8_t sects = size / ino->dev->block;

    if (dr->type_ == IDE_ATA)
        errno = ATA_Data(ATA_READ, dr, lba, sects, (uint8_t *)data);

    else if (dr->type_ == IDE_ATAPI)
        errno = ATAPI_Read(dr, lba, sects, (uint8_t *)data);

    else
        errno = EIO;
    return errno == 0 ? 0 : -1;
}

int ata_write(inode_t *ino, const char *data, size_t size, xoff_t off, int flags)
{
    struct ATA_Drive *dr = &sdx[ino->lba];
    uint32_t lba = off / ino->dev->block;
    uint8_t sects = size / ino->dev->block;

    if (dr->type_ == IDE_ATA)
        errno = ATA_Data(ATA_WRITE, dr, lba, sects, (uint8_t *)data);

    else if (dr->type_ == IDE_ATAPI)
        errno = EROFS;

    else
        errno = EIO;
    return errno == 0 ? 0 : -1;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

ino_ops_t ata_ino_ops = {
    .read = ata_read,
    .write = ata_write,
    //    .ioctl = ata_ioctl,
};




/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void ata_setup()
{
    int i;
    memset(sdx, 0, 4 * sizeof(struct ATA_Drive));
    sdx[0].pbase_ = 0x1f0;
    sdx[0].pctrl_ = 0x3f6;
    sdx[0].disc_ = 0xa0;
    sdx[1].pbase_ = 0x1f0;
    sdx[1].pctrl_ = 0x3f6;
    sdx[1].disc_ = 0xb0;
    sdx[2].pbase_ = 0x170;
    sdx[2].pctrl_ = 0x376;
    sdx[2].disc_ = 0xa0;
    sdx[3].pbase_ = 0x170;
    sdx[3].pctrl_ = 0x376;
    sdx[3].disc_ = 0xb0;

    // outb(0x3f6 + ATA_REG_CONTROL - 0x0A, 2);
    // outb(0x376 + ATA_REG_CONTROL - 0x0A, 2);
    irq_register(14, ata_irq_handler, NULL);
    irq_register(15, ata_irq_handler, NULL);


    for (i = 0; i < 4; ++i) {
        if (ATA_Detect(&sdx[i])) {
            inode_t *blk = vfs_inode(i, FL_BLK, NULL, &ata_ino_ops);
            blk->length = sdx[i].size_;
            blk->lba = i;
            blk->dev->block = sdx[i].type_ == IDE_ATA ? 512 : 2048;
            blk->dev->flags = FD_RDONLY;
            blk->dev->model = strdup(sdx[i].model_);
            blk->dev->devclass = strdup(IDE_ATA ? "IDE ATA" : "IDE ATAPI");
            vfs_mkdev(blk, sdNames[i]);
            vfs_close_inode(blk);
        }
    }
}


void ata_teardown()
{
    int i;
    for (i = 0; i < 4; ++i) {
        if (sdx[i].type_ == IDE_ATA || sdx[i].type_ == IDE_ATAPI)
            vfs_rmdev(sdNames[i]);
    }
}


EXPORT_MODULE(ide_ata, ata_setup, ata_teardown);
