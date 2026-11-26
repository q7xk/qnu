#ifndef QNU_PCI_H
#define QNU_PCI_H
#include <qnu/types.h>
void pci_init(void);
void pci_dump(void);
void pci_wireless_dump(void);
void pci_usb_dump(void);
void pci_storage_dump(void);
u32 pci_read32(u8 bus,u8 dev,u8 func,u8 off);
void pci_write32(u8 bus,u8 dev,u8 func,u8 off,u32 val);
#endif
