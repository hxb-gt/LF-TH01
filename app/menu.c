#include "app/display.h"
#include "app/heater.h"
#include "app/menu.h"

unsigned int tim_360s = 0; //每360秒加1
unsigned int tim_125ms = 0;//每125ms加1

static struct state_machine sys_machine;
static struct menu_item *cur_item;


static void power_up(struct state_machine *machine);
static void power_up_enter(struct state_machine *machine);
static void standby(struct state_machine *machine);
static void standby_enter(struct state_machine *machine);
static void standby_key_subc(struct state_machine *machine);
static void standby_key_func(struct state_machine *machine);
static void funcs(struct state_machine *machine);
static void funcs_enter(struct state_machine *machine);
static void funcs_key_subc(struct state_machine *machine);
static void funcs_key_func(struct state_machine *machine);
static void appoint(struct state_machine *machine);
static void appoint_enter(struct state_machine *machine);
static void appoint_key_subc(struct state_machine *machine);
static void appoint_key_func(struct state_machine *machine);
static void appointr(struct state_machine *machine);
static void appointr_enter(struct state_machine *machine);
static void appointr_key_subc(struct state_machine *machine);
static void appointr_key_func(struct state_machine *machine);
static void work(struct state_machine *machine);
static void work_enter(struct state_machine *machine);
static void work_key_subc(struct state_machine *machine);
static void work_key_func(struct state_machine *machine);
static void warm(struct state_machine *machine);
static void warm_enter(struct state_machine *machine);
static void warm_key_subc(struct state_machine *machine);
static void warm_key_func(struct state_machine *machine);
static void finalize(struct state_machine *machine);
static void key_add(struct state_machine *machine, enum MENU_KEYS key);
static void key_dec(struct state_machine *machine, enum MENU_KEYS key);

static struct menu_item *set_next_menu_item(unsigned char item);
static struct menu_item *get_cur_menu_item(void);

//#define menu null
static code struct menu_map menu = { 
	
#if 1
	finalize,
	{
        {power_up,    power_up_enter, NULL,              NULL}, // 上电状态
        {standby,     standby_enter,  standby_key_subc,  standby_key_func}, // 待机状态
        {funcs,       funcs_enter,    funcs_key_subc,    funcs_key_func}, // 功能设置状态
        {appoint,     appoint_enter,  appoint_key_subc,  appoint_key_func},   // 预约设置状态
        {appointr,    appointr_enter, appointr_key_subc, appointr_key_func},   // 预约运行状态
        {work,        work_enter,     NULL,              work_key_func},   		// 煮状态
        {warm,	      warm_enter,     warm_key_subc,     warm_key_func},		// 煮状态
	}

#else
		0
#endif
};

/*
不同模式时间参数，单位为6分钟
*/
static code struct time_paras default_appoint_time = {
	60, 5, 95, 5, 5, 5, 5 //预约模式
};
static code struct time_paras default_time[] ={
	{60, 5, 95, 5, 5, 5, 5},  //待机模式
	{10, 1, 15, 1, 1, 1, 1},  //煮模式
	{10, 1, 25, 1, 1, 1, 1},  //炖模式
	{10, 1, 20, 1, 1, 1, 1},  //蒸模式
	{3,  1, 3,  1, 1, 1, 1},  //烹饪模式
	{5,  1, 5,  1, 1, 1, 1},  //火锅模式
//	{9.9*60, 0.5*60, 9.5*60, 0.1*60, 0.1*60, 0.1*60, 0.1*60},  //保温模式
};

static code struct power_step power_roadmap[] = {
//	{},
	{ 2, { {10, POWER_MAX }, {10, POWER_500W} } },
	{ 2, { {10, POWER_MAX }, {10, POWER_500W} } },
	{ 2, { {10, POWER_MAX }, {10, POWER_500W} } },
	{ 3, { { 1, POWER_250W}, { 2, POWER_450W}, { 3, POWER_700W} } },
	{ 5, { { 1, POWER_250W}, { 2, POWER_300W}, { 3, POWER_450W}, { 4, POWER_600W}, { 5, POWER_700W} } },
};
static code unsigned char buzzer_snd_time[] = {
  //NO_KEY,  KEY_ADD_S,   KEY_DEC_S,   KEY_ADD_L,    KEY_DEC_L,   KEY_SUBC,   KEY_FUNC,   KEY_ADD_DEC
	8,        4,             4,            4,             4,           4,           4,           4
};

