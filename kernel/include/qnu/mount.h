#ifndef QNU_MOUNT_H
#define QNU_MOUNT_H
#include <qnu/types.h>
typedef struct { char source[16]; char target[64]; char fstype[16]; u64 first_lba; } mount_t;
int mount_add(const char *source,const char *target,const char *fstype,u64 first_lba);
mount_t *mount_lookup(const char *target);
void mount_list(void);
#endif
