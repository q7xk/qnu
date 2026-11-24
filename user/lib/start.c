#include "syscall.h"
int main(int argc,char**argv);
void _start(void){
    /* stack: argc, argv... for now kernel doesn't set it up; call main(0,0) */
    int r = main(0,0);
    exit(r);
}
