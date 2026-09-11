#include "app_main.h"
#include "hardware.h"

void app_main(void) 
{
    hardware_init();  
    app_start();
}
