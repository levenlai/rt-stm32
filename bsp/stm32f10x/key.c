#include "stm32f10x.h"
#include "board.h"

#include <rtthread.h>
#include "key.h"

struct key_device
{
    struct rt_device parent;
    rt_timer_t poll_timer;
    struct rt_semaphore *key_semaphore;
    rt_uint32_t key_value;
    rt_uint32_t key_cnt;
    rt_uint32_t (*GetKeyDat)(void);
};

static struct key_device *key = RT_NULL;

rt_uint32_t GetKeyBoard(void)
{
    /* read GPIO port*/
    return VK_NULL;
}

void Keypad_timeout(void* parameter)
{
    struct key_device *key_d = (struct key_device *)parameter;
    rt_uint32_t dat;

    if(key_d->GetKeyDat == RT_NULL)
    {
        rt_timer_stop(key_d->poll_timer);
        return;
    }
    
    dat = key_d->GetKeyDat() & VK_MASK;
    if(key_d->key_value != dat)
    {
        if(++key_d->key_cnt >= 3)
        {
            /* 150ms arrival*/
            key_d->key_cnt = 0;
            
            if(dat != VK_NULL)
            {
                key_d->key_value = dat;
                key_d->parent.user_data = (void *)(dat | VK_KEY_DOWN);
            }
            else
            {
                key_d->parent.user_data = (void *)((key_d->key_value & VK_MASK) | VK_KEY_UP);
                key_d->key_value = dat;
            }

            rt_sem_release(key_d->key_semaphore);
        }
    }
    else
    {
        if(key_d->key_value & VK_MASK != VK_NULL)
        {
            /* repeat key */
            if(++key_d->key_cnt >= 40)
            {
                /*delay 2s*/
                rt_sem_release(key_d->key_semaphore);
                key_d->key_cnt = 35;
            }
        }
        else
        {
            key_d->parent.user_data = VK_NULL;
            key_d->key_cnt = 0;
            key_d->key_value = VK_NULL;
        }
    }
}

static rt_err_t rt_keyboard_open(rt_device_t dev, rt_uint16_t oflag)
{
    rt_timer_start(((struct key_device *)dev)->poll_timer);
    return RT_EOK;
}

static rt_err_t rt_keyboard_close(rt_device_t dev)
{
    rt_timer_stop(((struct key_device *)dev)->poll_timer);
    return RT_EOK;
}

static rt_err_t rt_keyboard_init (rt_device_t dev)
{
    /*
     * configure the gpio pin as input
     */
    return RT_EOK;
}

static rt_err_t rt_keyboard_control (rt_device_t dev, rt_uint8_t cmd, void *args)
{
    if(!(dev->flag & RT_DEVICE_OFLAG_OPEN))
        return RT_EIO;
    
    switch(cmd)
    {
        case RT_KEYBOARD_READ_VALUE:
            if(rt_sem_take(((struct key_device *)dev)->key_semaphore, RT_WAITING_FOREVER))
            {
                rt_uint32_t dat;
                dat = (rt_uint32_t)((struct key_device *)dev)->parent.user_data;
                if(dat != VK_NULL)
                {
                    *(rt_uint32_t *)args = dat;
                }
                else
                {
                    return RT_EEMPTY;
                }
            }
            break;
    }
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
    key->parent.open = rt_keyboard_open;
    key->parent.close = rt_keyboard_close;
    key->parent.user_data = RT_NULL;

    key->GetKeyDat = GetKeyBoard;

    /* create 1/20 second timer */
    key->poll_timer = rt_timer_create("keypad", Keypad_timeout, &key,
                                        RT_TICK_PER_SECOND/20, RT_TIMER_FLAG_PERIODIC);
    key->key_semaphore = rt_sem_create("keyboard", 0, RT_IPC_FLAG_FIFO);

    /* register touch device to RT-Thread */
    rt_device_register(&(key->parent), "keypad", RT_DEVICE_FLAG_RDWR);
}

#ifdef RT_USING_FINSH
#include <finsh.h>

void key_t( rt_uint16_t vk_key)
{
    key->parent.user_data = (void *)(vk_key | VK_KEY_DOWN);
    rt_sem_release(key->key_semaphore);
}

FINSH_FUNCTION_EXPORT(key_t, key ascii) ;
#endif

