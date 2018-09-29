/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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


/* PCI Device IDs */
#define E1000_DEV_ID_82542               0x1000
#define E1000_DEV_ID_82543GC_FIBER       0x1001
#define E1000_DEV_ID_82543GC_COPPER      0x1004
#define E1000_DEV_ID_82544EI_COPPER      0x1008
#define E1000_DEV_ID_82544EI_FIBER       0x1009
#define E1000_DEV_ID_82544GC_COPPER      0x100C
#define E1000_DEV_ID_82544GC_LOM         0x100D
#define E1000_DEV_ID_82540EM             0x100E
#define E1000_DEV_ID_82540EM_LOM         0x1015
#define E1000_DEV_ID_82540EP_LOM         0x1016
#define E1000_DEV_ID_82540EP             0x1017
#define E1000_DEV_ID_82540EP_LP          0x101E
#define E1000_DEV_ID_82545EM_COPPER      0x100F
#define E1000_DEV_ID_82545EM_FIBER       0x1011
#define E1000_DEV_ID_82545GM_COPPER      0x1026
#define E1000_DEV_ID_82545GM_FIBER       0x1027
#define E1000_DEV_ID_82545GM_SERDES      0x1028
#define E1000_DEV_ID_82546EB_COPPER      0x1010
#define E1000_DEV_ID_82546EB_FIBER       0x1012
#define E1000_DEV_ID_82546EB_QUAD_COPPER 0x101D
#define E1000_DEV_ID_82541EI             0x1013
#define E1000_DEV_ID_82541EI_MOBILE      0x1018
#define E1000_DEV_ID_82541ER_LOM         0x1014
#define E1000_DEV_ID_82541ER             0x1078
#define E1000_DEV_ID_82547GI             0x1075
#define E1000_DEV_ID_82541GI             0x1076
#define E1000_DEV_ID_82541GI_MOBILE      0x1077
#define E1000_DEV_ID_82541GI_LF          0x107C
#define E1000_DEV_ID_82546GB_COPPER      0x1079
#define E1000_DEV_ID_82546GB_FIBER       0x107A
#define E1000_DEV_ID_82546GB_SERDES      0x107B
#define E1000_DEV_ID_82546GB_PCIE        0x108A
#define E1000_DEV_ID_82546GB_QUAD_COPPER 0x1099
#define E1000_DEV_ID_82547EI             0x1019
#define E1000_DEV_ID_82547EI_MOBILE      0x101A
#define E1000_DEV_ID_82546GB_QUAD_COPPER_KSP3 0x10B5
#define E1000_DEV_ID_INTEL_CE4100_GBE    0x2E6E


/* Register Set.
 *
 * Registers are defined to be 32 bits and  should be accessed as 32 bit values.
 * These registers are physically located on the NIC, but are mapped into the
 * host memory address space.
 *
 * RW - register is both readable and writable
 * RO - register is read only
 * WO - register is write only
 * R/clr - register is read only and is cleared when read
 * A - register array
 */
#define REG_CTRL     0x00000  /* Device Control - RW */
#define REG_CTRL_DUP 0x00004  /* Device Control Duplicate (Shadow) - RW */
#define REG_STATUS   0x00008  /* Device Status - RO */
#define REG_EECD     0x00010  /* EEPROM/Flash Control - RW */
#define REG_EERD     0x00014  /* EEPROM Read - RW */
#define REG_CTRL_EXT 0x00018  /* Extended Device Control - RW */
#define REG_FLA      0x0001C  /* Flash Access - RW */
#define REG_MDIC     0x00020  /* MDI Control - RW */

