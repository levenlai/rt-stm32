/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
#define __TM1618_C__

#include "stm32f10x.h"
#include "TM1618.h"

#define SET_STB()               (GPIOE->BSRR = GPIO_Pin_4)
#define CLR_STB()               (GPIOE->BRR = GPIO_Pin_4)
#define SET_CLK()               (GPIOE->BSRR = GPIO_Pin_3)
#define CLR_CLK()               (GPIOE->BRR = GPIO_Pin_3)

#define GPIOE_OFFSET              (GPIOE_BASE - PERIPH_BASE + 8)            //GPIOE->IDR
#define GPIOE_PINNUMBER           2
#define GPIOE_PIN2_GET            (PERIPH_BB_BASE + (GPIOE_OFFSET * 32) + (GPIOE_PINNUMBER * 4))


void SET_DAT(void)
{
    //GPIOE-->GPIO_Pin_2
    uint32_t dat;

    dat = GPIOE->CRL;
    dat &= ~0x00000F00;
    dat |= 0x3 << (4 * 2);
    GPIOE->CRL = dat;

    GPIOE->BSRR = GPIO_Pin_2;

//    GPIO_WriteBit(GPIOE, GPIO_Pin_2, Bit_SET);
}

void CLR_DAT(void)
{
    uint32_t dat;

    dat = GPIOE->CRL;
    dat &= ~0x00000F00;
    dat |= 0x3 << (4 * 2);
    GPIOE->CRL = dat;

    GPIOE->BRR = GPIO_Pin_2;
}

uint8_t GET_DAT(void)
{
    uint32_t dat;

    dat = GPIOE->CRL;
    dat &= ~0x00000F00;
    dat |= 0x8 << (4 * 2);
    GPIOE->CRL = dat;

    GPIOE->BSRR = GPIO_Pin_2;           //MODE_IPU

    return *(__IO uint32_t *)GPIOE_PIN2_GET;
//    return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_2);
}

void TM1618Delay(void)
{
    int i = 3;

    while(--i);
}

void TM1618_SendData(unsigned char dat)
{
    unsigned char i;

    for( i = 0; i < 8; i++ )
    {
        CLR_CLK();
        TM1618Delay();
        
        if( dat & 0x01 )
        {
            SET_DAT();
        }
        else
        {
            CLR_DAT();
        }
        TM1618Delay();

        SET_CLK();
        TM1618Delay();

        dat >>= 1;
    }
}

unsigned char TM1618_GetData(void)
{
    unsigned char dat = 0;
    unsigned char i;

    for( i = 0; i < 8; i++ )
    {
        CLR_CLK();
        TM1618Delay();
        SET_CLK();
        TM1618Delay();

        dat >>= 1;

        if( GET_DAT() & 0x01 )
        {
            dat |= 0x80;
        }

    }
    
    return dat;
}

void TM1618_SendCommand(unsigned char command)
{
    SET_STB();
    TM1618Delay();
    CLR_STB();

    TM1618_SendData(command);
}

unsigned char TM1618_GetKey(void)
{
    unsigned char dat[3], i;

    TM1618_SendCommand(TM1618_COMMAND_DISPLAY_DATA | DATA_READ);

    SET_DAT();
    TM1618Delay();
    
    for( i = 0; i < 3; i++ )
    {
        dat[i] = TM1618_GetData();
    }

    SET_STB();

    return (((dat[0] & 0x02) >> 1) | ((dat[0] & 0x10) >> 3)
            | ((dat[1] & 0x02) << 1) | ((dat[1] & 0x10) >> 1)
            | ((dat[2] & 0x02) << 3));
}

void TM1618_Display(uint8_t pos, uint8_t *pbuf, uint8_t tsize)
{
    int i;
    
    TM1618_SendCommand(TM1618_COMMAND_DISPLAY_MODE | MODE_SEG8BIT4);
    TM1618_SendCommand(TM1618_COMMAND_DISPLAY_DATA);
    TM1618_SendCommand(TM1618_COMMAND_DISPLAY_ADDR | pos);            //Start from address 0

    for( i = 0; i < tsize; i++)
    {
        TM1618_SendData(pbuf[(tsize - i)*2]);
        TM1618_SendData(pbuf[(tsize - i)*2 + 1]);
    }
    TM1618_SendCommand(TM1618_COMMAND_DISPLAY_ON | DISPLAY_ON);


    SET_STB();
}

#if 0
void TM1618_Process(void)
{
    unsigned char *pBuf;
    int i;
    
    pBuf = GetDisplayBuffer();

    TM1618_SendCommand(TM1618_COMMAND_DISPLAY_MODE | MODE_SEG8BIT4);
    TM1618_SendCommand(TM1618_COMMAND_DISPLAY_DATA);
    TM1618_SendCommand(TM1618_COMMAND_DISPLAY_ADDR | 0);            //Start from address 0

    for( i = 0; i < 4; i++)
    {
        TM1618_SendData(pBuf[(3 - i)*2]);
        TM1618_SendData(pBuf[(3 - i)*2 + 1]);
    }
    TM1618_SendCommand(TM1618_COMMAND_DISPLAY_ON | DISPLAY_ON);


    SET_STB();
}
#endif
