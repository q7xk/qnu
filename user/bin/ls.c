#include "../lib/syscall.h"
typedef struct { unsigned long ino; long off; unsigned short reclen; unsigned char type; char name[]; } dent_t;
int main(int argc,char**argv){ (void)argc;(void)argv; int fd=open("/",0); if(fd<0){ const char*m="ls: /\n"; int l=0; while(m[l])l++; write(1,m,l); return 1; } char buf[512]; long n; while((n=sys3(217,fd,(long)buf,512))>0){ long pos=0; while(pos<n){ dent_t*d=(dent_t*)(buf+pos); int l=0; while(d->name[l]) l++; write(1,d->name,l); write(1,"\n",1); pos+=d->reclen; } } close(fd); return 0; }
