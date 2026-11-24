#ifndef QNU_ACPI_H
#define QNU_ACPI_H
#include <qnu/types.h>
void acpi_init(void);
void acpi_dump(void);
void *acpi_find_sdt(const char sig[4]);
#endif
