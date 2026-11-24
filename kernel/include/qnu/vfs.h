#ifndef QNU_VFS_H
#define QNU_VFS_H
#include <qnu/types.h>
#define VFS_FILE 1
#define VFS_DIR 2
typedef struct vfs_node {
    char name[64]; u32 type; u64 size; u8 *data; u32 cap;
    struct vfs_node *parent; struct vfs_node *children; struct vfs_node *next;
} vfs_node_t;
void vfs_init(void);
vfs_node_t *vfs_root(void);
vfs_node_t *vfs_lookup(const char *path);
vfs_node_t *vfs_create(const char *path, u32 type);
int vfs_read(vfs_node_t *n, u64 off, void *buf, u64 len);
int vfs_write(vfs_node_t *n, u64 off, const void *buf, u64 len);
void vfs_ls(const char *path);
#endif
