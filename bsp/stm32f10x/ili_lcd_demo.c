#include "ili_lcd_demo.h"

// Compatible list:
// ili9320 ili9325 ili9328
// LG4531

//������������,�����������
#ifdef __CC_ARM                			 /* ARM Compiler 	*/
#define lcd_inline   				static __inline
#elif defined (__ICCARM__)        		/* for IAR Compiler */
#define lcd_inline 					inline
#elif defined (__GNUC__)        		/* GNU GCC Compiler */
#define lcd_inline 					static __inline
#else
#define lcd_inline                 static
#endif

#define rw_data_prepare()               write_cmd(34)


/********* control ***********/
#include "stm32f10x.h"
#include "board.h"

//����ض���.���������ض���ʱ.
#define printf               rt_kprintf //ʹ��rt_kprintf�����
//#define printf(...)                       //�����

/* LCD is connected to the FSMC_Bank1_NOR/SRAM2 and NE2 is used as ship select signal */
/* RS <==> A2 */
#define LCD_REG              (*((volatile unsigned short *) 0x60000000)) /* RS = 0 */
#define LCD_RAM              (*((volatile unsigned short *) 0x60020000)) /* RS = 1 */

/*
Һ�����ܽŷֲ� - �޴���������
 1 - VCC      2 - VCC      3 - GND      4 - GND      5 - DB0      6 - DB1      7 - DB2      8 - DB3
 9 - DB4     10 - DB5     11 - DB6     12 - DB7     13 - DB8     14 - DB9     15 - DB10    16 - DB11
17 - DB12    18 - DB13    19 - DB14    20 - DB15    21 - nCS     22 - RS      23 - nWR     24 - nRD
25 - nRST    26 - BL      27 - TS_nCS  28 - TS_CLK  29 - TS_DOUT 30 - TS_DIN  31 - GND     32 - TS_nPENIRQ
*/


static void LCD_FSMCConfig(void)
{
    FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
    FSMC_NORSRAMTimingInitTypeDef  Timing_read,Timing_write;

    /* FSMC GPIO configure */
    {
        GPIO_InitTypeDef GPIO_InitStructure;
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOF
                               | RCC_APB2Periph_GPIOG, ENABLE);
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);

        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;				 

        /* BL */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;		 //LCD �������
        GPIO_Init(GPIOC, &GPIO_InitStructure);

        /* nRST*/
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 ; 	 //LCD-RST
        GPIO_Init(GPIOE, &GPIO_InitStructure);  	

        /* Set DB0~DB15 as alternate function push pull
           PD14(D0), PD15(D1), PD0(D2), PD1(D3), PE7(D4), PE8(D5), PE9(D6), PE10(D7), PE11(D8), PE12(D9), 
           PE13(D10), PE14(D11), PE15(D12), PD8(D13), PD9(D14), PD10(D15)
        */

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | 
                                        GPIO_Pin_14 | GPIO_Pin_15;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_Init(GPIOD, &GPIO_InitStructure);

        /* Set PE.07(D4), PE.08(D5), PE.09(D6), PE.10(D7), PE.11(D8), PE.12(D9), PE.13(D10),
         PE.14(D11), PE.15(D12) as alternate function push pull */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | 
                                    GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | 
                                    GPIO_Pin_15;
        GPIO_Init(GPIOE, &GPIO_InitStructure); 

        /* NE1 configuration (nCS) */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; 
        GPIO_Init(GPIOD, &GPIO_InitStructure);

        /* RS */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 ; 
        GPIO_Init(GPIOD, &GPIO_InitStructure); 

        /* nWR */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 ; 
        GPIO_Init(GPIOD, &GPIO_InitStructure); 

        /* nRD */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 ; 
        GPIO_Init(GPIOD, &GPIO_InitStructure); 
#if 0
        GPIO_SetBits(GPIOD, GPIO_Pin_7);			//CS=1 
        GPIO_SetBits(GPIOD, GPIO_Pin_14| GPIO_Pin_15 |GPIO_Pin_0 | GPIO_Pin_1);  	 
        GPIO_SetBits(GPIOE, GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10);   
        GPIO_ResetBits(GPIOE, GPIO_Pin_0);
        GPIO_ResetBits(GPIOE, GPIO_Pin_1);			//RESET=0
        GPIO_SetBits(GPIOD, GPIO_Pin_4);		    //RD=1
        GPIO_SetBits(GPIOD, GPIO_Pin_5);			//WR=1

        GPIO_SetBits(GPIOD, GPIO_Pin_13);			//LIGHT
