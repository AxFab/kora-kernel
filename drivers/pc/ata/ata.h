#ifndef _ATA_H
#define _ATA_H 1

#include <kernel/vfs.h>

/* default ports */
#define ATA_PRIMARY_PORT_BASE       0x1F0
#define ATA_PRIMARY_PORT_CONTROL    0x3F6
#define ATA_SECONDARY_PORT_BASE       0x170
#define ATA_SECONDARY_PORT_CONTROL    0x376

/* ATA status */
#define ATA_STATUS_BSY              0x80    /* Busy */
#define ATA_STATUS_DRDY             0x40    /* Drive ready */
#define ATA_STATUS_DF               0x20    /* Drive write fault */
#define ATA_STATUS_DSC              0x10    /* Drive seek complete */
#define ATA_STATUS_DRQ              0x08    /* Data request ready */
#define ATA_STATUS_CORR             0x04    /* Corrected data */
#define ATA_STATUS_IDX              0x02    /* Inlex */
#define ATA_STATUS_ERR              0x01    /* Error */

/* ATA errors */
#define ATA_ERROR_BBK               0x080   /* Bad block */
#define ATA_ERROR_UNC               0x040   /* Uncorrectable data */
#define ATA_ERROR_MC                0x020   /* Media changed */
#define ATA_ERROR_IDNF              0x010   /* ID mark not found */
#define ATA_ERROR_MCR               0x008   /* Media change request */
#define ATA_ERROR_ABRT              0x004   /* Command aborted */
#define ATA_ERROR_TK0NF             0x002   /* Track 0 not found */
#define ATA_ERROR_AMNF              0x001   /* No address mark */

/* ATA commands */
#define ATA_CMD_RESET               0x04
#define ATA_CMD_READ_PIO            0x20
#define ATA_CMD_READ_PIO_EXT        0x24
#define ATA_CMD_WRITE_PIO           0x30
#define ATA_CMD_WRITE_PIO_EXT       0x34
#define ATA_CMD_READ_DMA            0xC8
#define ATA_CMD_READ_DMA_EXT        0x25
#define ATA_CMD_WRITE_DMA           0xCA
#define ATA_CMD_WRITE_DMA_EXT       0x35
#define ATA_CMD_CACHE_FLUSH         0xE7
#define ATA_CMD_CACHE_FLUSH_EXT     0xEA
#define ATA_CMD_PACKET              0xA0
#define ATA_CMD_IDENTIFY_PACKET     0xA1
#define ATA_CMD_IDENTIFY            0xEC

/* ATAPI commands */
#define ATAPI_CMD_READ              0xA8
#define ATAPI_CMD_EJECT             0x1B

/* ATA identification */
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

/* ATA registers */
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

/* ATA devices */
#define ATADEV_PATA  0x0000
#define ATADEV_SATA  0xC33C
#define ATADEV_PATAPI  0xEB14
#define ATADEV_SATAPI  0x9669
#define ATADEV_UNKOWN  0xFFFF

/* ATA capabilities */
#define ATA_CAP_LBA 0x200
#define ATA_CAP_LBA48  0x4000000



#define ATA_MODE_CHS 0x10
#define ATA_MODE_LBA28 0x20
#define ATA_MODE_LBA48 0x40

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


typedef struct ata_drive ata_drive_t;

struct ata_drive {
    uint8_t id;
    uint8_t id_command;
    uint8_t mode;
    bool slave;
    uint16_t base;
    uint16_t ctrl;
    uint16_t block;
    uint16_t signature;
    uint16_t capabilities;
    uint32_t command_sets;
    uint32_t max_lba;
    const char *name;
    const char *class;
    ino_ops_t *ops;
    char model[44];
};


extern ino_ops_t ata_ops_pata;
extern ino_ops_t ata_ops_patapi;

void ata_wait(ata_drive_t *drive);
int ata_poll(ata_drive_t *drive, bool check);
void ata_soft_reset(ata_drive_t *drive);
void ata_probe();


#endif  /* _ATA_H */
