#include <qnu/log.h>
#include <qnu/pci.h>
/* Disk scaffold: detect AHCI/NVMe/virtio-blk; full drivers/block IO not yet implemented */
void disk_init(void){
    log_write("[DISK] storage scan (scaffold)\n");
    pci_storage_dump();
    log_write("[DISK] AHCI/NVMe/virtio-blk drivers: not implemented yet\n");
}
