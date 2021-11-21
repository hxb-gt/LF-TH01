#ifndef __HEATER_H__
#define __HEATER_H__

#define POWER_MAX			0
#define POWER_700W			1
#define POWER_600W			2
#define POWER_500W			3
#define POWER_450W			4
#define POWER_300W			5
#define POWER_250W			6
#define POWER_150W			7


void heater_init();
void heater_set_power(unsigned int walt);


#endif
