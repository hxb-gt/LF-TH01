#include "includes\ca51f5sfr.h"
#include "includes\ca51f5xsfr.h"
#include "app\display.h"


//bit 7:  point bit.
//bit6~bit0 : num
static unsigned char  _number[MAX_DISPLAY_NUM]; 
static unsigned char  _led[(MAX_LED_NUM / 8) + 1];
static unsigned char  _scan_phase = 0;

sbit LED0		= P3^0;
sbit LED1		= P3^0;
sbit LED2		= P3^0;
sbit LED3		= P3^0;
sbit LED4		= P3^0;


/*	
           A
        *******
    F   *     *  B
   	    *     *
        *******  G
    E   *     *	
        *     *  C
        *******
           D

*/
// 阿拉伯数字映射到数码管的管脚编号
// 如上图对应段, 一个段对应两个管脚编号
// 第一个管脚高，第二个管脚低，表示开启此段
static code unsigned char number_segment_to_pin[MAX_DISPLAY_NUM][9][2] = { 
	// a      b      c      d      e      f      g     point  
	{{2,3}, {2,4}, {5,2}, {2,6}, {2,5}, {3,2}, {4,2}, {2,1}}, // 数字1  
	{{2,3}, {2,4}, {5,2}, {2,6}, {2,5}, {3,2}, {4,2}, {2,1}}, // 数字1  
};

//扫描数码管的第N个段，数码管之间可错开
//此数字为number_segment 表中的索引
static code unsigned char number_scan_order[MAX_DISPLAY_NUM][8] = { 
	{0,0,0,0,0,0,0,0}, // 第一位数码管
	{0,1,1,0,0,0,0,0}, // 第二位数码管
};


// 阿拉伯数字映射到数码管对应段的状态
// 1表示开启，0关闭
static code unsigned char number_to_pinidx[12][9] = { 
	// a   b   c   d   e   f   g   point  
	{0,0,0,0,0,0,0,0}, // 数字0	
	{0,1,1,0,0,0,0,0}, // 数字1  
	{1,1,0,1,1,0,1,0}, // 数字2  
	{1,1,1,1,0,0,1,0}, // 数字3  
	{0,1,1,0,0,1,1,0}, // 数字4  
	{1,0,1,0,0,1,1,0}, // 数字5  
	{1,0,1,1,1,1,1,0}, // 数字6  
	{1,1,1,0,0,0,0,0}, // 数字7  
	{1,1,1,1,1,1,1,0}, // 数字8  
	{1,1,1,1,0,1,1,0}, // 数字9   
	{0,0,0,0,0,0,1,0}, // -  单杠
	{1,1,0,1,1,1,1,0}  // 字母P
};

void display_init()
{
	unsigned char i;

	_scan_phase = 0;
		
	for(i = 0; i < MAX_DISPLAY_NUM; i++) {
		_number[i] = 0;
	}
	
	for(i = 0; i < ((MAX_LED_NUM / 8) + 1); i++) {
		_led[i] = 0;
	}
	
}

void display_set_number(unsigned char idx, unsigned char value)
{
	_number[idx] = value;
}

void display_set_led(unsigned char *led, unsigned char len) 
{
	unsigned char i;
	for (i = 0;i < len; i++) {
		_led[i] = led[i];
	}
}

static void display_set_number_pin(unsigned char idx, unsigned char on) 
{
	switch(idx) {
	case 0:
		LED0 = on;
		break;
	case 1:
		LED1 = on;
		break;
	case 2:
		LED2 = on;
		break;
	case 3:
		LED3 = on;
		break;
	case 4:
		LED4 = on;
		break;
	}
}

static void display_set_led_pin(unsigned char idx, unsigned char on) 
{
	//只有管脚设置为高才点亮LED处理
	if (!on) 
		return;
	if (on) {
		_led[idx / 8] |= 1 << (idx % 8);
	} else {
		_led[idx / 8] &= ~(1 << (idx % 8));
	}
}


void display_scan_show(void)
{
	unsigned char iterator, index, pin1, pin0, onoff;

#ifdef DISPLAY_SCAN_PARALLEL
	for(iterator = 0; iterator < MAX_DISPLAY_NUM; iterator++) {
		index = number_scan_order[iterator][_scan_phase];
		
		pin1 = number_segment_to_pin[iterator][index][0];
		pin0 = number_segment_to_pin[iterator][index][1];
		
		onoff = number_to_pinidx[_number[iterator] & 0x7f][index];
		
		display_set_led_pin(pin1, onoff);	
		
		display_set_number_pin(pin1, onoff);
		display_set_number_pin(pin0, 0);
		
	}
	
	_scan_phase++;
	if (_scan_phase == 8)
		_scan_phase = 0;
#else
	iterator = _scan_phase / 8;
	index = number_scan_order[iterator][_scan_phase];
	
	pin1 = number_segment_to_pin[iterator][index][0];
	pin0 = number_segment_to_pin[iterator][index][1];
	
	onoff = number_to_pinidx[_number[iterator] & 0x7f][index];
	
	display_set_led_pin(pin1, onoff);	
	
	display_set_number_pin(pin1, onoff);
	display_set_number_pin(pin0, 0);


	_scan_phase++;
	if (_scan_phase == MAX_DISPLAY_NUM * 8)
		_scan_phase = 0;

#endif
}