#endif
    }
    /* FSMC GPIO configure */

    /*-- FSMC Configuration -------------------------------------------------*/
    FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &Timing_read;
    FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &Timing_write;
    FSMC_NORSRAMStructInit(&FSMC_NORSRAMInitStructure);

    Timing_read.FSMC_AddressSetupTime = 3;             /* ��ַ����ʱ��  */
    Timing_read.FSMC_DataSetupTime = 4;                /* ���ݽ���ʱ��  */
    Timing_read.FSMC_AccessMode = FSMC_AccessMode_B;    /* FSMC ����ģʽ */

    Timing_write.FSMC_AddressSetupTime = 2;             /* ��ַ����ʱ��  */
    Timing_write.FSMC_DataSetupTime = 3;                /* ���ݽ���ʱ��  */
    Timing_write.FSMC_AccessMode = FSMC_AccessMode_B;   /* FSMC ����ģʽ */

    /* Color LCD configuration ------------------------------------
       LCD configured as follow:
          - Data/Address MUX = Disable
          - Memory Type = SRAM
          - Data Width = 16bit
          - Write Operation = Enable
          - Extended Mode = Enable
          - Asynchronous Wait = Disable */
    FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1;
    FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
    FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
    FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
    FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
    FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
    FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable;
    FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;

    FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);
}

static void delay(int cnt)
{
    volatile unsigned int dl;
    while(cnt--)
    {
        for(dl=0; dl<500; dl++);
    }
}

static void lcd_port_init(void)
{
    LCD_FSMCConfig();
}

lcd_inline void write_cmd(unsigned short cmd)
{
    LCD_REG = cmd;
}

lcd_inline unsigned short read_data(void)
{
    return LCD_RAM;
}

lcd_inline void write_data(unsigned short data_code )
{
    LCD_RAM = data_code;
}

lcd_inline void write_reg(unsigned char reg_addr,unsigned short reg_val)
{
    LCD_REG = reg_addr;
    LCD_RAM = reg_val;
}

lcd_inline unsigned short read_reg(unsigned char reg_addr)
{
    unsigned short val=0;
    write_cmd(reg_addr);
    val = read_data();
    return (val);
}

lcd_inline void SetBackLight(BitAction enable)
{
    GPIO_WriteBit(GPIOC, GPIO_Pin_7, enable);            //LIGHT
}

/********* control <ֻ��ֲ���Ϻ�������> ***********/

static unsigned short deviceid=0;//����һ����̬������������LCD��ID

//����LCD��ID
unsigned int lcd_getdeviceid(void)
{
    return deviceid;
}

static unsigned short BGR2RGB(unsigned short c)
{
    u16  r, g, b, rgb;

    b = (c>>0)  & 0x1f;
    g = (c>>5)  & 0x3f;
    r = (c>>11) & 0x1f;

    rgb =  (b<<11) + (g<<5) + (r<<0);

    return( rgb );
}

static void lcd_SetCursor(unsigned int x,unsigned int y)
{
    write_reg(32,x);    /* 0-239 */
    write_reg(33,y);    /* 0-319 */
}

/* ��ȡָ����ַ��GRAM */
static unsigned short lcd_read_gram(unsigned int x,unsigned int y)
{
    unsigned short temp;
    lcd_SetCursor(x,y);
    rw_data_prepare();
    /* dummy read */
    temp = read_data();
    temp = read_data();
    return temp;
}

static void lcd_clear(unsigned short Color)
{
    unsigned int index=0;
    lcd_SetCursor(0,0);
    rw_data_prepare();                      /* Prepare to write GRAM */
    for (index=0; index<(LCD_WIDTH*LCD_HEIGHT); index++)
    {
        write_data(Color);
    }
}

