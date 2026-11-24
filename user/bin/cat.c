#include "../lib/syscall.h"
int main(int argc,char**argv){
    (void)argc;(void)argv;
    const char p[]="/hello.txt";
    int fd=open(p,0);
    if(fd<0){ const char e[]="cat: open failed\n"; write(1,e,sizeof(e)-1); return 1; }
    char buf[128]; i64 r;
    while((r=read(fd,buf,128))>0) write(1,buf,(u64)r);
    close(fd);
    return 0;
}
