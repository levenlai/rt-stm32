#include "stm32f10x.h"
#include "board.h"

#include <rtthread.h>
#include <rtgui/event.h>
#include <rtgui/kbddef.h>
#include <rtgui/rtgui_server.h>

#include "key.h"

struct key_device
{
    struct rt_device parent;
    rt_timer_t poll_timer;
};

static struct key_device *key = RT_NULL;

static void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable the EXTI0 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 9;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

rt_inline void EXTI_Enable(rt_uint32_t enable)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    /* Configure  EXTI  */
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//FallingÏÂ½µÑØ RisingÉÏÉý

    if (enable)
    {
        /* enable */
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    }
    else
    {
        /* disable */
        EXTI_InitStructure.EXTI_LineCmd = DISABLE;
    }

    EXTI_Init(&EXTI_InitStructure);
    EXTI_ClearITPendingBit(EXTI_Line0);
}

static void EXTI_Configuration(void)
{
    /* PB1 touch INT */
    {
        GPIO_InitTypeDef GPIO_InitStructure;
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOA,&GPIO_InitStructure);
    }

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);

    /* Configure  EXTI  */
    EXTI_Enable(1);
}

void EXTI0_IRQHandler(void)
{
    EXTI_Enable(0);
    
    if(key != RT_NULL)
    {
        rt_timer_start(key->poll_timer);
    }

    EXTI_ClearITPendingBit(EXTI_Line0);
}

void Keypad_timeout(void* parameter)
{
    static unsigned char key_down = 0, key_cnt = 0;
    int tmer = RT_TICK_PER_SECOND / 10;
    struct rtgui_event_kbd kbdevent;
    
    if ((!key_down) && GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) != 0)
    {
        EXTI_Enable(1);
        key_cnt = 0;
        /* stop timer */
        rt_timer_stop(key->poll_timer);
        rt_timer_control(key->poll_timer , RT_TIMER_CTRL_SET_TIME , &tmer);
        return;
    }

    if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) != 0)
    {
        EXTI_Enable(1);
        /* stop timer */
        rt_timer_stop(key->poll_timer);
        key_down = 0;
        key_cnt = 0;

        kbdevent.parent.type = RTGUI_EVENT_KBD;
        kbdevent.parent.sender = RT_NULL;
        kbdevent.type = RTGUI_KEYUP;
        kbdevent.key = RTGUIK_RIGHT;
        kbdevent.mod = RTGUI_KMOD_NONE;
        kbdevent.unicode = 0;

        rt_kprintf("key up\r\n");

        rtgui_server_post_event(&kbdevent.parent, sizeof(struct rtgui_event_kbd));
        rt_timer_control(key->poll_timer , RT_TIMER_CTRL_SET_TIME , &tmer);
    }
    else
    {
        if(key_down == 0)
        {
            key_down = 1;

            rt_kprintf("key down\r\n");
            kbdevent.parent.type = RTGUI_EVENT_KBD;
            kbdevent.parent.sender = RT_NULL;
            kbdevent.type = RTGUI_KEYDOWN;
            kbdevent.key = RTGUIK_RIGHT;
            kbdevent.mod = RTGUI_KMOD_NONE;
            kbdevent.unicode = 0;
            rtgui_server_post_event(&kbdevent.parent, sizeof(struct rtgui_event_kbd));
            
            tmer = RT_TICK_PER_SECOND/5;
            rt_timer_control(key->poll_timer , RT_TIMER_CTRL_SET_TIME , &tmer);
        }
        else
        {
            if(++key_cnt >= 10)
            {
                key_cnt = 9;
                rt_kprintf("key repeat\r\n");
                kbdevent.parent.type = RTGUI_EVENT_KBD;
                kbdevent.parent.sender = RT_NULL;
                kbdevent.type = RTGUI_KEYDOWN;
                kbdevent.key = RTGUIK_RIGHT;
                kbdevent.mod = RTGUI_KMOD_NONE;
                kbdevent.unicode = 0;
                rtgui_server_post_event(&kbdevent.parent, sizeof(struct rtgui_event_kbd));
            }
        }
    }
}

static rt_err_t rt_keyboard_init (rt_device_t dev)
{
    NVIC_Configuration();
    EXTI_Configuration();

    return RT_EOK;
}

static rt_err_t rt_keyboard_control (rt_device_t dev, rt_uint8_t cmd, void *args)
{
    return RT_EOK;
}

void rt_keyboard_hw_init(void)
{
    key = (struct key_device *)rt_malloc (sizeof(struct key_device));
    if (key == RT_NULL) 
        return; /* no memory yet */

    /* init device structure */
    key->parent.type = RT_Device_Class_Unknown;
    key->parent.init = rt_keyboard_init;
    key->parent.control = rt_keyboard_control;
    key->parent.user_data = RT_NULL;

    /* create 1/10 second timer */
    key->poll_timer = rt_timer_create("keypad", Keypad_timeout, RT_NULL,
                                        RT_TICK_PER_SECOND/10, RT_TIMER_FLAG_PERIODIC);

    /* register touch device to RT-Thread */
    rt_device_register(&(key->parent), "keypad", RT_DEVICE_FLAG_RDWR);
}

#ifdef RT_USING_FINSH
#include <finsh.h>

void key_t( rt_uint16_t key)
{
    struct rtgui_event_kbd kdb;

    kdb.parent.type = RTGUI_EVENT_KBD;
    kdb.type = RTGUI_KEYDOWN;
    kdb.key = key;
    kdb.mod = RTGUI_KMOD_NONE;
    kdb.unicode = 0;
    
    rtgui_server_post_event(&kdb.parent, sizeof(struct rtgui_event_kbd));
    rt_thread_delay(2) ;
    kdb.type = RTGUI_KEYUP;
    rtgui_server_post_event(&kdb.parent, sizeof(struct rtgui_event_kbd));
}

FINSH_FUNCTION_EXPORT(key_t, key ascii) ;
#endif

