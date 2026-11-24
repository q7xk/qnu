#include "../lib/syscall.h"
static int sl(const char*s){int n=0;while(s[n])n++;return n;}
int main(int argc,char**argv){
    (void)argc;(void)argv;
    /* no argv setup yet; just print placeholder */
    const char m[]="echo: argv not wired yet\n";
    write(1,m,sizeof(m)-1);
    return 0;
}
