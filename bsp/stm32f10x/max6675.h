#ifndef __MAX6675_H__
#define __MAX6675_H__

#ifdef __MAX6675_C__
#define __MAX6675_DEC__
#else
#define __MAX6675_DEC__ extern
#endif

typedef enum __MAX6675_DEVICE_INDEX
{
    max6675_1,
    max6675_2
}MAX6675_DEVICE_INDEX;

__MAX6675_DEC__ uint16_t GetMAX6675Data(MAX6675_DEVICE_INDEX device);

#endif
 