//工作模式对应的led灯
static code unsigned char mode_to_led[] = {
	LED_COOK, LED_STEW, LED_BRAISE, LED_COOKING, LED_HOT_POT, 
};

static code unsigned char test[1] = {
1
};

static void reset_timer()//7,639 8192	
{              
	tim_360s = 0;
	tim_125ms = 0;
	tim_360s = test[0];
}

/* 设置工作模式  */
static void set_mode(struct state_machine *machine, enum WORK_MODE mode)
{
	machine->mode = mode;
	machine->def_times = &default_time[mode];
	machine->times = machine->def_times->default_time;
}

/* 在当前模式基础上设置下一个工作模式  */
static void set_next_mode(struct state_machine *machine)
{
	machine->mode++;
	if (machine->mode == MODE_MAX) 
		machine->mode = MODE_COOK;
	machine->def_times = &default_time[machine->mode];
	machine->times = machine->def_times->default_time;
}
/*  开启蜂鸣器  */
static void buzzer_on(struct state_machine *machine, enum MENU_KEYS keys)
{
	machine->buzzer = 1;
	machine->buzzer_time_start = tim_125ms;
	machine->buzzer_time = buzzer_snd_time[keys];
}
static void buzzer_handle(struct state_machine *machine)
{
	unsigned char time;
	if (machine->buzzer == 1) {
		//开启蜂鸣器
		if (machine->buzzer_time_start > tim_125ms) { //计数器发生反转
			time = TIM_125MS_CNT_MAX - machine->buzzer_time_start + tim_125ms;
		} else {
			time = tim_125ms - machine->buzzer_time_start;
		}
		// 响500ms
		if (time > machine->buzzer_time) {
			machine->buzzer = 0;
			machine->buzzer_time = 0;
			machine->buzzer_time_start = 0;
		}
	}
}

/*************************************************************************************************** 
<-上电状态->
1) 数码管显示88 ; 
2) 所有led开启
3) 关闭加热器; 
4) 蜂鸣器开启
5) 1s 后进入待机状态
*/
static void power_up(struct state_machine *machine)
{
	//蜂鸣器
	buzzer_handle(machine);
	
	// 1s 后进入待机状态
	if (tim_360s == 0 && tim_125ms > 8) {
		set_next_menu_item(MENU_STANDBY);
	}
}
static void power_up_enter(struct state_machine *machine)
{
	machine->_number[0] = NUM8; 
	machine->_number[1] = NUM8;
	machine->_led[0] = LED_ALL_ON; 
	machine->power = 0;			
	buzzer_on(machine, NO_KEY);
	reset_timer();
}

/*************************************************************************************************** 
进入待机初始化函数 
1) 数码管显示--, 并以0.5s闪烁
2) 关闭led开启
3) 关闭加热器
4) 关闭蜂鸣器
5) 按<保温/预约>按键进入保温设置状态
6) 按<功能/取消>按键进入功能设置状态
*/
static void standby(struct state_machine *machine)
{
		//蜂鸣器
	buzzer_handle(machine);
	
	if ((tim_125ms % 8) < 4) {
		machine->_number[0] = NUM_; 
		machine->_number[1] = NUM_;
	} else {
		machine->_number[0] = NUM0; 
		machine->_number[1] = NUM0;
	}
}
static void standby_enter(struct state_machine *machine)
{
	machine->_led[0] = LED_ALL_OFF; 
	machine->appointment = 0;	
	machine->power = 0;			
	reset_timer();

}
static void standby_key_subc(struct state_machine *machine )
{	
	buzzer_on(machine, KEY_SUBC);
	/*  进入保温菜单  */
	set_next_menu_item(MENU_STAY_WARM);
}
static void standby_key_func(struct state_machine *machine )
{	
	buzzer_on(machine, KEY_FUNC);
	/*  进入功能设置菜单,默认设置为煮工作状态  */
	set_next_menu_item(MENU_FUNCS);
	set_mode(machine, MODE_COOK);
}

