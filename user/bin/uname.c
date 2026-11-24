#include "../lib/syscall.h"
struct uts { char sys[65],node[65],rel[65],ver[65],mach[65]; };
static int sl(const char*s){int n=0;while(s[n])n++;return n;}
int main(int argc,char**argv){ (void)argc;(void)argv; struct uts u; uname(&u); write(1,u.sys,sl(u.sys)); write(1," ",1); write(1,u.rel,sl(u.rel)); write(1," ",1); write(1,u.mach,sl(u.mach)); write(1,"\n",1); return 0; }
