#include <qnu/shell.h>
#include <qnu/log.h>
#include <qnu/mm.h>
#include <qnu/boot.h>
#include <qnu/acpi.h>
#include <qnu/cpu.h>
#include <qnu/io.h>
#include <qnu/pci.h>
#include <qnu/boot_internal.h>
#include <limine.h>
#include <qnu/vfs.h>
#include <qnu/sched.h>
#include <qnu/proc.h>

static char buf[160];
static int len = 0;
static bool ready = false;
static char cwd[128] = "/";

static int streq(const char *a,const char*b){while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;}
static int starts(const char *s,const char*p){while(*p){if(*s++!=*p++)return 0;}return 1;}
static int slen(const char*s){int n=0;while(s[n])n++;return n;}
static void scp(char*d,const char*s,int m){int i=0;for(;s[i]&&i<m-1;i++)d[i]=s[i];d[i]=0;}
static const char *arg1(const char *cmd){ while(*cmd && *cmd!=' ') cmd++; while(*cmd==' ') cmd++; return cmd; }
static void next_arg(const char **p, char *out, int m){ int i=0; while(**p==' ') (*p)++; while(**p && **p!=' ' && i<m-1) out[i++]=*(*p)++; out[i]=0; }
static void resolve_path(const char *in, char *out, int m){
    if(!in || !*in){ scp(out,cwd,m); return; }
    if(in[0]=='/'){ scp(out,in,m); return; }
    int i=0; for(;cwd[i] && i<m-1;i++) out[i]=cwd[i];
    if(i>1 && out[i-1]!='/') out[i++]='/';
    for(int j=0; in[j] && i<m-1; j++) out[i++]=in[j];
    out[i]=0;
}

static void prompt(void){ log_printf("qnu:%s# ", cwd); }

