#include <qnu/vfs.h>
#include <qnu/mm.h>
#include <qnu/log.h>

static vfs_node_t root;
static int str_eq(const char *a,const char*b){while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;}
static int str_len(const char*s){int n=0;while(s[n])n++;return n;}
static void str_cp(char*d,const char*s,int m){int i=0;for(;s[i]&&i<m-1;i++)d[i]=s[i];d[i]=0;}

extern u32 rust_crc32(const u8 *data, u64 len);
static u32 crc32(const u8 *data, u64 len){ return rust_crc32(data,len); }

static vfs_node_t *new_node(const char *name,u32 type,u32 flags){
    vfs_node_t *n=(vfs_node_t*)kmalloc(sizeof(vfs_node_t)); if(!n) return NULL;
    for(int i=0;i<64;i++)n->name[i]=0; str_cp(n->name,name,64);
    n->type=type; n->flags=flags; n->size=0; n->data=0; n->cap=0; n->checksum=0; n->parent=0; n->children=0; n->next=0;
    return n;
}

void vfs_init(void){
    for(int i=0;i<64;i++)root.name[i]=0; str_cp(root.name,"/",64);
    root.type=VFS_DIR; root.flags=VFS_SUBVOL; root.size=0; root.data=0; root.cap=0; root.checksum=0; root.parent=&root; root.children=0; root.next=0;
    log_ok("VFS initialized (btrfs-like ramfs: subvol/cow/checksum)");
}

vfs_node_t *vfs_root(void){ return &root; }
static vfs_node_t *find_child(vfs_node_t *dir,const char *name){ for(vfs_node_t*c=dir->children;c;c=c->next) if(str_eq(c->name,name)) return c; return NULL; }

vfs_node_t *vfs_lookup(const char *path){
    if(!path || !*path) return NULL;
    if(str_eq(path,"/")) return &root;
    vfs_node_t *cur=&root; char part[64]; int pi=0; int i = (path[0]=='/')?1:0;
    for(; ;i++){
        char ch=path[i];
        if(ch=='/'||ch==0){ if(pi>0){ part[pi]=0; if(cur->type!=VFS_DIR) return NULL; cur=find_child(cur,part); if(!cur) return NULL; pi=0; } if(ch==0) break; }
        else if(pi<63) part[pi++]=ch;
    }
    return cur;
}

static int split_parent(const char *path,char *parent,int ps,char *name,int ns){
    if(!path||path[0]!='/') return 0; int len=str_len(path); int last=-1; for(int i=0;i<len;i++) if(path[i]=='/') last=i;
    if(last<=0){ str_cp(parent,"/",ps); str_cp(name,path+1,ns); } else { int j=0; for(;j<last&&j<ps-1;j++) parent[j]=path[j]; parent[j]=0; str_cp(name,path+last+1,ns); }
    return 1;
}

vfs_node_t *vfs_mkdir(const char *path){ return vfs_create(path, VFS_DIR); }

vfs_node_t *vfs_create(const char *path,u32 type){
    char parent[128], name[64]; if(!split_parent(path,parent,128,name,64)) return NULL;
    vfs_node_t *dir=vfs_lookup(parent); if(!dir||dir->type!=VFS_DIR) return NULL;
    vfs_node_t *ex=find_child(dir,name); if(ex) return ex;
    vfs_node_t *n=new_node(name,type,(type==VFS_FILE)?VFS_COW:0); if(!n) return NULL;
    n->parent=dir; n->next=dir->children; dir->children=n; return n;
}

vfs_node_t *vfs_subvol_create(const char *path){
    vfs_node_t *n=vfs_create(path,VFS_DIR); if(n) n->flags |= VFS_SUBVOL;
    return n;
}

static vfs_node_t *clone_node(vfs_node_t *src, vfs_node_t *parent){
    vfs_node_t *n=new_node(src->name,src->type,src->flags); if(!n) return NULL;
    n->size=src->size; n->cap=src->cap; n->checksum=src->checksum; n->parent=parent;
    if(src->data && src->cap){ n->data=(u8*)kmalloc(src->cap); for(u32 i=0;i<src->cap;i++) n->data[i]=src->data[i]; }
    vfs_node_t *prev=NULL;
    for(vfs_node_t *c=src->children;c;c=c->next){ vfs_node_t *cc=clone_node(c,n); cc->next=NULL; if(!prev) n->children=cc; else prev->next=cc; prev=cc; }
    return n;
}

int vfs_snapshot(const char *src, const char *dst){
    vfs_node_t *s=vfs_lookup(src); if(!s||!(s->flags&VFS_SUBVOL)) return -1;
    char parent[128], name[64]; if(!split_parent(dst,parent,128,name,64)) return -1;
    vfs_node_t *dir=vfs_lookup(parent); if(!dir||dir->type!=VFS_DIR) return -1;
    if(find_child(dir,name)) return -1;
    vfs_node_t *n=clone_node(s,dir); if(!n) return -1;
    str_cp(n->name,name,64); n->parent=dir; n->next=dir->children; dir->children=n; return 0;
}

