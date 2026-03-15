/*
 * UserFunctions.c
 *
 *  Created on: 14 Mar 2026
 *      Author: rezas
 */

#include "UserFunctions.h"

void Delay(Ifx_TickTime time) // Time in ms
{
    waitTime(IfxStm_getTicksFromMilliseconds(time)); /* Delay of 500ms*/
}

