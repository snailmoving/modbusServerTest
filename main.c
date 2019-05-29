#include <stdio.h>
#include "modbusTcp.h"
int main() {
    printf("Hello, World!\n");
    setDevice();
    modbusCycle();
    closeDevice();
    return 0;
}