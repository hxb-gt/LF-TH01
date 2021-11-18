#ifndef _SYSTEM_CLOCK_C_
#define _SYSTEM_CLOCK_C_
/*********************************************************************************************************************/
#include "ca51f5_config.h"
#include "../../includes/ca51f5sfr.h"
#include "../../includes/ca51f5xsfr.h"
#include "../../includes/gpiodef_f5.h"

#include "../../includes/system.h"
#include "../../Library/includes/system_clock.h"
#include <intrins.h>
/*********************************************************************************************************************/
void Sys_Clk_Set_IRCH(void)
{
 	CKCON |= IHCKE;
 	CKCON = (CKCON&0xFE) | CKSEL_IRCH;		
}
void Sys_Clk_Set_IRCL(void)
{
 	CKCON |= ILCKE;
 	CKCON = (CKCON&0xFE) | CKSEL_IRCL;		
}
/*********************************************************************************************************************/
#endif
