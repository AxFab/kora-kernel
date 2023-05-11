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
    if (!drive->slave) {
        ata_soft_reset(drive);
        ata_wait(drive);
    }

    outb(drive->base + ATA_REG_HDDEVSEL, drive->slave ? 0xb0 : 0xa0);
    // ata_select_drive(drive);
    ata_poll(drive, false);

    // Send ATA Identify Command:
    outb(drive->base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_wait(drive);

    int k, status = inb(drive->base + ATA_REG_STATUS);
    if (status == 0) {
        drive->mode = 0;
        return;
    }

    uint8_t mode_lo = inb(drive->base + ATA_REG_LBA1);
    uint8_t mode_hi = inb(drive->base + ATA_REG_LBA2);
    uint16_t mode = (mode_hi << 8) | mode_lo;

    switch (mode) {
    case ATADEV_PATA: // Parallel ATA device
        drive->id_command = ATA_CMD_IDENTIFY;
        drive->block = 512;
        drive->class = "IDE PATA";
        drive->ops = &ata_ops_pata;
        break;
    case ATADEV_SATA: // Emulated SATA
        drive->id_command = ATA_CMD_IDENTIFY;
        drive->block = 512;
        drive->class = "IDE SATA";
        drive->ops = &ata_ops_pata; // TODO
        break;
    case ATADEV_PATAPI:
        drive->id_command = ATA_CMD_IDENTIFY_PACKET;
        drive->block = 2048;
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

    if (drive->id_command == ATA_CMD_IDENTIFY) {

        if (drive->command_sets & ATA_CAP_LBA48) {
            /* 48-bit LBA */
            uint32_t high = 0;//*(uint32_t *) (ptr + ATA_IDENT_MAX_LBA_EXT);
            uint32_t low = *(uint32_t *) (ptr + ATA_IDENT_MAX_LBA);
            drive->max_lba = ((uint64_t) high << 32) | low;
            drive->mode = ATA_MODE_LBA48;
        } else if (drive->command_sets & ATA_CAP_LBA) {
            /* 28-bit LBA */
            drive->max_lba = *(uint32_t *) (ptr + ATA_IDENT_MAX_LBA);
            drive->mode = ATA_MODE_LBA28;
        } else {
            // TODO -- find number of head !?
            // uint8_t hddlvl =  inb(drive->base + ATA_REG_HDDEVSEL);
            drive->max_lba = 1 * 16 * 63;
            drive->mode = ATA_MODE_CHS;
        }
    } else {
        drive->mode = ATA_MODE_ATAPI;
        patapi_read_capacity(drive);
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
    // kdump2(ptr, 128 * 4);
    // kprintf(-1, "ata%d: signature: 0x%x\n", drive->id, drive->signature);
    // kprintf(-1, "ata%d: capabilities: 0x%x\n", drive->id, drive->capabilities);
    // kprintf(-1, "ata%d: command Sets: 0x%x\n", drive->id, drive->command_sets);
    // kprintf(-1, "ata%d: max LBA: 0x%x\n", drive->id, drive->max_lba);
    // kprintf(-1, "ata%d: model: %s\n", drive->id, drive->model);

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

int ata_match_pci_device(uint16_t vendor, uint32_t class, uint16_t device) {
    return (class >> 8) == 0x0101 ? 0 : -1;
}

void ata_probe()
{
    int i = 0;
    // pci_scan_device(0x01, 0x01, &ide, 1);
    // class_id >> 8 = 0x0101;
    struct PCI_device *ide = NULL;
    // pci_device_t *ide = NULL;
    ide = pci_search(ata_match_pci_device, &i);

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

void ata_setup()
{
    irq_register(14, ata_irq_handler, (void*)0);
    irq_register(15, ata_irq_handler, (void*)1);
    ata_probe();
}


void ata_teardown()
{
    for (int i = 0; i < 4; ++i) {
        if (drives[i].mode)
            vfs_rmdev(drives[i].name);
    }
}


EXPORT_MODULE(ide_ata, ata_setup, ata_teardown);
