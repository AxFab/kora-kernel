#include "e1000.h"

static void E1000_read_hw_address(struct E1000_adapter *adapter,
                                  uint8_t *mac_address)
{
    int i, j;
    PCI_wr32(adapter->pci, 0, REG_EERD, 0x1);
    for (i = 0; i < 1000; ++i) {
        if (PCI_rd32(adapter->pci, 0, REG_EERD) & 0x10) {

            for (j = 0; j < 3; ++j) {

                uint32_t value = 0;
                PCI_wr32(adapter->pci, 0, REG_EERD, (j << 8) | 1);
                while ((value & 0x10) == 0) {
                    value = PCI_rd32(adapter->pci, 0, REG_EERD);
                }
                value =  (value >> 16) & 0xFFFF;
                mac_address[2 * j] = value & 0xff;
                mac_address[2 * j + 1] = value >> 8;
            }
            return;
        }
    }

    for (i = 0; i < 6; ++i) {
        mac_address[i] = ((uint8_t *)adapter->pci->bar[0].mmio + 0x5400)[i];
    }


    // /* initialize eeprom parameters */
    // if (e1000_init_eeprom_params(hw)) {
    //   e_err(probe, "EEPROM initialization failed\n");
    //   goto err_eeprom;
    // }

    // /* before reading the EEPROM, reset the controller to
    //  * put the device in a known good starting state
    //  */

    // e1000_reset_hw(hw);

    // /* make sure the EEPROM is good */
    // if (e1000_validate_eeprom_checksum(hw) < 0) {
    //   e_err(probe, "The EEPROM Checksum Is Not Valid\n");
    //   e1000_dump_eeprom(adapter);
    //   /* set MAC address to all zeroes to invalidate and temporary
    //    * disable this device for the user. This blocks regular
    //    * traffic while still permitting ethtool ioctls from reaching
    //    * the hardware as well as allowing the user to run the
    //    * interface after manually setting a hw addr using
    //    * `ip set address`

    //   memset(hw->mac_addr, 0, netdev->addr_len);
    // } else {
    //   /* copy the MAC address out of the EEPROM */
    //   if (e1000_read_mac_addr(hw))
    //     e_err(probe, "EEPROM Read Error\n");
    // }
    // /* don't block initialization here due to bad MAC address */
    // memcpy(netdev->dev_addr, hw->mac_addr, netdev->addr_len);

    // if (!is_valid_ether_addr(netdev->dev_addr))
    //   e_err(probe, "Invalid MAC Address\n");

}

static void E1000_features(struct E1000_adapter *adapter)
{
    /* Identify the MAC */
    // switch (adapter->pci->device_id) {
    //   case E1000_DEV_ID_82540EM:
    //     // adapter->mac_type = e1000_82540;
    //   default:
    //     // ERROR
    // }

    /* Set media type and TBI compatibility. */
    // TODO

    /* Gets the current PCI bus type, speed, and width of the hardware */
    int status = PCI_rd32(adapter->pci, 0, REG_STATUS);
    // TODO

}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void E1000_irq_disable(struct E1000_adapter *adapter)
{
    PCI_wr32(adapter->pci, 0, REG_IMC, ~0);
    PCI_rd32(adapter->pci, 0, REG_STATUS); /* Flush */

}

static void E1000_irq_enable(struct E1000_adapter *adapter)
{
    PCI_wr32(adapter->pci, 0, REG_IMS, ~0);
    PCI_rd32(adapter->pci, 0, REG_STATUS); /* Flush */
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static void E1000_probe(struct PCI_device *pci, const char *name)
{
    uint8_t mac_address[6];

    /* Allocate IO ports or MMIO as required */

    /* Allocate the structure */
    struct E1000_adapter *adapter = (struct E1000_adapter *)kalloc(sizeof(
                                        struct E1000_adapter));
    adapter->pci = pci;
    E1000_read_hw_address(adapter, mac_address);
    net_device_t *ndev = net_register(name, adapter, mac_address, 6);



    E1000_irq_disable(adapter);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

// void E1000_setup()
// {


// }
