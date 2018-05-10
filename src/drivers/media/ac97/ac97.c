#include <kernel/core.h>
#include <kernel/drv/pci.h>
#include <kernel/cpu.h>

#define AC97_VENDOR_ID 0x8086
#define AC97_DEVICE_ID 0x2415
#define AC97_NAME "ICH AC97"


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */


void ac97_startup(struct PCI_device *pci)
{
    // IO region #0: d100..d200
    // IO region #1: d200..d240

}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int ac97_match_pci_device(uint16_t vendor, uint32_t class, uint16_t device)
{
    if (vendor != AC97_VENDOR_ID || device != AC97_DEVICE_ID)
        return -1;
    return 0;
}


void ac97_setup()
{
    struct PCI_device *pci = NULL;

    for(;;) {
        pci = PCI_search2(ac97_match_pci_device);
        if (pci == NULL)
            break;
        // kprintf(0, "Found %s (PCI.%02d.%02d)\n", AC97_NAME, pci->bus, pci->slot);
        ac97_startup(pci);
    }
}

void ac97_teardown()
{
}


MODULE(ac97, MOD_AGPL, ac97_setup, ac97_teardown);
// MOD_REQUIRE(pci);