int vfs_read(vfs_node_t *n,u64 off,void *buf,u64 len){
    if(!n||n->type!=VFS_FILE) return -1; if(off>=n->size) return 0;
    if(off+len>n->size) len=n->size-off;
    u8 *b=(u8*)buf; for(u64 i=0;i<len;i++) b[i]=n->data[off+i]; return (int)len;
}

int vfs_write(vfs_node_t *n,u64 off,const void *buf,u64 len){
    if(!n||n->type!=VFS_FILE) return -1;
    u64 need=off+len;
    if((n->flags&VFS_COW) && n->data){
        /* CoW: allocate new buffer */
        u32 nc=(u32)(need>n->cap?need+64:n->cap);
        u8*nd=(u8*)kmalloc(nc); if(!nd) return -1;
        for(u32 i=0;i<n->cap;i++) nd[i]=n->data[i];
        n->data=nd; n->cap=nc;
    } else if(need>n->cap){
        u32 nc=(u32)(need+64); u8*nd=(u8*)kmalloc(nc); if(!nd) return -1;
        for(u32 i=0;i<n->cap;i++)nd[i]=n->data[i]; n->data=nd; n->cap=nc;
    }
    const u8*b=(const u8*)buf; for(u64 i=0;i<len;i++) n->data[off+i]=b[i];
    if(need>n->size) n->size=need;
    n->checksum = crc32(n->data,n->size);
    return (int)len;
}

u32 vfs_checksum(vfs_node_t *n){ if(!n||n->type!=VFS_FILE) return 0; n->checksum=crc32(n->data,n->size); return n->checksum; }

void vfs_ls(const char *path){
    vfs_node_t *d=vfs_lookup(path); if(!d){ log_write("ls: not found\n"); return; }
    if(d->type!=VFS_DIR){ log_printf("%s%s size=%lu cs=%x\n", d->name, (d->flags&VFS_COW)?"[cow]":"", d->size, d->checksum); return; }
    for(vfs_node_t*c=d->children;c;c=c->next){
        const char *suf = c->type==VFS_DIR?"/":"";
        const char *sv = (c->flags&VFS_SUBVOL)?"[subvol]":"";
        const char *cow = (c->flags&VFS_COW)?"[cow]":"";
        log_printf("%s%s%s%s\n", c->name, suf, sv, cow);
    }
}

void vfs_info(const char *path){
    vfs_node_t *n=vfs_lookup(path); if(!n){ log_write("not found\n"); return; }
    log_printf("name=%s type=%s flags=%x size=%lu checksum=%x\n", n->name, n->type==VFS_DIR?"dir":"file", n->flags, n->size, n->checksum);
}

int vfs_readdir(vfs_node_t *dir, u32 idx, char *name, int max, u32 *type){
    if(!dir || dir->type!=VFS_DIR) return -1;
    u32 i=0; for(vfs_node_t*c=dir->children;c;c=c->next, i++){
        if(i==idx){ int j=0; for(;c->name[j] && j<max-1;j++) name[j]=c->name[j]; name[j]=0; if(type)*type=c->type; return 1; }
    }
    return 0;
}

int vfs_remove(const char *path){
    vfs_node_t *n=vfs_lookup(path); if(!n||n==&root) return -1;
    vfs_node_t *dir=n->parent; if(!dir) return -1;
    vfs_node_t *prev=NULL,*c=dir->children;
    while(c){ if(c==n){ if(prev) prev->next=c->next; else dir->children=c->next; return 0; } prev=c; c=c->next; }
    return -1;
}

int vfs_copy(const char *src, const char *dst){
    vfs_node_t *s=vfs_lookup(src); if(!s||s->type!=VFS_FILE) return -1;
    vfs_node_t *d=vfs_lookup(dst);
    if(!d) d=vfs_create(dst,VFS_FILE);
    if(!d||d->type!=VFS_FILE) return -1;
    d->size=0; d->checksum=0;
    return vfs_write(d,0,s->data,s->size)>=0?0:-1;
}

int vfs_rename(const char *src, const char *dst){
    vfs_node_t *n=vfs_lookup(src); if(!n||n==&root) return -1;
    char parent[128], name[64]; if(!split_parent(dst,parent,128,name,64)) return -1;
    vfs_node_t *ndir=vfs_lookup(parent); if(!ndir||ndir->type!=VFS_DIR) return -1;
    /* unlink */
    vfs_node_t *prev=NULL,*c=n->parent->children;
    while(c){ if(c==n){ if(prev) prev->next=c->next; else n->parent->children=c->next; break; } prev=c; c=c->next; }
    str_cp(n->name,name,64); n->parent=ndir; n->next=ndir->children; ndir->children=n; return 0;
}

