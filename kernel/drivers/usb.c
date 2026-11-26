#include <qnu/log.h>
#include <qnu/pci.h>
#include <qnu/types.h>
/* USB scaffold: detect controllers, real xHCI/EHCI/UHCI/HID stack not yet implemented */
void usb_init(void){
    log_write("[USB] controller scan (scaffold)\n");
    pci_usb_dump(); /* will add in pci */
    log_write("[USB] xHCI/HID stack: not implemented yet\n");
}
