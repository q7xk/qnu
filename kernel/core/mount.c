#include <qnu/mount.h>
#include <qnu/log.h>
#define MAX_MOUNT 16
static mount_t mounts[MAX_MOUNT]; static int count=0;
static void cp(char*d,const char*s,int m){int i=0;for(;s[i]&&i<m-1;i++)d[i]=s[i];d[i]=0;}
int mount_add(const char*source,const char*target,const char*fstype,u64 first_lba){
    if(count>=MAX_MOUNT) return -1; mount_t*m=&mounts[count++]; cp(m->source,source,16); cp(m->target,target,64); cp(m->fstype,fstype,16); m->first_lba=first_lba; log_printf("[mount] %s on %s type %s\n",source,target,fstype); return 0;
}
mount_t *mount_lookup(const char*target){ for(int i=0;i<count;i++){int j=0;while(mounts[i].target[j]&&target[j]&&mounts[i].target[j]==target[j])j++;if(mounts[i].target[j]==0&&target[j]==0)return &mounts[i];}return 0; }
void mount_list(void){ for(int i=0;i<count;i++) log_printf("%s on %s type %s\n",mounts[i].source,mounts[i].target,mounts[i].fstype); }
