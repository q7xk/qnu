#include "../lib/syscall.h"
int main(int argc,char**argv){ (void)argc;(void)argv; const char *m="qnu-sh: minimal user shell (scaffold)\n"; int l=0; while(m[l]) l++; write(1,m,l); return 0; }
