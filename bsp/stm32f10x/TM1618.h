#ifndef __TM1618_H__
#define __TM1618_H__

#ifdef __TM1618_C__
#define __TM1618_DEC__
#else
#define __TM1618_DEC__ extern
#endif /* __TM1618_C__ */

#define TM1618_COMMAND_DISPLAY_MODE             0x00
    #define MODE_SEG8BIT4                       0
    #define MODE_SEG7BIT5                       1
    #define MODE_SEG6BIT6                       2
    #define MODE_SEG5BIT7                       3

#define TM1618_COMMAND_DISPLAY_DATA             0x40
    #define DATA_WIRTE                          0
    #define DATA_READ                           0x02
    #define DATA_FIXED_ADDR                     0x04
    #define DATA_TEST                           0x08
    
#define TM1618_COMMAND_DISPLAY_ADDR             0xC0
    #define ADDR_MASK                           0x0F
    
#define TM1618_COMMAND_DISPLAY_ON               0x80
    #define DISPLAY_ON                          0x08
    #define DISPLAY_LIGHT_MASK                  0x07

__TM1618_DEC__ unsigned char TM1618_GetKey(void);
__TM1618_DEC__ void TM1618_SendData(unsigned char dat);


#endif /* __TM1618_H__ */

