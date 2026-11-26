#ifndef QNU_VFS_H
#define QNU_VFS_H
#include <qnu/types.h>
#define VFS_FILE 1
#define VFS_DIR 2
#define VFS_SUBVOL 4
#define VFS_COW 8
#define VFS_DEV 16
typedef struct vfs_node {
    char name[64]; u32 type; u32 flags; u64 size; u8 *data; u32 cap; u32 checksum;
    struct vfs_node *parent; struct vfs_node *children; struct vfs_node *next;
} vfs_node_t;
void vfs_init(void);
vfs_node_t *vfs_root(void);
vfs_node_t *vfs_lookup(const char *path);
vfs_node_t *vfs_create(const char *path, u32 type);
vfs_node_t *vfs_mkdir(const char *path);
vfs_node_t *vfs_subvol_create(const char *path);
int vfs_snapshot(const char *src, const char *dst);
int vfs_read(vfs_node_t *n, u64 off, void *buf, u64 len);
int vfs_write(vfs_node_t *n, u64 off, const void *buf, u64 len);
u32 vfs_checksum(vfs_node_t *n);
void vfs_ls(const char *path);
void vfs_info(const char *path);
int vfs_readdir(vfs_node_t *dir, u32 idx, char *name, int max, u32 *type);
int vfs_remove(const char *path);
int vfs_copy(const char *src, const char *dst);
int vfs_rename(const char *src, const char *dst);
int vfs_mount_fat32(const char *blk,unsigned long long first_lba,const char *target);
#endif
