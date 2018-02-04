#include <kernel/core.h>
#include <kernel/net.h>
#include <kernel/cpu.h>
#include <kernel/drv/pci.h>
#include <string.h>

#ifdef K_MODULE
#  define VGA_setup setup
#  define VGA_teardown teardown
#endif


#define INNOTEK_VENDOR 0x80ee
#define VGA_CTRL_CLASSID 0x30000

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define VGA_AC_INDEX    0x3C0
#define VGA_AC_WRITE    0x3C0
#define VGA_AC_READ   0x3C1
#define VGA_MISC_WRITE    0x3C2
#define VGA_SEQ_INDEX   0x3C4
#define VGA_SEQ_DATA    0x3C5
#define VGA_DAC_READ_INDEX  0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA    0x3C9
#define VGA_MISC_READ   0x3CC
#define VGA_GC_INDEX    0x3CE
#define VGA_GC_DATA     0x3CF
/*      COLOR emulation   MONO emulation */
#define VGA_CRTC_INDEX    0x3D4   /* 0x3B4 */
#define VGA_CRTC_DATA   0x3D5   /* 0x3B5 */
#define VGA_INSTAT_READ   0x3DA /* Input status */

#define VGA_NUM_SEQ_REGS  5
#define VGA_NUM_CRTC_REGS 25
#define VGA_NUM_GC_REGS   9
#define VGA_NUM_AC_REGS   21
#define VGA_NUM_REGS    (1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + \
        VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

typedef struct VBE_info VBE_info_t;

PACK(struct VBE_info {
    char signature[4]; // must be "VESA" to indicate valid VBE support
    uint16_t version;     // VBE version; high byte is major version, low byte is minor version
    uint32_t oem;     // segment:offset pointer to OEM
    uint32_t capabilities;    // bitfield that describes card capabilities
    uint32_t video_modes;   // segment:offset pointer to list of supported video modes
    uint16_t video_memory;    // amount of video memory in 64KB blocks
    uint16_t software_rev;    // software revision
    uint32_t vendor;      // segment:offset to card vendor string
    uint32_t product_name;    // segment:offset to card model name
    uint32_t product_rev;   // segment:offset pointer to product revision
    char reserved[222];   // reserved for future expansion
    char oem_data[256];   // OEM BIOSes store their strings in this area
});


// struct VBE_regs {
//   uint8_t misc;
//   uint8_t seq[VGA_NUM_SEQ_REGS];
//   uint8_t seq[VGA_NUM_SEQ_REGS];
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void VBE_dumpregs()
{
    int i;
    kprintf(-1, "uint8_t g_mode[] = {\n");
    kprintf(-1, "  /* MISCELLANEOUS register */\n");
    kprintf(-1, "  0x%02x,", inb(VGA_MISC_READ) & 0xFF);

    kprintf(-1, "\n  /* SEQUENCER registers */ \n ");
    for (i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        outb(VGA_SEQ_INDEX, i);
        kprintf(-1, " 0x%02x,", inb(VGA_SEQ_DATA) & 0xFF);
        if ((i + 1) % 8 == 0) {
            kprintf(-1, "\n ");
        }
    }

    kprintf(-1, "\n  /* CRTC registers */ \n ");
    for (i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        outb(VGA_CRTC_INDEX, i);
        kprintf(-1, " 0x%02x,", inb(VGA_CRTC_DATA) & 0xFF);
        if ((i + 1) % 8 == 0) {
            kprintf(-1, "\n ");
        }
    }

    kprintf(-1, "\n  /* GRAPHICS CONTROLLER registers */ \n ");
    for (i = 0; i < VGA_NUM_GC_REGS; i++) {
        outb(VGA_GC_INDEX, i); // GRAPHICS CONTROLLER
        kprintf(-1, " 0x%02x,", inb(VGA_GC_DATA) & 0xFF);
        if ((i + 1) % 8 == 0) {
            kprintf(-1, "\n ");
        }
    }

    kprintf(-1, "\n  /* ATTRIBUTE CONTROLLER registers */ \n ");
    for (i = 0; i < VGA_NUM_AC_REGS; i++) {
        inb(VGA_INSTAT_READ);
        outb(VGA_AC_INDEX, i); // ATTRIBUTE CONTROLLER
        kprintf(-1, " 0x%02x,", inb(VGA_AC_READ) & 0xFF);
        if ((i + 1) % 8 == 0) {
            kprintf(-1, "\n ");
        }
    }
    kprintf(-1, "};\n");

    inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20);
}

