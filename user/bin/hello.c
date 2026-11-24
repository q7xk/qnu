#include "../lib/syscall.h"
int main(int argc,char**argv){ (void)argc;(void)argv; const char m[]="Hello from QNU ring3!\n"; write(1,m,sizeof(m)-1); return 0; }
