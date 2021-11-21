#ifndef __LED_DRIVER_H__
#define __LED_DRIVER_H__


#define MAX_DISPLAY_NUM			2
#define MAX_LED_NUM				5

#define DISPLAY_SCAN_PARALLEL	1
#define DISPLAY_POINT			0x80

#define NUM0	0
#define NUM1	1
#define NUM2	2
#define NUM3	3
#define NUM4	4
#define NUM5	5
#define NUM6	6
#define NUM7	7
#define NUM8	8
#define NUM9	9
#define NUM_	10
#define NUMP	11

#define NUM_DOT	0x80


void display_init();
void display_set_number(unsigned char idx, unsigned char num);
void display_set_led(unsigned char *led, unsigned char len);
void display_scan_show(void);


#endif