/*************************************************************************************************** 
进入功能设置函数 
1) 数码管显示工作时间
2)  led以0.5s频率闪烁
3) 关闭加热器
4) 关闭蜂鸣器
5) 按<保温/预约>按键进入预约设置菜单
6) 按<功能/取消>按键设置为下一个加热模式
*/
static void funcs(struct state_machine *machine)
{
		//蜂鸣器
	buzzer_handle(machine);
	
	//指示灯以0.5秒闪烁
	if ((tim_125ms % 8) < 4) {
		machine->_led[0] = mode_to_led[machine->mode]; 
	} else {
		machine->_led[0] = LED_ALL_OFF;
	}
	
	//数码管显示
	machine->_number[0] = (machine->times / 60) | DISPLAY_POINT;
	machine->_number[1] = (machine->times % 60) / 6;
	
	//如果5秒后没有按键设置，进入对应工作模式
	if (tim_360s == 0 && tim_125ms > 8*5) {
		/*  进入设置的工作模式  */
		set_next_menu_item(MENU_COOK);
	}
}
static void funcs_enter(struct state_machine *machine)
{
	machine->appointment = 0;			
	machine->power = 0;			
	reset_timer();
}
static void funcs_key_subc(struct state_machine *machine )
{	
	buzzer_on(machine, KEY_SUBC);
	//烹饪和火锅不支持预约
	if (machine->mode == MODE_COOKING || machine->mode == MODE_COOKING)
		return;
	/*  进入预约设置模式  */
	set_next_menu_item(MENU_APPOINT_SET);
}

static void funcs_key_func(struct state_machine *machine )
{
	buzzer_on(machine, KEY_FUNC);
	set_next_mode(machine);
}


/*************************************************************************************************** 
进入预约设置状态
1) 数码管显示预约时间
2)  led以0.5s频率闪烁
3) 关闭加热器
4) 关闭蜂鸣器
5) 按<保温/预约>按键不动作
6) 按<功能/取消>按键设置为功能设置菜单
*/
static void appoint(struct state_machine *machine)
{
		//蜂鸣器
	buzzer_handle(machine);
	
	//指示灯以0.5秒闪烁
	if ((tim_125ms % 8) < 4) {
		machine->_led[0] = mode_to_led[machine->mode]; 
	} else {
		machine->_led[0] = LED_ALL_OFF;
	}
	
	//数码管显示
	machine->_number[0] = (machine->appointment_time / 60) | DISPLAY_POINT;
	machine->_number[1] = (machine->appointment_time % 60) / 6;
	
	//如果5秒后没有按键设置，进入对应工作模式
	if (tim_360s == 0 && tim_125ms > 8*5) {
		/*  进入预约设置模式  */
		set_next_menu_item(MENU_APPOINT_RUN);
	}
}
static void appoint_enter(struct state_machine *machine)
{
	machine->power = 0;	
	machine->appointment = 1;	
	machine->appointment_time = default_appoint_time.default_time;
//	machine->buzzer = 0;
	reset_timer();
}
static void appoint_key_subc(struct state_machine *machine )
{
	machine->mode = machine->mode;
//	buzzer_on(machine, KEY_SUBC);
	/*  进入预约设置模式  */
//	set_next_menu_item(MENU_APPOINT_SET);
}