void VBE_writeregs(uint8_t *cfg)
{
    int i;
    /* Write MISCELLANEOUS register */
    outb(VGA_MISC_WRITE, cfg[0]);
    /* Write SEQUENCER register */
    for (i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        outb(VGA_SEQ_INDEX, i);
        outb(VGA_SEQ_DATA, cfg[i + 1]);
    }
    /* Unlock CRTC registers */
    outb(VGA_CRTC_INDEX, 0x03);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);
    /* make sure they remain unlocked */
    cfg[6 + 3] |= 0x80;
    cfg[6 + 17] &= ~0x80;
    /* Write CRTC registers */
    for (i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        outb(VGA_CRTC_INDEX, i);
        outb(VGA_CRTC_DATA, cfg[i + 1 + VGA_NUM_SEQ_REGS]);
        cfg++;
    }

    /* write GRAPHICS CONTROLLER regs */
    for (i = 0; i < VGA_NUM_GC_REGS; i++) {
        outb(VGA_GC_INDEX, i);
        outb(VGA_GC_DATA, cfg[i + 1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS]);
    }
    /* write ATTRIBUTE CONTROLLER regs */
    for (i = 0; i < VGA_NUM_AC_REGS; i++) {
        inb(VGA_INSTAT_READ);
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, cfg[i + 1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS +
                                 VGA_NUM_GC_REGS]);
    }

    inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20);
}

struct VBE_regs {
    uint8_t misc;
    uint8_t seq[5];
    uint8_t crtc[25];
    /* 00 - Horizontal total register
       01 - Horizontal display end
       02 - Horizontal blanking start
       03 - Horizontal display skew[5..6] / Horizontal blanking end [0..4]
       04 - Horizontal retrace start
       05 - Horizontal blanking end [5] / Horizontal retrace end [0..4]
       06 - Vertical Total

    */
    uint8_t gc[9];
    /*
      Index 00h -- Set/Reset Register
      Index 01h -- Enable Set/Reset Register
      Index 02h -- Color Compare Register
      Index 03h -- Data Rotate Register
      Index 04h -- Read Map Select Register
      Index 05h -- Graphics Mode Register
      Index 06h -- Miscellaneous Graphics Register
      Index 07h -- Color Don't Care Register
      Index 08h -- Bit Mask Register
      */
    uint8_t ac[21];
};


unsigned char g_90x60_text[] = {
    /* MISC */
    0xE7,
    /* SEQ */
    0x03, 0x01, 0x03, 0x00, 0x02,
    /* CRTC */
    0x6B, 0x59, 0x5A, 0x82, 0x60, 0x8D, 0x0B, 0x3E,
    0x00, 0x47, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00,
    0xEA, 0x0C, 0xDF, 0x2D, 0x08, 0xE8, 0x05, 0xA3,
    0xFF,
    /* GC */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
    0xFF,
    /* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x0C, 0x00, 0x0F, 0x08, 0x00,
};

unsigned char g_640x480x16[] = {
    /* MISC */
    0xE3,
    /* SEQ */
    0x03, 0x01, 0x08, 0x00, 0x06,
    /* CRTC */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xEA, 0x0C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
    0xFF,
    /* GC */
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x0F,
    0xFF,
    /* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x01, 0x00, 0x0F, 0x00, 0x00
};

uint8_t g_800x600x32[] = {
    /* MISCELLANEOUS register */
    0x67,
    /* SEQUENCER registers */
    0x3, 0x0, 0xf, 0x0, 0xa,
    /* CRTC registers */
    0x5f, 0x63, 0x50, 0x82, 0x55, 0x81, 0xbf, 0x5d,
    0x0, 0x40, 0x20, 0x0, 0x0, 0x0, 0x10, 0x0,
    0x9c, 0x0, 0x57, 0x64, 0x5f, 0x96, 0xb9, 0xa3,
    0xff,
    /* GRAPHICS CONTROLLER registers */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x50, 0x5, 0xf,
    0xff,
    /* ATTRIBUTE CONTROLLER registers */
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x14, 0x7,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x4d, 0x0, 0xf, 0x8, 0x0,
};


int VGA_statup(struct PCI_device *pci)
{
    // VBE_info_t info;


    // memcpy(info.signature, "VBE2", 4);

    VBE_dumpregs();
    // VBE_writeregs(g_800x600x32);
    kprintf(-1, "Is this mode supported ? \n");
    // int ret = bios_call(0x4f00, &info);
    // kprintf(-1, "[VBE ] Return value %x\n", ret);
    // bufdump(&info, 512);
}


int VGA_setup()
{
    struct PCI_device *pci;
    const char *name;

    for (;;) {
        pci = PCI_search(INNOTEK_VENDOR, VGA_CTRL_CLASSID, 0xbeef);
        name = "VGA stuff";
        if (pci == NULL) {
            return 0;
        }

        kprintf(0, "Found %s\n", name);
        VGA_statup(pci);
    }
}

int VGA_teardown()
{
    return 0;
}