static void run_cmd(const char *cmd) {
    char a[32]; int i=0; while(cmd[i] && cmd[i]!=' ' && i<31){a[i]=cmd[i];i++;} a[i]=0;
    const char *rest = arg1(cmd);
    char p1[128], p2[128], r1[128], r2[128]; const char *rp=rest; next_arg(&rp,p1,128); next_arg(&rp,p2,128); resolve_path(p1,r1,128); resolve_path(p2,r2,128);

    if (streq(a,"help")) {
        log_write("GNU/FreeBSD-like builtins: help uname echo pwd cd ls cat touch mkdir rm cp mv ps df mount env id whoami date uptime free sysctl info subvol snap wifi bluetooth pci acpi cpu mem heap memmap ticks reboot clear\n");
    } else if (streq(a,"uname")) {
        if(streq(p1,"-a")) log_write("QNU qnu 0.0.9 QNU Kernel x86_64\n");
        else log_write("QNU\n");
    } else if (streq(a,"echo")) { log_write(rest); log_write("\n"); }
    else if (streq(a,"pwd")) { log_write(cwd); log_write("\n"); }
    else if (streq(a,"cd")) { const char *t = *p1? r1 : "/"; vfs_node_t*n=vfs_lookup(t); if(n && n->type==VFS_DIR) scp(cwd,t,128); else log_write("cd: not found\n"); }
    else if (streq(a,"ls")) { vfs_ls(*p1?r1:cwd); }
    else if (streq(a,"cat")) { vfs_node_t*n=vfs_lookup(*p1?r1:"/hello.txt"); if(n){ char b[256]; int r=vfs_read(n,0,b,255); if(r>0){b[r]=0; log_write(b);} else log_write("\n"); } else log_write("cat: not found\n"); }
    else if (streq(a,"touch")) { if(*p1){ vfs_node_t*n=vfs_create(r1,VFS_FILE); if(n) log_write(""); else log_write("touch: err\n"); } }
    else if (streq(a,"mkdir")) { if(*p1){ if(vfs_mkdir(r1)){} else log_write("mkdir: err\n"); } }
    else if (streq(a,"rm")) { if(*p1){ if(vfs_remove(r1)==0){} else log_write("rm: err\n"); } }
    else if (streq(a,"cp")) { if(*p1&&*p2){ if(vfs_copy(r1,r2)==0){} else log_write("cp: err\n"); } }
    else if (streq(a,"mv")) { if(*p1&&*p2){ if(vfs_rename(r1,r2)==0){} else log_write("mv: err\n"); } }
    else if (streq(a,"ps")) { sched_ps(); }
    else if (streq(a,"df")) { const struct qnu_boot_info*bi=boot_info_get(); log_printf("Filesystem 1K-blocks Used Available Use%% Mounted\nramfs %lu %lu %lu 0%% /\n", bi->total_memory/1024, (bi->total_memory-bi->usable_memory)/1024, bi->usable_memory/1024); }
    else if (streq(a,"mount")) {
        if(*p1 && streq(p1,"-t")){
            /* mount -t fat32 <source> <target> */
            char dummy[16], fs[16], src[32], tgt[64]; const char *r=rest; next_arg(&r,dummy,16); next_arg(&r,fs,16); next_arg(&r,src,32); next_arg(&r,tgt,64);
            if(streq(fs,"fat32")){
                const char *blkname = src; if(starts(blkname,"/dev/")) blkname=src+5;
                extern int part_scan_mbr(const char*, void*, int);
                extern int fat32_fsck(const char*, unsigned long long);
                extern int mount_add(const char*,const char*,const char*,unsigned long long);
                u64 first=0; /* TODO partition scan simplified */
                if(mount_add(src,*tgt?tgt:"/mnt","fat32",first)==0){ fat32_fsck(blkname,first); log_write("mount: fat32 mounted (read-write API available; VFS read-through limited)\n"); } else log_write("mount: err\n");
            } else log_write("mount: only -t fat32 supported currently\n");
        } else { log_write("ramfs on / type ramfs (rw,subvol,cow,checksum)\n"); extern void mount_list(void); mount_list(); }
    }
    else if (streq(a,"umount")) { log_write("umount: not implemented\n"); }
    else if (streq(a,"fsck")) { extern int fat32_fsck(const char*,unsigned long long); const char *blkname=*p1?p1:"vda"; if(starts(blkname,"/dev/")) blkname=p1+5; if(fat32_fsck(blkname,0)==0) log_write("fsck: ok\n"); else log_write("fsck: err\n"); }
    else if (streq(a,"env")) { log_write("PATH=/bin\nHOME=/home/kullanici\nSHELL=/bin/qsh\nOS=QNU\n"); }
    else if (streq(a,"id")||streq(a,"whoami")) { log_write("kullanici uid=0 gid=0\n"); }
    else if (streq(a,"date")||streq(a,"uptime")) { extern u64 pit_get_ticks(void); u64 t=pit_get_ticks(); log_printf("uptime %lu seconds\n", t/100); }
    else if (streq(a,"free")) { const struct qnu_boot_info*bi=boot_info_get(); u64 t=0,u=0; heap_stats(&t,&u); log_printf("mem total=%luMB usable=%luMB free_pages=%lu heap_used=%lu\n", bi->total_memory/1024/1024, bi->usable_memory/1024/1024, pmm_free_pages(), u); }
    else if (streq(a,"sysctl")) { log_write("kernel.name=QNU\nkernel.version=0.0.9\nkernel.arch=x86_64\nvm.heap=slab+free-list\nfs.root=ramfs\n"); }
    else if (streq(a,"info")) { vfs_info(*p1?r1:cwd); }
    else if (streq(a,"subvol")) { if(*p1){ if(vfs_subvol_create(r1)){} else log_write("subvol: err\n"); } }
    else if (streq(a,"snap")) { if(*p1&&*p2){ if(vfs_snapshot(r1,r2)==0){} else log_write("snap: err\n"); } else log_write("usage: snap <src> <dst>\n"); }
    else if (streq(a,"wifi")) { pci_wireless_dump(); log_write("WiFi stack: not implemented\n"); }
    else if (streq(a,"bluetooth")) { pci_wireless_dump(); log_write("BLE stack: not implemented\n"); }
    else if (streq(a,"pci")) { pci_dump(); }
    else if (streq(a,"acpi")) { acpi_dump(); }
    else if (streq(a,"cpu")) { cpu_info_dump(); }
    else if (streq(a,"mem")) { const struct qnu_boot_info*bi=boot_info_get(); log_printf("total=%luMB usable=%luMB free_pages=%lu\n", bi->total_memory/1024/1024, bi->usable_memory/1024/1024, pmm_free_pages()); }
    else if (streq(a,"heap")) { u64 t=0,u=0; heap_stats(&t,&u); log_printf("heap total=%lu used=%lu free=%lu\n", t,u,t-u); }
    else if (streq(a,"memmap")) { struct limine_memmap_response *mm=boot_memmap(); if(!mm) log_write("no memmap\n"); else for(u64 i2=0;i2<mm->entry_count;i2++){ struct limine_memmap_entry*e=mm->entries[i2]; log_printf(" %x base=%lx len=%lx\n", e->type,e->base,e->length);} }
    else if (streq(a,"ticks")) { extern u64 pit_get_ticks(void); log_printf("ticks=%lu sched=%lu\n", pit_get_ticks(), sched_get_ticks()); }
    else if (streq(a,"reboot")) { log_write("Rebooting...\n"); struct {u16 l;u64 b;} __attribute__((packed)) idtr={0,0}; __asm__ volatile("lidt %0; int $0x03"::"m"(idtr)); }
    else if (streq(a,"clear")) { log_write("\n"); }
    else if (streq(a,"exec")) { if(*p1){ int rc=proc_exec(r1,NULL); log_printf("[exec] rc=%d\n", rc); } else log_write("usage: exec /bin/hello\n"); }
    else if (streq(a,"version")) { log_write("QNU Kernel 0.0.9\n"); }
    else if (len>0) { log_printf("sh: command not found: %s\n", a); }
}

void shell_init(void) { len=0; ready=true; scp(cwd,"/",128); log_write("\nQNU shell ready (GNU/FreeBSD-like builtins). Type 'help'.\n"); prompt(); }
void shell_input_char(char c) {
    if(!ready) return; if(c=='\r') c='\n';
    if(c=='\n'){ log_write("\n"); buf[len]=0; run_cmd(buf); len=0; prompt(); return; }
    if((c=='\b'||c==127)&&len>0){ len--; log_write("\b \b"); return; }
    if(c>=32 && c<127 && len<159){ buf[len++]=c; char s[2]={c,0}; log_write(s); }
}
void shell_poll(void) { if (inb(0x3FD) & 1) { char c=(char)inb(0x3F8); shell_input_char(c); } }