static void appoint_key_func(struct state_machine *machine )
{
	buzzer_on(machine, KEY_FUNC);
	/*  退出预约设置模式进入功能设置状态 */
	set_next_menu_item(MENU_FUNCS);
}
/*************************************************************************************************** 
进入预约工作状态 
1) 数码管显示剩余预约时间
2)  预约指示灯常亮
3) 关闭加热器
4) 关闭蜂鸣器
5) 按<保温/预约>按键进入保温工作模式
6) 按<功能/取消>按键进入待机菜单
*/
static void appointr(struct state_machine *machine)
{
	unsigned int remain_time;

	//蜂鸣器
	buzzer_handle(machine);

	//预约指示灯亮
	machine->_led[0] = LED_APPOINT;
	
	//数码管显示预约剩余时间
	remain_time = ((machine->appointment_time / 6) - tim_360s);
	machine->_number[0] = (remain_time / 10) | DISPLAY_POINT;
	machine->_number[1] = (remain_time % 10);
	
	//如果预约时间到，进入对应工作模式
	if (remain_time == 0) {
		/*  进入预约设置模式  */
		set_next_menu_item(MENU_COOK);
	}
}
static void appointr_enter(struct state_machine *machine)
{
	machine->power = 0;	
	machine->appointment = 1;	
	reset_timer();
}

static void appointr_key_subc(struct state_machine *machine )
{	
	buzzer_on(machine, KEY_SUBC);
	/*  进入保温模式  */
	set_next_menu_item(MENU_STAY_WARM);
}

static void appointr_key_func(struct state_machine *machine )
{
	buzzer_on(machine, KEY_FUNC);
	/*  退出预约设置模式进入功能设置状态 */
	set_next_menu_item(MENU_STANDBY);
}

/*************************************************************************************************** 
进入保温工作状态 
1) 数码管显示预约时间
2)  led以0.5s频率闪烁
3) 关闭加热器
4) 关闭蜂鸣器
5) 按<保温/预约>按键不动作
6) 按<功能/取消>按键进入待机菜单
*/
static void work(struct state_machine *machine)
{
	unsigned char i;
	unsigned int period,tmp;
	unsigned int remain_time;
	struct power_step *map = &power_roadmap[machine->mode];

	//蜂鸣器
	buzzer_handle(machine);

	if (machine->mode == MODE_COOKING || machine->mode == MODE_HOT_POT) {
		machine->power = map->item[machine->times].power;
		//数码管固定显示BB
		machine->_number[0] = NUMP;
		machine->_number[1] = machine->times;
	} else {
		period = 0;
		for (i = 0; i < map->step_cnt; i++) { /*  统计所有步骤所用时间  */
			period += map->item[i].step_time;
		}
		/*  剩余多少工作时间  */
		remain_time = machine->times - tim_360s*6;
		machine->_number[0] = (remain_time / 60) | DISPLAY_POINT;
		machine->_number[1] = (remain_time % 60) / 6;
	
		tmp = machine->times % period;
		period = 0;
		for (i = 0; i < map->step_cnt; i++) { /*  统计所有步骤所用时间  */
			period += map->item[i].step_time;
			if (tmp < period)
				break;
		}
		machine->power = map->item[i].power;
	}
}
static void work_enter(struct state_machine *machine)
{
	machine->_led[0] = mode_to_led[machine->mode]; 
	machine->power = 0;	
	machine->appointment = 0;			
	reset_timer();
}

static void work_key_subc(struct state_machine *machine )
{	
		machine->mode = machine->mode;

	/*  进入保温模式  */
//	set_next_menu_item(MENU_STAY_WARM);
}

static void work_key_func(struct state_machine *machine )
{
	buzzer_on(machine, KEY_FUNC);
	/*  进入待机模式*/
	set_next_menu_item(MENU_STANDBY);
}



/*************************************************************************************************** 
进入保温工作状态 
1) 数码管显示预约时间
2)  led以0.5s频率闪烁
3) 关闭加热器
4) 关闭蜂鸣器
5) 按<保温/预约>按键不动作
6) 按<功能/取消>按键进入待机菜单
*/
static void warm(struct state_machine *machine)
{
	//蜂鸣器
	buzzer_handle(machine);
	
	//预约指示灯亮
	machine->_led[0] = LED_WARM;
	
	//数码管固定显示BB
	machine->_number[0] = NUM8;
	machine->_number[1] = NUM8;
}
static void warm_enter(struct state_machine *machine)
{
	machine->power = 60;	
	machine->appointment = 0;			
	reset_timer();
}

