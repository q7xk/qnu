#include <qnu/vfs.h>
#include <qnu/mm.h>
#include <qnu/log.h>

static vfs_node_t root;
static int str_eq(const char *a,const char*b){while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;}
static int str_len(const char*s){int n=0;while(s[n])n++;return n;}
static void str_cp(char*d,const char*s,int m){int i=0;for(;s[i]&&i<m-1;i++)d[i]=s[i];d[i]=0;}

void vfs_init(void){ for(int i=0;i<64;i++)root.name[i]=0; str_cp(root.name,"/",64); root.type=VFS_DIR; root.size=0; root.data=0; root.cap=0; root.parent=&root; root.children=0; root.next=0; log_ok("VFS initialized (ramfs)"); }
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

vfs_node_t *vfs_create(const char *path,u32 type){
    if(!path||path[0]!='/') return NULL;
    char parent[128]; char name[64]; int len=str_len(path);
    int last=-1; for(int i=0;i<len;i++) if(path[i]=='/') last=i;
    if(last<=0){ str_cp(parent,"/",128); str_cp(name,path+1,64); }
    else { int j=0; for(;j<last&&j<127;j++) parent[j]=path[j]; parent[j]=0; str_cp(name,path+last+1,64); }
    vfs_node_t *dir=vfs_lookup(parent); if(!dir||dir->type!=VFS_DIR) return NULL;
    if(find_child(dir,name)) return find_child(dir,name);
    vfs_node_t *n=(vfs_node_t*)kmalloc(sizeof(vfs_node_t)); if(!n) return NULL;
    for(int i=0;i<64;i++)n->name[i]=0; str_cp(n->name,name,64);
    n->type=type; n->size=0; n->data=0; n->cap=0; n->parent=dir; n->children=0; n->next=dir->children; dir->children=n;
    return n;
}

int vfs_read(vfs_node_t *n,u64 off,void *buf,u64 len){
    if(!n||n->type!=VFS_FILE) return -1; if(off>=n->size) return 0;
    if(off+len>n->size) len=n->size-off;
    u8 *b=(u8*)buf; for(u64 i=0;i<len;i++) b[i]=n->data[off+i]; return (int)len;
}

int vfs_write(vfs_node_t *n,u64 off,const void *buf,u64 len){
    if(!n||n->type!=VFS_FILE) return -1;
    u64 need=off+len;
    if(need>n->cap){ u32 nc=(u32)(need+64); u8*nd=(u8*)kmalloc(nc); if(!nd) return -1; for(u32 i=0;i<n->cap;i++)nd[i]=n->data[i]; n->data=nd; n->cap=nc; }
    const u8*b=(const u8*)buf; for(u64 i=0;i<len;i++) n->data[off+i]=b[i];
    if(need>n->size) n->size=need; return (int)len;
}

void vfs_ls(const char *path){
    vfs_node_t *d=vfs_lookup(path); if(!d){ log_write("ls: not found\n"); return; }
    if(d->type!=VFS_DIR){ log_printf("%s\n", d->name); return; }
    for(vfs_node_t*c=d->children;c;c=c->next) log_printf("%s%s\n", c->name, c->type==VFS_DIR?"/":"");
}