static void lcd_data_bus_test(void)
{
    unsigned short temp1;
    unsigned short temp2;
    /* [5:4]-ID~ID0 [3]-AM-1��ֱ-0ˮƽ */
    write_reg(0x0003,(1<<12)|(1<<5)|(1<<4) | (0<<3) );

    /* wirte */
    lcd_SetCursor(0,0);
    rw_data_prepare();
    write_data(0x5555);
    write_data(0xAAAA);

    /* read */
    lcd_SetCursor(0,0);
    if (
        (deviceid ==0x9325)
        || (deviceid ==0x9328)
        || (deviceid ==0x9320)
    )
    {
        temp1 = lcd_read_gram(0, 0);//BGR2RGB( lcd_read_gram(0,0) );
        temp2 = lcd_read_gram(1,0);//BGR2RGB( lcd_read_gram(1,0) );
    }
    else if( deviceid ==0x4531 )
    {
        temp1 = lcd_read_gram(0,0);
        temp2 = lcd_read_gram(1,0);
    }

    if( (temp1 == 0x5555) && (temp2 == 0xAAAA) )
    {
        printf(" data bus test pass!\r\n");
    }
    else
    {
        printf(" data bus test error: %04X %04X\r\n",temp1,temp2);
    }
}

void lcd_Initializtion(void)
{
    int color1;
    
    lcd_port_init();
    deviceid = read_reg(0x00);

    /* deviceid check */
    if(
        (deviceid != 0x4531)
        && (deviceid != 0x7783)
        && (deviceid != 0x9320)
        && (deviceid != 0x9325)
        && (deviceid != 0x9328)
        && (deviceid != 0x9300)
    )
    {
        printf("Invalid LCD ID:%08X\r\n",deviceid);
        printf("Please check you hardware and configure.");
        //while(1);
        return ;
    }
    else
    {
        printf("\r\nLCD Device ID : %04X ",deviceid);
    }

    if(deviceid==0x9325|| deviceid==0x9328)
    {

        #if 1
        write_reg(0x00e7,0x0010);
        write_reg(0x0000,0x0001);  			        //start internal osc
#if defined(_ILI_REVERSE_DIRECTION_)
        write_reg(0x0001,0x0000);                    //Reverse Display
#else
        write_reg(0x0001,0x0100);                    //
#endif
        write_reg(0x0002,0x0700); 				    //power on sequence
        /* [5:4]-ID1~ID0 [3]-AM-1��ֱ-0ˮƽ */
        write_reg(0x0003,(1<<12)|(1<<5)|(0<<4) | (1<<3) );
        write_reg(0x0004,0x0000);
        write_reg(0x0008,0x0207);
        write_reg(0x0009,0x0000);
        write_reg(0x000a,0x0000); 				//display setting
        write_reg(0x000c,0x0001);				//display setting
        write_reg(0x000d,0x0000); 				//0f3c
        write_reg(0x000f,0x0000);
        //Power On sequence //
        write_reg(0x0010,0x0000);
        write_reg(0x0011,0x0007);
        write_reg(0x0012,0x0000);
        write_reg(0x0013,0x0000);
        rt_thread_delay(1);
        write_reg(0x0010,0x1590);
        write_reg(0x0011,0x0227);
        rt_thread_delay(1);
        write_reg(0x0012,0x009c);
        rt_thread_delay(1);
        write_reg(0x0013,0x1900);
        write_reg(0x0029,0x0023);
        write_reg(0x002b,0x000e);
        rt_thread_delay(1);
        write_reg(0x0020,0x0000);
        write_reg(0x0021,0x0000);
        rt_thread_delay(1);
        write_reg(0x0030,0x0007);
        write_reg(0x0031,0x0707);
        write_reg(0x0032,0x0006);
        write_reg(0x0035,0x0704);
        write_reg(0x0036,0x1f04);
        write_reg(0x0037,0x0004);
        write_reg(0x0038,0x0000);
        write_reg(0x0039,0x0706);
        write_reg(0x003c,0x0701);
        write_reg(0x003d,0x000f);
        rt_thread_delay(1);
        write_reg(0x0050,0x0000);
        write_reg(0x0051,0x00ef);
        write_reg(0x0052,0x0000);
        write_reg(0x0053,0x013f);
#if defined(_ILI_REVERSE_DIRECTION_)
        write_reg(0x0060,0x2700);
#else
        write_reg(0x0060,0xA700);
#endif
        write_reg(0x0061,0x0001);
        write_reg(0x006a,0x0000);
        write_reg(0x0080,0x0000);
        write_reg(0x0081,0x0000);
        write_reg(0x0082,0x0000);
        write_reg(0x0083,0x0000);
        write_reg(0x0084,0x0000);
        write_reg(0x0085,0x0000);
        write_reg(0x0090,0x0010);
        write_reg(0x0092,0x0000);
        write_reg(0x0093,0x0003);
        write_reg(0x0095,0x0110);
        write_reg(0x0097,0x0000);
        write_reg(0x0098,0x0000);
        //display on sequence
        write_reg(0x0007,0x0133);
        write_reg(0x0020,0x0000);
        write_reg(0x0021,0x0000);
        #else
        write_reg(0x00E3, 0x3008); // Set internal timing
        write_reg(0x00E7, 0x0012); // Set internal timing
        write_reg(0x00EF, 0x1231); // Set internal timing
        write_reg(0x0000, 0x0100); // Start Oscillation
        write_reg(0x0001, 0x0100); // set SS and SM bit
        write_reg(0x0002, 0x0700); // set 1 line inversion

        write_reg(0x0003, 0x10b0); // set GRAM write direction and BGR=0,262K colors,1 transfers/pixel.
        write_reg(0x0004, 0x0000); // Resize register
        write_reg(0x0008, 0x0202); // set the back porch and front porch
        write_reg(0x0009, 0x0000); // set non-display area refresh cycle ISC[3:0]
        write_reg(0x000A, 0x0001); // FMARK function
        write_reg(0x000C, 0x0000); // RGB interface setting
        write_reg(0x000D, 0x0000); // Frame marker Position
        write_reg(0x000F, 0x0000); // RGB interface polarity
        //Power On sequence 
        write_reg(0x0010, 0x1590); // SAP, BT[3:0], AP, DSTB, SLP, STB
        write_reg(0x0011, 0x0007); // DC1[2:0], DC0[2:0], VC[2:0]
        write_reg(0x0012, 0x0000); // VREG1OUT voltage
        write_reg(0x0013, 0x0000); // VDV[4:0] for VCOM amplitude
        rt_thread_delay(5); // Dis-charge capacitor power voltage
        write_reg(0x0010, 0x1690); // SAP, BT[3:0], AP, DSTB, SLP, STB
        write_reg(0x0011, 0x0227); // R11h=0x0221 at VCI=3.3V, DC1[2:0], DC0[2:0], VC[2:0]
        rt_thread_delay(5); // Delay 50ms
        write_reg(0x0012, 0x001C); // External reference voltage= Vci;
        rt_thread_delay(5); // Delay 50ms
        write_reg(0x0013, 0x1800); // R13=1200 when R12=009D;VDV[4:0] for VCOM amplitude
        write_reg(0x0029, 0x001C); // R29=000C when R12=009D;VCM[5:0] for VCOMH
        write_reg(0x002B, 0x000D); // Frame Rate = 91Hz
        rt_thread_delay(5); // Delay 50ms
        write_reg(0x0020, 0x0000); // GRAM horizontal Address
        write_reg(0x0021, 0x0000); // GRAM Vertical Address
        // ----------- Adjust the Gamma Curve ----------//

        write_reg(0x0030, 0x0203);
        write_reg(0x0031, 0x080F);
        write_reg(0x0032, 0x0401);
        write_reg(0x0033, 0x050B);
        write_reg(0x0034, 0x3330);
        write_reg(0x0035, 0x0B05);
        write_reg(0x0036, 0x0005);
        write_reg(0x0037, 0x0F08);
        write_reg(0x0038, 0x0302);
        write_reg(0x0039, 0x3033);
        //---------------- Set GRAM area ---------------//
        write_reg(0x0050, 0x0000); // Horizontal GRAM Start Address
        write_reg(0x0051, 0x00EF); // Horizontal GRAM End Address
        write_reg(0x0052, 0x0000); // Vertical GRAM Start Address
        write_reg(0x0053, 0x013F); // Vertical GRAM Start Address
        write_reg(0x0060, 0xA700); // Gate Scan Line
        write_reg(0x0061, 0x0001); // NDL,VLE, REV
        write_reg(0x006A, 0x0000); // set scrolling line
        //-------------- Partial Display Control ---------//
        write_reg(0x0080, 0x0000);
        write_reg(0x0081, 0x0000);
        write_reg(0x0082, 0x0000);
        write_reg(0x0083, 0x0000);
        write_reg(0x0084, 0x0000);
        write_reg(0x0085, 0x0000);
        //-------------- Panel Control -------------------//
        write_reg(0x0090, 0x0010);
        write_reg(0x0092, 0x0000);
        write_reg(0x0093, 0x0003);
        write_reg(0x0095, 0x0110);
        write_reg(0x0097, 0x0000);
        write_reg(0x0098, 0x0000);
        write_reg(0x0007, 0x0133); // 262K color and display ON

        write_reg(32, 0);
        write_reg(33, 0x013F);
    	write_cmd(34);
    	for(color1=0; color1<76800; color1++)
    	{
    	  write_data(0xffff);
    	}
        #endif

    }
    else if( deviceid==0x9320|| deviceid==0x9300)
    {
        write_reg(0x00,0x0000);
#if defined(_ILI_REVERSE_DIRECTION_)
        write_reg(0x0001,0x0100);                    //Reverse Display
#else
        write_reg(0x0001,0x0000);                    // Driver Output Contral.
#endif
        write_reg(0x02,0x0700);	//LCD Driver Waveform Contral.
//		write_reg(0x03,0x1030);	//Entry Mode Set.
        write_reg(0x03,0x1018);	//Entry Mode Set.

        write_reg(0x04,0x0000);	//Scalling Contral.
        write_reg(0x08,0x0202);	//Display Contral 2.(0x0207)
        write_reg(0x09,0x0000);	//Display Contral 3.(0x0000)
        write_reg(0x0a,0x0000);	//Frame Cycle Contal.(0x0000)
        write_reg(0x0c,(1<<0));	//Extern Display Interface Contral 1.(0x0000)
        write_reg(0x0d,0x0000);	//Frame Maker Position.
        write_reg(0x0f,0x0000);	//Extern Display Interface Contral 2.

        delay(15);
        write_reg(0x07,0x0101);	//Display Contral.
        delay(15);

        write_reg(0x10,(1<<12)|(0<<8)|(1<<7)|(1<<6)|(0<<4));	//Power Control 1.(0x16b0)
        write_reg(0x11,0x0007);								//Power Control 2.(0x0001)
        write_reg(0x12,(1<<8)|(1<<4)|(0<<0));					//Power Control 3.(0x0138)
        write_reg(0x13,0x0b00);								//Power Control 4.
        write_reg(0x29,0x0000);								//Power Control 7.

        write_reg(0x2b,(1<<14)|(1<<4));

        write_reg(0x50,0);		//Set X Start.
        write_reg(0x51,239);	//Set X End.
        write_reg(0x52,0);		//Set Y Start.
        write_reg(0x53,319);	//Set Y End.

#if defined(_ILI_REVERSE_DIRECTION_)
        write_reg(0x0060,0x2700);  //Driver Output Control.
#else
        write_reg(0x0060,0xA700);
#endif
        write_reg(0x61,0x0001);	//Driver Output Control.
        write_reg(0x6a,0x0000);	//Vertical Srcoll Control.

        write_reg(0x80,0x0000);	//Display Position? Partial Display 1.
        write_reg(0x81,0x0000);	//RAM Address Start? Partial Display 1.
        write_reg(0x82,0x0000);	//RAM Address End-Partial Display 1.
        write_reg(0x83,0x0000);	//Displsy Position? Partial Display 2.
        write_reg(0x84,0x0000);	//RAM Address Start? Partial Display 2.
        write_reg(0x85,0x0000);	//RAM Address End? Partial Display 2.

        write_reg(0x90,(0<<7)|(16<<0));	//Frame Cycle Contral.(0x0013)
        write_reg(0x92,0x0000);	//Panel Interface Contral 2.(0x0000)
        write_reg(0x93,0x0001);	//Panel Interface Contral 3.
        write_reg(0x95,0x0110);	//Frame Cycle Contral.(0x0110)
        write_reg(0x97,(0<<8));	//
        write_reg(0x98,0x0000);	//Frame Cycle Contral.


        write_reg(0x07,0x0173);	//(0x0173)
    }
    else if( deviceid==0x4531 )
    {
        // Setup display
        write_reg(0x00,0x0001);
        write_reg(0x10,0x0628);
        write_reg(0x12,0x0006);
        write_reg(0x13,0x0A32);
        write_reg(0x11,0x0040);
        write_reg(0x15,0x0050);
        write_reg(0x12,0x0016);
        delay(15);
        write_reg(0x10,0x5660);
        delay(15);
        write_reg(0x13,0x2A4E);
#if defined(_ILI_REVERSE_DIRECTION_)
        write_reg(0x01,0x0100);
#else
        write_reg(0x01,0x0000);
#endif
        write_reg(0x02,0x0300);

        write_reg(0x03,0x1030);
//	    write_reg(0x03,0x1038);

        write_reg(0x08,0x0202);
        write_reg(0x0A,0x0000);
        write_reg(0x30,0x0000);
        write_reg(0x31,0x0402);
        write_reg(0x32,0x0106);
        write_reg(0x33,0x0700);
        write_reg(0x34,0x0104);
        write_reg(0x35,0x0301);
        write_reg(0x36,0x0707);
        write_reg(0x37,0x0305);
        write_reg(0x38,0x0208);
        write_reg(0x39,0x0F0B);
        delay(15);
        write_reg(0x41,0x0002);

#if defined(_ILI_REVERSE_DIRECTION_)
        write_reg(0x0060,0x2700);
#else
        write_reg(0x0060,0xA700);
#endif

        write_reg(0x61,0x0001);
        write_reg(0x90,0x0119);
        write_reg(0x92,0x010A);
        write_reg(0x93,0x0004);
        write_reg(0xA0,0x0100);
//	    write_reg(0x07,0x0001);
        delay(15);
//	    write_reg(0x07,0x0021);
        delay(15);
//	    write_reg(0x07,0x0023);
        delay(15);
//	    write_reg(0x07,0x0033);
        delay(15);
        write_reg(0x07,0x0133);
        delay(15);
        write_reg(0xA0,0x0000);
        delay(20);
    }
    else if( deviceid ==0x7783)
    {
        // Start Initial Sequence
        write_reg(0x00FF,0x0001);
        write_reg(0x00F3,0x0008);
        write_reg(0x0001,0x0100);
        write_reg(0x0002,0x0700);
        write_reg(0x0003,0x1030);  //0x1030
        write_reg(0x0008,0x0302);
        write_reg(0x0008,0x0207);
        write_reg(0x0009,0x0000);
        write_reg(0x000A,0x0000);
        write_reg(0x0010,0x0000);  //0x0790
        write_reg(0x0011,0x0005);
        write_reg(0x0012,0x0000);
        write_reg(0x0013,0x0000);
        delay(20);
        write_reg(0x0010,0x12B0);
        delay(20);
        write_reg(0x0011,0x0007);
        delay(20);
        write_reg(0x0012,0x008B);
        delay(20);
        write_reg(0x0013,0x1700);
        delay(20);
        write_reg(0x0029,0x0022);

        //################# void Gamma_Set(void) ####################//
        write_reg(0x0030,0x0000);
        write_reg(0x0031,0x0707);
        write_reg(0x0032,0x0505);
        write_reg(0x0035,0x0107);
        write_reg(0x0036,0x0008);
        write_reg(0x0037,0x0000);
        write_reg(0x0038,0x0202);
        write_reg(0x0039,0x0106);
        write_reg(0x003C,0x0202);
        write_reg(0x003D,0x0408);
        delay(20);
        write_reg(0x0050,0x0000);
        write_reg(0x0051,0x00EF);
        write_reg(0x0052,0x0000);
        write_reg(0x0053,0x013F);
        write_reg(0x0060,0xA700);
        write_reg(0x0061,0x0001);
        write_reg(0x0090,0x0033);
        write_reg(0x002B,0x000B);
        write_reg(0x0007,0x0133);
        delay(20);
    }

    //�������߲���,���ڲ���Ӳ�������Ƿ�����.
    lcd_data_bus_test();

    //����
    lcd_clear( Blue );
    rt_thread_delay(5);
    SetBackLight(Bit_SET);
}

