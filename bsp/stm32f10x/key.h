#ifndef __KEY_H__
#define __KEY_H__

#define RT_KEYBOARD_READ_VALUE              0x01


/*
 * define the virtual key value
*/

#define VK_KEY_UP                           0x80000000
#define VK_KEY_DOWN                         0x00000000

/*define MAX key number is 256*/
#define VK_MASK                             0xFF

#define VK_NULL                             0
#define VK_F1                               1
#define VK_F2                               2
#define VK_ENTER                            10
#define VK_ESCAPE                           11

void rt_keyboard_hw_init(void);

#endif


