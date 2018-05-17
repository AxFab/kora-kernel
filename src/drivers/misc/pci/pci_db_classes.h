/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
case 0x000000:
return "Any device except for VGA-Compatible devices";
case 0x000100:
return "VGA-Compatible Device";
case 0x010000:
return "SCSI Bus Controller";
case 0x010200:
return "Floppy Disk Controller";
case 0x010300:
return "IPI Bus Controller";
case 0x010400:
return "RAID Controller";
case 0x010520:
return "ATA Controller (Single DMA)";
case 0x010530:
return "ATA Controller (Chained DMA)";
case 0x010600:
return "Serial ATA (Vendor Specific Interface)";
case 0x010601:
return "Serial ATA (AHCI 1.0)";
case 0x010700:
return "Serial Attached SCSI (SAS)";
case 0x018000:
return "Other Mass Storage Controller";
case 0x020000:
return "Ethernet Controller";
case 0x020100:
return "Token Ring Controller";
case 0x020200:
return "FDDI Controller";
case 0x020300:
return "ATM Controller";
case 0x020400:
return "ISDN Controller";
case 0x020500:
return "WorldFip Controller";
case 0x028000:
return "Other Network Controller";
case 0x030000:
return "VGA-Compatible Controller";
case 0x030001:
return "8512-Compatible Controller";
case 0x030100:
return "XGA Controller";
case 0x030200:
return "3D Controller (Not VGA-Compatible)";
case 0x038000:
return "Other Display Controller";
case 0x040000:
return "Video Device";
case 0x040100:
return "Audio Device";
case 0x040200:
return "Computer Telephony Device";
case 0x048000:
return "Other Multimedia Device";
case 0x050000:
return "RAM Controller";
case 0x050100:
return "Flash Controller";
case 0x058000:
return "Other Memory Controller";
case 0x060000:
return "Host Bridge";
case 0x060100:
return "ISA Bridge";
case 0x060200:
return "EISA Bridge";
case 0x060300:
return "MCA Bridge";
case 0x060400:
return "PCI-to-PCI Bridge";
case 0x060401:
return "PCI-to-PCI Bridge (Subtractive Decode)";
case 0x060500:
return "PCMCIA Bridge";
case 0x060600:
return "NuBus Bridge";
case 0x060700:
return "CardBus Bridge";
case 0x060940:
return "PCI-to-PCI Bridge (Semi-Transparent, Primary)";
case 0x060980:
return "PCI-to-PCI Bridge (Semi-Transparent, Secondary)";
case 0x060A00:
return "InfiniBrand-to-PCI Host Bridge";
case 0x068000:
return "Other Bridge Device";
case 0x070000:
return "Generic XT-Compatible Serial Controller";
case 0x070001:
return "16450-Compatible Serial Controller";
case 0x070002:
return "16550-Compatible Serial Controller";
case 0x070003:
return "16650-Compatible Serial Controller";
case 0x070004:
return "16750-Compatible Serial Controller";
case 0x070005:
return "16850-Compatible Serial Controller";
case 0x070006:
return "16950-Compatible Serial Controller";
case 0x070100:
return "Parallel Port";
case 0x070101:
return "Bi-Directional Parallel Port";
case 0x070102:
return "ECP 1.X Compliant Parallel Port";
case 0x070103:
return "IEEE 1284 Controller";
case 0x0701FE:
return "IEEE 1284 Target Device";
case 0x070200:
return "Multiport Serial Controller";
case 0x070300:
return "Generic Modem";
case 0x070301:
return "Hayes Compatible Modem (16450-Compatible Interface)";
case 0x070302:
return "Hayes Compatible Modem (16550-Compatible Interface)";
case 0x070303:
return "Hayes Compatible Modem (16650-Compatible Interface)";
case 0x070304:
return "Hayes Compatible Modem (16750-Compatible Interface)";
case 0x070400:
return "IEEE 488.1/2 (GPIB) Controller";
case 0x070500:
return "Smart Card";
case 0x078000:
return "Other Communications Device";
case 0x080000:
return "Generic 8259 PIC";
case 0x080001:
return "ISA PIC";
case 0x080002:
return "EISA PIC";
case 0x080010:
return "I/O APIC Interrupt Controller";
case 0x080020:
return "I/O(x) APIC Interrupt Controller";
case 0x080100:
return "Generic 8237 DMA Controller";
case 0x080101:
return "ISA DMA Controller";
case 0x080102:
return "EISA DMA Controller";
case 0x080200:
return "Generic 8254 System Timer";
case 0x080201:
return "ISA System Timer";
case 0x080202:
return "EISA System Timer";
case 0x080300:
return "Generic RTC Controller";
case 0x080301:
return "ISA RTC Controller";
case 0x080400:
return "Generic PCI Hot-Plug Controller";
case 0x088000:
return "Other System Peripheral";
case 0x090000:
return "Keyboard Controller";
case 0x090100:
return "Digitizer";
case 0x090200:
return "Mouse Controller";
case 0x090300:
return "Scanner Controller";
case 0x090400:
return "Gameport Controller (Generic)";
case 0x090410:
return "Gameport Controller (Legacy)";
case 0x098000:
return "Other Input Controller";
case 0x0A0000:
return "Generic Docking Station";
case 0x0A8000:
return "Other Docking Station";
case 0x0B0000:
return "386 Processor";
case 0x0B0100:
return "486 Processor";
case 0x0B0200:
return "Pentium Processor";
case 0x0B1000:
return "Alpha Processor";
case 0x0B2000:
return "PowerPC Processor";
case 0x0B3000:
return "MIPS Processor";
case 0x0B4000:
return "Co-Processor";
case 0x0C0000:
return "IEEE 1394 Controller (FireWire)";
case 0x0C0010:
return "IEEE 1394 Controller (1394 OpenHCI Spec)";
case 0x0C0100:
return "ACCESS.bus";
case 0x0C0200:
return "SSA";
case 0x0C0300:
return "USB (Universal Host Controller Spec)";
case 0x0C0310:
return "USB (Open Host Controller Spec)";
case 0x0C0320:
return "USB2 Host Controller (Intel Enhanced Host Controller Interface)";
case 0x0C0330:
return "USB3 XHCI Controller";
case 0x0C0380:
return "Unspecified USB Controller";
case 0x0C03FE:
return "USB (Not Host Controller)";
case 0x0C0400:
return "Fibre Channel";
case 0x0C0500:
return "SMBus";
case 0x0C0600:
return "InfiniBand";
case 0x0C0700:
return "IPMI SMIC Interface";
case 0x0C0701:
return "IPMI Kybd Controller Style Interface";
case 0x0C0702:
return "IPMI Block Transfer Interface";
case 0x0C0800:
return "SERCOS Interface Standard (IEC 61491)";
case 0x0C0900:
return "CANbus";
case 0x0D0000:
return "iRDA Compatible Controller";
case 0x0D0100:
return "Consumer IR Controller";
case 0x0D1000:
return "RF Controller";
case 0x0D1100:
return "Bluetooth Controller";
case 0x0D1200:
return "Broadband Controller";
case 0x0D2000:
return "Ethernet Controller (802.11a)";
case 0x0D2100:
return "Ethernet Controller (802.11b)";
case 0x0D8000:
return "Other Wireless Controller";
case 0x0E0000:
return "Message FIFO";
case 0x0F0100:
return "TV Controller";
case 0x0F0200:
return "Audio Controller";
case 0x0F0300:
return "Voice Controller";
case 0x0F0400:
return "Data Controller";
case 0x100000:
return "Network and Computing Encrpytion/Decryption";
case 0x101000:
return "Entertainment Encryption/Decryption";
case 0x108000:
return "Other Encryption/Decryption";
case 0x110000:
return "DPIO Modules";
case 0x110100:
return "Performance Counters";
case 0x111000:
return "Communications Syncrhonization Plus Time and Frequency Test/Measurment";
case 0x112000:
return "Management Card";
case 0x118000:
return "Other Data Acquisition/Signal Processing Controller";