/*  �������ص� ��ɫ,X,Y */
void rt_hw_lcd_set_pixel(const char* pixel, int x, int y)
{
    lcd_SetCursor(x,y);

    rw_data_prepare();
    write_data(*(rt_uint16_t*)pixel);
}

/* ��ȡ���ص���ɫ */
void rt_hw_lcd_get_pixel(char* pixel, int x, int y)
{
    *(rt_uint16_t*)pixel = BGR2RGB( lcd_read_gram(x,y) );
}

/* ��ˮƽ�� */
void rt_hw_lcd_draw_hline(const char* pixel, int x1, int x2, int y)
{
    /* [5:4]-ID~ID0 [3]-AM-1��ֱ-0ˮƽ */
    write_reg(0x0003,(1<<12)|(1<<5)|(1<<4) | (0<<3) );

    lcd_SetCursor(x1, y);
    rw_data_prepare(); /* Prepare to write GRAM */
    while (x1 < x2)
    {
        write_data( *(rt_uint16_t*)pixel );
        x1++;
    }
}

/* ��ֱ�� */
void rt_hw_lcd_draw_vline(const char* pixel, int x, int y1, int y2)
{
    /* [5:4]-ID~ID0 [3]-AM-1��ֱ-0ˮƽ */
    write_reg(0x0003,(1<<12)|(1<<5)|(0<<4) | (1<<3) );

    lcd_SetCursor(x, y1);
    rw_data_prepare(); /* Prepare to write GRAM */
    while (y1 < y2)
    {
        write_data( *(rt_uint16_t*)pixel );
        y1++;
    }
}