#define REG_SCTL     0x00024  /* SerDes Control - RW */
#define REG_FEXTNVM  0x00028  /* Future Extended NVM register */
#define REG_FCAL     0x00028  /* Flow Control Address Low - RW */
#define REG_FCAH     0x0002C  /* Flow Control Address High -RW */
#define REG_FCT      0x00030  /* Flow Control Type - RW */
#define REG_VET      0x00038  /* VLAN Ether Type - RW */
#define REG_ICR      0x000C0  /* Interrupt Cause Read - R/clr */
#define REG_ITR      0x000C4  /* Interrupt Throttling Rate - RW */
#define REG_ICS      0x000C8  /* Interrupt Cause Set - WO */
#define REG_IMS      0x000D0  /* Interrupt Mask Set - RW */
#define REG_IMC      0x000D8  /* Interrupt Mask Clear - WO */
#define REG_IAM      0x000E0  /* Interrupt Acknowledge Auto Mask */

#define REG_RCTL     0x00100  /* RX Control - RW */
#define REG_RDTR1    0x02820  /* RX Delay Timer (1) - RW */
#define REG_RDBAL1   0x02900  /* RX Descriptor Base Address Low (1) - RW */
#define REG_RDBAH1   0x02904  /* RX Descriptor Base Address High (1) - RW */
#define REG_RDLEN1   0x02908  /* RX Descriptor Length (1) - RW */
#define REG_RDH1     0x02910  /* RX Descriptor Head (1) - RW */
#define REG_RDT1     0x02918  /* RX Descriptor Tail (1) - RW */
#define REG_FCTTV    0x00170  /* Flow Control Transmit Timer Value - RW */
#define REG_TXCW     0x00178  /* TX Configuration Word - RW */
#define REG_RXCW     0x00180  /* RX Configuration Word - RO */
#define REG_TCTL     0x00400  /* TX Control - RW */
#define REG_TCTL_EXT 0x00404  /* Extended TX Control - RW */
#define REG_TIPG     0x00410  /* TX Inter-packet gap -RW */
#define REG_TBT      0x00448  /* TX Burst Timer - RW */
#define REG_AIT      0x00458  /* Adaptive Interframe Spacing Throttle - RW */
#define REG_LEDCTL   0x00E00  /* LED Control - RW */
#define REG_EXTCNF_CTRL  0x00F00  /* Extended Configuration Control */
#define REG_EXTCNF_SIZE  0x00F08  /* Extended Configuration Size */
#define REG_PHY_CTRL     0x00F10  /* PHY Control Register in CSR */
#define FEXTNVM_SW_CONFIG  0x0001
#define REG_PBA      0x01000  /* Packet Buffer Allocation - RW */
#define REG_PBS      0x01008  /* Packet Buffer Size */
#define REG_EEMNGCTL 0x01010  /* MNG EEprom Control */
#define REG_FLASH_UPDATES 1000
#define REG_EEARBC   0x01024  /* EEPROM Auto Read Bus Control */
#define REG_FLASHT   0x01028  /* FLASH Timer Register */
#define REG_EEWR     0x0102C  /* EEPROM Write Register - RW */
#define REG_FLSWCTL  0x01030  /* FLASH control register */
#define REG_FLSWDATA 0x01034  /* FLASH data register */
#define REG_FLSWCNT  0x01038  /* FLASH Access Counter */
#define REG_FLOP     0x0103C  /* FLASH Opcode Register */
#define REG_ERT      0x02008  /* Early Rx Threshold - RW */
#define REG_FCRTL    0x02160  /* Flow Control Receive Threshold Low - RW */
#define REG_FCRTH    0x02168  /* Flow Control Receive Threshold High - RW */
#define REG_PSRCTL   0x02170  /* Packet Split Receive Control - RW */
#define REG_RDFH     0x02410  /* RX Data FIFO Head - RW */
#define REG_RDFT     0x02418  /* RX Data FIFO Tail - RW */
#define REG_RDFHS    0x02420  /* RX Data FIFO Head Saved - RW */
#define REG_RDFTS    0x02428  /* RX Data FIFO Tail Saved - RW */
#define REG_RDFPC    0x02430  /* RX Data FIFO Packet Count - RW */
#define REG_RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define REG_RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define REG_RDLEN    0x02808  /* RX Descriptor Length - RW */
#define REG_RDH      0x02810  /* RX Descriptor Head - RW */
#define REG_RDT      0x02818  /* RX Descriptor Tail - RW */
#define REG_RDTR     0x02820  /* RX Delay Timer - RW */
#define REG_RDBAL0   REG_RDBAL  /* RX Desc Base Address Low (0) - RW */
#define REG_RDBAH0   REG_RDBAH  /* RX Desc Base Address High (0) - RW */
#define REG_RDLEN0   REG_RDLEN  /* RX Desc Length (0) - RW */
#define REG_RDH0     REG_RDH    /* RX Desc Head (0) - RW */
#define REG_RDT0     REG_RDT    /* RX Desc Tail (0) - RW */
#define REG_RDTR0    REG_RDTR   /* RX Delay Timer (0) - RW */
#define REG_RXDCTL   0x02828  /* RX Descriptor Control queue 0 - RW */
#define REG_RXDCTL1  0x02928  /* RX Descriptor Control queue 1 - RW */
#define REG_RADV     0x0282C  /* RX Interrupt Absolute Delay Timer - RW */
#define REG_RSRPD    0x02C00  /* RX Small Packet Detect - RW */
#define REG_RAID     0x02C08  /* Receive Ack Interrupt Delay - RW */
#define REG_TXDMAC   0x03000  /* TX DMA Control - RW */
#define REG_KABGTXD  0x03004  /* AFE Band Gap Transmit Ref Data */
#define REG_TDFH     0x03410  /* TX Data FIFO Head - RW */
#define REG_TDFT     0x03418  /* TX Data FIFO Tail - RW */
#define REG_TDFHS    0x03420  /* TX Data FIFO Head Saved - RW */
#define REG_TDFTS    0x03428  /* TX Data FIFO Tail Saved - RW */
#define REG_TDFPC    0x03430  /* TX Data FIFO Packet Count - RW */
#define REG_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define REG_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define REG_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define REG_TDH      0x03810  /* TX Descriptor Head - RW */
#define REG_TDT      0x03818  /* TX Descripotr Tail - RW */
#define REG_TIDV     0x03820  /* TX Interrupt Delay Value - RW */
#define REG_TXDCTL   0x03828  /* TX Descriptor Control - RW */
#define REG_TADV     0x0382C  /* TX Interrupt Absolute Delay Val - RW */
#define REG_TSPMT    0x03830  /* TCP Segmentation PAD & Min Threshold - RW */
#define REG_TARC0    0x03840  /* TX Arbitration Count (0) */
#define REG_TDBAL1   0x03900  /* TX Desc Base Address Low (1) - RW */
#define REG_TDBAH1   0x03904  /* TX Desc Base Address High (1) - RW */
#define REG_TDLEN1   0x03908  /* TX Desc Length (1) - RW */
#define REG_TDH1     0x03910  /* TX Desc Head (1) - RW */
#define REG_TDT1     0x03918  /* TX Desc Tail (1) - RW */
#define REG_TXDCTL1  0x03928  /* TX Descriptor Control (1) - RW */
#define REG_TARC1    0x03940  /* TX Arbitration Count (1) */
#define REG_CRCERRS  0x04000  /* CRC Error Count - R/clr */
#define REG_ALGNERRC 0x04004  /* Alignment Error Count - R/clr */
#define REG_SYMERRS  0x04008  /* Symbol Error Count - R/clr */
#define REG_RXERRC   0x0400C  /* Receive Error Count - R/clr */
#define REG_MPC      0x04010  /* Missed Packet Count - R/clr */
#define REG_SCC      0x04014  /* Single Collision Count - R/clr */
#define REG_ECOL     0x04018  /* Excessive Collision Count - R/clr */
#define REG_MCC      0x0401C  /* Multiple Collision Count - R/clr */
#define REG_LATECOL  0x04020  /* Late Collision Count - R/clr */
#define REG_COLC     0x04028  /* Collision Count - R/clr */
#define REG_DC       0x04030  /* Defer Count - R/clr */
#define REG_TNCRS    0x04034  /* TX-No CRS - R/clr */
#define REG_SEC      0x04038  /* Sequence Error Count - R/clr */
#define REG_CEXTERR  0x0403C  /* Carrier Extension Error Count - R/clr */
#define REG_RLEC     0x04040  /* Receive Length Error Count - R/clr */
#define REG_XONRXC   0x04048  /* XON RX Count - R/clr */
#define REG_XONTXC   0x0404C  /* XON TX Count - R/clr */
#define REG_XOFFRXC  0x04050  /* XOFF RX Count - R/clr */
#define REG_XOFFTXC  0x04054  /* XOFF TX Count - R/clr */
#define REG_FCRUC    0x04058  /* Flow Control RX Unsupported Count- R/clr */
#define REG_PRC64    0x0405C  /* Packets RX (64 bytes) - R/clr */
#define REG_PRC127   0x04060  /* Packets RX (65-127 bytes) - R/clr */
#define REG_PRC255   0x04064  /* Packets RX (128-255 bytes) - R/clr */
#define REG_PRC511   0x04068  /* Packets RX (255-511 bytes) - R/clr */
#define REG_PRC1023  0x0406C  /* Packets RX (512-1023 bytes) - R/clr */
#define REG_PRC1522  0x04070  /* Packets RX (1024-1522 bytes) - R/clr */
#define REG_GPRC     0x04074  /* Good Packets RX Count - R/clr */
#define REG_BPRC     0x04078  /* Broadcast Packets RX Count - R/clr */
#define REG_MPRC     0x0407C  /* Multicast Packets RX Count - R/clr */
#define REG_GPTC     0x04080  /* Good Packets TX Count - R/clr */
#define REG_GORCL    0x04088  /* Good Octets RX Count Low - R/clr */
#define REG_GORCH    0x0408C  /* Good Octets RX Count High - R/clr */
#define REG_GOTCL    0x04090  /* Good Octets TX Count Low - R/clr */
#define REG_GOTCH    0x04094  /* Good Octets TX Count High - R/clr */
#define REG_RNBC     0x040A0  /* RX No Buffers Count - R/clr */
#define REG_RUC      0x040A4  /* RX Undersize Count - R/clr */
#define REG_RFC      0x040A8  /* RX Fragment Count - R/clr */
#define REG_ROC      0x040AC  /* RX Oversize Count - R/clr */
#define REG_RJC      0x040B0  /* RX Jabber Count - R/clr */
#define REG_MGTPRC   0x040B4  /* Management Packets RX Count - R/clr */
#define REG_MGTPDC   0x040B8  /* Management Packets Dropped Count - R/clr */
#define REG_MGTPTC   0x040BC  /* Management Packets TX Count - R/clr */
#define REG_TORL     0x040C0  /* Total Octets RX Low - R/clr */
#define REG_TORH     0x040C4  /* Total Octets RX High - R/clr */
#define REG_TOTL     0x040C8  /* Total Octets TX Low - R/clr */
#define REG_TOTH     0x040CC  /* Total Octets TX High - R/clr */
#define REG_TPR      0x040D0  /* Total Packets RX - R/clr */
#define REG_TPT      0x040D4  /* Total Packets TX - R/clr */
#define REG_PTC64    0x040D8  /* Packets TX (64 bytes) - R/clr */
#define REG_PTC127   0x040DC  /* Packets TX (65-127 bytes) - R/clr */
#define REG_PTC255   0x040E0  /* Packets TX (128-255 bytes) - R/clr */
#define REG_PTC511   0x040E4  /* Packets TX (256-511 bytes) - R/clr */
#define REG_PTC1023  0x040E8  /* Packets TX (512-1023 bytes) - R/clr */
#define REG_PTC1522  0x040EC  /* Packets TX (1024-1522 Bytes) - R/clr */
#define REG_MPTC     0x040F0  /* Multicast Packets TX Count - R/clr */
#define REG_BPTC     0x040F4  /* Broadcast Packets TX Count - R/clr */
#define REG_TSCTC    0x040F8  /* TCP Segmentation Context TX - R/clr */
#define REG_TSCTFC   0x040FC  /* TCP Segmentation Context TX Fail - R/clr */
#define REG_IAC      0x04100  /* Interrupt Assertion Count */
#define REG_ICRXPTC  0x04104  /* Interrupt Cause Rx Packet Timer Expire Count */
#define REG_ICRXATC  0x04108  /* Interrupt Cause Rx Absolute Timer Expire Count */
#define REG_ICTXPTC  0x0410C  /* Interrupt Cause Tx Packet Timer Expire Count */
#define REG_ICTXATC  0x04110  /* Interrupt Cause Tx Absolute Timer Expire Count */
#define REG_ICTXQEC  0x04118  /* Interrupt Cause Tx Queue Empty Count */
#define REG_ICTXQMTC 0x0411C  /* Interrupt Cause Tx Queue Minimum Threshold Count */
#define REG_ICRXDMTC 0x04120  /* Interrupt Cause Rx Descriptor Minimum Threshold Count */
#define REG_ICRXOC   0x04124  /* Interrupt Cause Receiver Overrun Count */
#define REG_RXCSUM   0x05000  /* RX Checksum Control - RW */
#define REG_RFCTL    0x05008  /* Receive Filter Control */
#define REG_MTA      0x05200  /* Multicast Table Array - RW Array */
#define REG_RA       0x05400  /* Receive Address - RW Array */
#define REG_VFTA     0x05600  /* VLAN Filter Table Array - RW Array */
#define REG_WUC      0x05800  /* Wakeup Control - RW */
#define REG_WUFC     0x05808  /* Wakeup Filter Control - RW */
#define REG_WUS      0x05810  /* Wakeup Status - RO */
#define REG_MANC     0x05820  /* Management Control - RW */
#define REG_IPAV     0x05838  /* IP Address Valid - RW */
#define REG_IP4AT    0x05840  /* IPv4 Address Table - RW Array */
#define REG_IP6AT    0x05880  /* IPv6 Address Table - RW Array */
#define REG_WUPL     0x05900  /* Wakeup Packet Length - RW */
#define REG_WUPM     0x05A00  /* Wakeup Packet Memory - RO A */
#define REG_FFLT     0x05F00  /* Flexible Filter Length Table - RW Array */
#define REG_HOST_IF  0x08800  /* Host Interface */
#define REG_FFMT     0x09000  /* Flexible Filter Mask Table - RW Array */
#define REG_FFVT     0x09800  /* Flexible Filter Value Table - RW Array */

