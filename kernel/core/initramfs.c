#include <qnu/vfs.h>
#include <qnu/log.h>
#include <qnu/types.h>

/* Minimal CPIO newc parser stub - initramfs can be loaded when a blob is provided */
typedef struct { char c_magic[6],c_ino[8],c_mode[8],c_uid[8],c_gid[8],c_nlink[8],c_mtime[8],c_filesize[8],c_devmajor[8],c_devminor[8],c_rdevmajor[8],c_rdevminor[8],c_namesize[8],c_check[8]; } __attribute__((packed)) cpio_newc_t;
static u32 hexn(const char *s,int n){ u32 v=0; for(int i=0;i<n;i++){ char c=s[i]; v<<=4; if(c>='0'&&c<='9')v|=c-'0'; else if(c>='a'&&c<='f')v|=c-'a'+10; else if(c>='A'&&c<='F')v|=c-'A'+10; } return v; }

int initramfs_load(const u8 *blob, u64 size){
    u64 off=0; int files=0;
    while(off + sizeof(cpio_newc_t) < size){
        const cpio_newc_t *h=(const cpio_newc_t*)(blob+off);
        if(h->c_magic[0]!='0'||h->c_magic[1]!='7'||h->c_magic[2]!='0'||h->c_magic[3]!='7'||h->c_magic[4]!='0'||h->c_magic[5]!='1') break;
        u32 namesz=hexn(h->c_namesize,8), filesz=hexn(h->c_filesize,8);
        u64 name_off=off+sizeof(cpio_newc_t);
        const char *name=(const char*)(blob+name_off);
        if(namesz>=11 && name[0]=='T'&&name[1]=='R'&&name[2]=='A'&&name[3]=='I'&&name[4]=='L'&&name[5]=='E'&&name[6]=='R') break;
        u64 data_off=(name_off+namesz+3)&~3ULL;
        char path[128]; int pi=0; path[pi++]='/';
        for(u32 i=0; name[i] && pi<126; i++){ if(i==0 && name[i]=='.') continue; if(i==1 && name[i]=='/') continue; path[pi++]=name[i]; }
        path[pi]=0;
        if(filesz>0){
            vfs_node_t *n=vfs_create(path,VFS_FILE);
            if(n) vfs_write(n,0,blob+data_off,filesz);
        } else { vfs_mkdir(path); }
        files++;
        off=(data_off+filesz+3)&~3ULL;
    }
    log_printf("[OK] initramfs loaded (%d entries)\n", files);
    return files;
}
