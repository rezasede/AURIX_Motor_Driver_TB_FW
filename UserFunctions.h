#ifndef USERFUNCTIONS_H_
#define USERFUNCTIONS_H_

#include "Bsp.h"

void Delay(Ifx_TickTime time);
void grantCpu2PeripheralAccess(void);

void allowTagRw(IfxApApu_ApuConfig *cfg, IfxApProt_TagId tagId);


#endif