#define REG_KUMCTRLSTA 0x00034    /* MAC-PHY interface - RW */
#define REG_MDPHYA     0x0003C    /* PHY address - RW */
#define REG_MANC2H     0x05860    /* Management Control To Host - RW */
#define REG_SW_FW_SYNC 0x05B5C    /* Software-Firmware Synchronization - RW */

#define REG_GCR       0x05B00 /* PCI-Ex Control */
#define REG_GSCL_1    0x05B10 /* PCI-Ex Statistic Control #1 */
#define REG_GSCL_2    0x05B14 /* PCI-Ex Statistic Control #2 */
#define REG_GSCL_3    0x05B18 /* PCI-Ex Statistic Control #3 */
#define REG_GSCL_4    0x05B1C /* PCI-Ex Statistic Control #4 */
#define REG_FACTPS    0x05B30 /* Function Active and Power State to MNG */
#define REG_SWSM      0x05B50 /* SW Semaphore */
#define REG_FWSM      0x05B54 /* FW Semaphore */
#define REG_FFLT_DBG  0x05F04 /* Debug Register */
#define REG_HICR      0x08F00 /* Host Interface Control */

/* RSS registers */
#define REG_CPUVEC    0x02C10 /* CPU Vector Register - RW */
#define REG_MRQC      0x05818 /* Multiple Receive Control - RW */
#define REG_RETA      0x05C00 /* Redirection Table - RW Array */
#define REG_RSSRK     0x05C80 /* RSS Random Key - RW Array */
#define REG_RSSIM     0x05864 /* RSS Interrupt Mask */
#define REG_RSSIR     0x05868 /* RSS Interrupt Request */