/* ?? */
void rt_hw_lcd_draw_blit_line(const char* pixels, int x, int y, rt_size_t size)
{
    rt_uint16_t *ptr;

    ptr = (rt_uint16_t*)pixels;

    /* [5:4]-ID~ID0 [3]-AM-1��ֱ-0ˮƽ */
    write_reg(0x0003,(1<<12)|(1<<5)|(1<<4) | (0<<3) );

    lcd_SetCursor(x, y);
    rw_data_prepare(); /* Prepare to write GRAM */
    while (size)
    {
        write_data(*ptr ++);
        size --;
    }
}

struct rt_device_graphic_ops lcd_ili_ops =
{
    rt_hw_lcd_set_pixel,
    rt_hw_lcd_get_pixel,
    rt_hw_lcd_draw_hline,
    rt_hw_lcd_draw_vline,
    rt_hw_lcd_draw_blit_line
};

struct rt_device _lcd_device;
static rt_err_t lcd_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t lcd_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t lcd_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t lcd_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
    switch (cmd)
    {
    case RTGRAPHIC_CTRL_GET_INFO:
    {
        struct rt_device_graphic_info *info;

        info = (struct rt_device_graphic_info*) args;
        RT_ASSERT(info != RT_NULL);

        info->bits_per_pixel = 16;
        info->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB565P;
        info->framebuffer = RT_NULL;
        info->width = 240;
        info->height = 320;
    }
    break;

    case RTGRAPHIC_CTRL_RECT_UPDATE:
        /* nothong to be done */
        break;

    default:
        break;
    }

    return RT_EOK;
}

void rt_hw_lcd_init(void)
{
    /* LCD RESET */
    /* PF10 : LCD RESET */
    #if 1
    {
        GPIO_InitTypeDef GPIO_InitStructure;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
        GPIO_Init(GPIOE,&GPIO_InitStructure);

        GPIO_ResetBits(GPIOE,GPIO_Pin_1);
        rt_thread_delay(10);
        GPIO_SetBits(GPIOE,GPIO_Pin_1);
        /* wait for lcd reset */
        rt_thread_delay(10);
    }
    #endif
    /* register lcd device */
    _lcd_device.type  = RT_Device_Class_Graphic;
    _lcd_device.init  = lcd_init;
    _lcd_device.open  = lcd_open;
    _lcd_device.close = lcd_close;
    _lcd_device.control = lcd_control;
    _lcd_device.read  = RT_NULL;
    _lcd_device.write = RT_NULL;

    _lcd_device.user_data = &lcd_ili_ops;
    lcd_Initializtion();

    /* register graphic device driver */
    rt_device_register(&_lcd_device, "lcd",
                       RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
}
