#include "UserFunctions.h"

#include "IfxApApu.h"
#include "IfxApProt.h"
#include "Geth/Std/IfxGeth.h"
#include "Hsphy/Std/IfxHsphy.h"
#include "pin.h"

void allowTagRw (IfxApApu_ApuConfig *cfg, IfxApProt_TagId tagId)
{
    if ((uint32) tagId < 32u)
    {
        cfg->wraTagId |= (1UL << (uint32) tagId);
        cfg->rdaTagId |= (1UL << (uint32) tagId);
    }
    else
    {
        uint32 bit = (uint32) tagId - 32u;
        cfg->wrbTagId |= (1UL << bit);
        cfg->rdbTagId |= (1UL << bit);
    }
}

void Delay (Ifx_TickTime time)
{
    waitTime(IfxStm_getTicksFromMilliseconds(time));
}
