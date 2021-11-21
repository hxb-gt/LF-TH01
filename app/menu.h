#ifndef __MENU_H__
#define __MENU_H__
#include "app/display.h"

#define NULL 	(void*)0
/*
button0 : '+' ; 
button1 : '-' 
button2 : 'ԤԼ/����' 
button3 : '����/ȡ��'
button4 : '+ & - ��ϼ�'
*/
#define MAX_BUTTON_NUM	6
#define MAX_MENU_ITEM	7

#define	LED_COOK		(1<<0)
#define	LED_STEW		(1<<1)
#define	LED_BRAISE		(1<<2)
#define	LED_COOKING		(1<<3)
#define	LED_HOT_POT		(1<<4)
#define	LED_APPOINT		(1<<5)
#define	LED_WARM		(1<<5)
#define LED_ALL_OFF		0
#define LED_ALL_ON		(LED_COOK | LED_STEW | LED_BRAISE | LED_COOKING | LED_HOT_POT)


#define TIM_125MS_CNT_MAX  (8*360)
#define TIM_360S_CNT_MAX   (50000)

enum MENU_KEYS{
	NO_KEY = 0,
	KEY_SUBC,
	KEY_FUNC,
	KEY_ADD_DEC,
	KEY_ADD_S,
	KEY_DEC_S,
	KEY_ADD_L,
	KEY_DEC_L,
	MAX_KEY
};
enum WORK_MODE{
//	MODE_STANDBY = 0,
	MODE_COOK = 0,
	MODE_STEW,
	MODE_BRAISE,
	MODE_COOKING,
	MODE_HOT_POT,
	MODE_MAX
};


struct time_paras {
	unsigned char default_time;
	unsigned char min_time;
	unsigned char max_time;
	unsigned char inc_step_s;
	unsigned char inc_step_l;
	unsigned char dec_step_s;
	unsigned char dec_step_l;
};

struct state_machine {
	struct time_paras *def_times;
	unsigned int times; //����ʱ�䣬��λΪ����
	unsigned int appointment_time; //ԤԼʱ�䣬��λΪ����
	unsigned int  buzzer_time_start; //������������ʼʱ�䣬��λΪ125ms
	unsigned int  power;
	unsigned char mode;
	unsigned char appointment;
	unsigned char  buzzer;
	
	unsigned char  buzzer_time; //����������ʱ�䳤�ȣ���λΪ125ms
	unsigned char _number[MAX_DISPLAY_NUM];
	unsigned char  _led[(MAX_LED_NUM / 8) + 1];
};

enum item_code {
	MENU_POWERUP = 0,
	MENU_STANDBY,
	MENU_FUNCS,
	MENU_APPOINT_SET,
	MENU_APPOINT_RUN,
	MENU_COOK,
	MENU_STAY_WARM,
};

typedef void (*menu_handle)(struct state_machine *machine);
/*
	index: �˵�����
	handle : ��ͬ������Ӧ����
			1) handle[0] ��ÿ��ѭ����ִ�еĺ���
			2) handle[1] ��ÿ�ν��뵱ǰ״ִ̬�еĺ���(ִ��һ��)
			3) handle[2] + �Ű����̰�
			4) handle[3] - �Ű����̰�
			5) handle[4] + �Ű�������
			6) handle[5] - �Ű�������
			7) handle[6] ԤԼ/���°���
			8) handle[7] ����/ȡ������
			9) handle[8] +/-ͬʱ����
			
*/
struct menu_item {
//	unsigned char index;
	menu_handle	  handle[4];	
};

struct menu_map {
	menu_handle  finalize;
	struct menu_item item[MAX_MENU_ITEM];
};

struct roadmap_item {
	unsigned char step_time;
	unsigned char  power;
};
struct power_step{
	unsigned char step_cnt;
	struct roadmap_item item[5]; 
};

extern unsigned int tim_360s; //ÿ360���1
extern unsigned int tim_125ms;//ÿ125ms��1

void menu_init();
void menu_loop(unsigned int ts_key);


#endif
