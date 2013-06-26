#define __MAX6675_C__

#include "stm32f10x.h"
#include "user.h"
#include "bsp.h"
#include "max6675.h"

#define SPI_Get_DAT()               (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6))

#define SPI_Set_Clk()               (GPIOA->BSRR = GPIO_Pin_5)//(GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET))
#define SPI_Set_CS1()               (GPIOC->BSRR = GPIO_Pin_4)//(GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_SET))
#define SPI_Set_CS2()               (GPIOC->BSRR = GPIO_Pin_5)//(GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_SET))

#define SPI_Clr_Clk()               (GPIOA->BRR = GPIO_Pin_5)//(GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET))
#define SPI_Clr_CS1()               (GPIOC->BRR = GPIO_Pin_4)//(GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_RESET))
#define SPI_Clr_CS2()               (GPIOC->BRR = GPIO_Pin_5)//(GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_RESET))

void MAX6675_Delay(void)
{
    int i = 20000;

    while(--i);
}

uint16_t GetMAX6675Data(MAX6675_DEVICE_INDEX device)
{
    uint16_t t = 0, i;

    SPI_Set_CS1();
    SPI_Clr_Clk();
    MAX6675_Delay();
    if(device == max6675_1)
    {
        SPI_Clr_CS1();
    }
    else
    {
        SPI_Clr_CS2();
    }
    MAX6675_Delay();
    t = 0;
    for(i = 0; i < 16; i++)
    {
        t <<= 1;
        SPI_Set_Clk();
        MAX6675_Delay(); 

        if(SPI_Get_DAT())
        {
            t++;
        }

        SPI_Clr_Clk();
        MAX6675_Delay();
    }

    if(device == max6675_1)
    {
        SPI_Set_CS1();
    }
    else
    {
        SPI_Set_CS2();
    }

    return t;
}

