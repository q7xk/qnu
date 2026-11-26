#include "../lib/syscall.h"
int main(int argc,char**argv){ (void)argc;(void)argv; const char m[]="QNU init: starting /bin/sh\n"; write(1,m,sizeof(m)-1); for(;;){ /* in real system we'd exec sh; here just idle */ } return 0; }
