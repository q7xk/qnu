#ifndef QNU_ACPI_H
#define QNU_ACPI_H
#include <qnu/types.h>
void acpi_init(void);
void acpi_dump(void);
void *acpi_find_sdt(const char sig[4]);
u32 acpi_madt_lapic(void);
u32 acpi_madt_ioapic(void);
u32 acpi_madt_cpus(void);
#endif