static void warm_key_subc(struct state_machine *machine )
{	
		machine->mode = machine->mode;
//	buzzer_on(machine, KEY_FUNC);
	/*  进入保温模式  */
//	set_next_menu_item(MENU_STAY_WARM);
}

static void warm_key_func(struct state_machine *machine )
{

	buzzer_on(machine, KEY_FUNC);
	/*  退出预约设置模式进入功能设置状态 */
	set_next_menu_item(MENU_STANDBY);
}


static void finalize(struct state_machine *machine )
{
	//显示数码管
	display_set_number(0, machine->_number[0]);
	display_set_number(1, machine->_number[1]);
	//显示led
	display_set_led(machine->_led, ((MAX_LED_NUM / 8) + 1));
	//设置功率
	heater_set_power(machine->power);
}


static void key_add(struct state_machine *machine, enum MENU_KEYS key)
{
	buzzer_on(machine, KEY_ADD_S);
	if (machine->appointment) {
		if (key == KEY_ADD_S)
			machine->appointment_time += default_appoint_time.inc_step_s;
		else
			machine->appointment_time += default_appoint_time.inc_step_l;
		
		if (machine->appointment_time > default_appoint_time.max_time) {
			machine->appointment_time = default_appoint_time.max_time;
		}
		return;
	}
	if (key == KEY_ADD_S)
		machine->times += machine->def_times->inc_step_s;
	else
		machine->times += machine->def_times->inc_step_l;
	
	if (machine->times > machine->def_times->max_time) {
		machine->times = machine->def_times->max_time;
	}
}
static void key_dec(struct state_machine *machine, enum MENU_KEYS key)
{
	buzzer_on(machine, KEY_DEC_S);

	if (machine->appointment) {
		if (key == KEY_ADD_S)
			machine->appointment_time -= default_appoint_time.dec_step_s;
		else
			machine->appointment_time -= default_appoint_time.dec_step_l;
		
		if (machine->appointment_time < default_appoint_time.min_time) {
			machine->appointment_time = default_appoint_time.min_time;
		}
		return;
	}
	if (key == KEY_ADD_S)
		machine->times -= machine->def_times->dec_step_s;
	else
		machine->times -= machine->def_times->dec_step_l;
	
	if (machine->times < machine->def_times->min_time) {
		machine->times = machine->def_times->min_time;
	}
}

enum MENU_KEYS key_translate(unsigned int ts_key)
{
	ts_key = ts_key;
	return KEY_FUNC;
}

void menu_init()
{

	set_next_menu_item(0);
}

static struct menu_item *set_next_menu_item(unsigned char item)
{
	cur_item = &menu.item[item];

	// 执行状态进入函数
	if (cur_item->handle[1] != NULL) {
		cur_item->handle[1](&sys_machine);
	}
	return cur_item;
}
static struct menu_item *get_cur_menu_item(void)
{
	return cur_item;
}

void menu_loop(unsigned int ts_key)
{
	enum MENU_KEYS key = key_translate(ts_key);
	struct menu_item *item = get_cur_menu_item();

	
	//按键处理
	switch(key) {
	case KEY_SUBC:
		item->handle[2](&sys_machine);
		break;
	case KEY_FUNC:
		item->handle[3](&sys_machine);
		break;
	case KEY_ADD_DEC:
		break;
		//item->handle[0](&sys_machine);
	case KEY_ADD_S:
	case KEY_ADD_L:
		key_add(&sys_machine, key);
		break;
	case KEY_DEC_S:
	case KEY_DEC_L:
		key_dec(&sys_machine, key);
		break; 
	}
	
	// 常态函数执行
	if (item->handle[0]) {
		item->handle[0](&sys_machine);
	}
	//最终公用处理函数
	menu.finalize(&sys_machine);
}
