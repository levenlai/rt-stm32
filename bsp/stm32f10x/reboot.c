#include "stm32f10x.h"
#include "rtthread.h"
#include <rtgui/driver.h>


#define printk          rt_kprintf
#define AIRCR_VECTKEY_MASK    ((uint32_t)0x05FA0000)

#ifdef RT_USING_FINSH
#include <finsh.h>

void reboot(void)
{
    printk("System reset..., %08X\r\n", SCB->AIRCR);

    SCB->AIRCR = AIRCR_VECTKEY_MASK | 0x04;
}
FINSH_FUNCTION_EXPORT(reboot, reset the system);
#endif