#define RCTL_EN                         (1 << 1)    /* Receiver Enable */
#define RCTL_SBP                        (1 << 2)    /* Store Bad Packets */
#define RCTL_UPE                        (1 << 3)    /* Unicast Promiscuous Enabled */
#define RCTL_MPE                        (1 << 4)    /* Multicast Promiscuous Enabled */
#define RCTL_LPE                        (1 << 5)    /* Long Packet Reception Enable */
#define RCTL_LBM_NONE                   (0 << 6)    /* No Loopback */
#define RCTL_LBM_PHY                    (3 << 6)    /* PHY or external SerDesc loopback */
#define RTCL_RDMTS_HALF                 (0 << 8)    /* Free Buffer Threshold is 1/2 of RDLEN */
#define RTCL_RDMTS_QUARTER              (1 << 8)    /* Free Buffer Threshold is 1/4 of RDLEN */
#define RTCL_RDMTS_EIGHTH               (2 << 8)    /* Free Buffer Threshold is 1/8 of RDLEN */
#define RCTL_MO_36                      (0 << 12)   /* Multicast Offset - bits 47:36 */
#define RCTL_MO_35                      (1 << 12)   /* Multicast Offset - bits 46:35 */
#define RCTL_MO_34                      (2 << 12)   /* Multicast Offset - bits 45:34 */
#define RCTL_MO_32                      (3 << 12)   /* Multicast Offset - bits 43:32 */
#define RCTL_BAM                        (1 << 15)   /* Broadcast Accept Mode */
#define RCTL_VFE                        (1 << 18)   /* VLAN Filter Enable */
#define RCTL_CFIEN                      (1 << 19)   /* Canonical Form Indicator Enable */
#define RCTL_CFI                        (1 << 20)   /* Canonical Form Indicator Bit Value */
#define RCTL_DPF                        (1 << 22)   /* Discard Pause Frames */
#define RCTL_PMCF                       (1 << 23)   /* Pass MAC Control Frames */
#define RCTL_SECRC                      (1 << 26)   /* Strip Ethernet CRC */

#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384                ((1 << 16) | (1 << 25))

#define TCTL_EN                         (1 << 1)    /* Transmit Enable */
#define TCTL_PSP                        (1 << 3)    /* Pad Short Packets */
#define TCTL_CT_SHIFT                   4           /* Collision Threshold */
#define TCTL_COLD_SHIFT                 12          /* Collision Distance */
#define TCTL_SWXOFF                     (1 << 22)   /* Software XOFF Transmission */
#define TCTL_RTLC                       (1 << 24)   /* Re-transmit on Late Collision */

#define CMD_EOP                         (1 << 0)    /* End of Packet */
#define CMD_IFCS                        (1 << 1)    /* Insert FCS */
#define CMD_IC                          (1 << 2)    /* Insert Checksum */
#define CMD_RS                          (1 << 3)    /* Report Status */
#define CMD_RPS                         (1 << 4)    /* Report Packet Sent */
#define CMD_VLE                         (1 << 6)    /* VLAN Packet Enable */
#define CMD_IDE                         (1 << 7)    /* Interrupt Delay Enable */

