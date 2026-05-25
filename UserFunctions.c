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



void grantCpu2PeripheralAccess(void)
{
    IfxApApu_ApuConfig periphApu;
    IfxApApu_ApuConfig memApu;
    IfxApApu_ApuMemoryConfig memCfg;

    IfxApApu_initConfig(&periphApu);
    periphApu.wraTagId = 0u;
    periphApu.wrbTagId = 0u;
    periphApu.rdaTagId = 0u;
    periphApu.rdbTagId = 0u;

    allowTagRw(&periphApu, IfxApProt_TagId_cpu2d);
    allowTagRw(&periphApu, IfxApProt_TagId_cpu2p);

    IfxHsphy_configureAccessToHsphy(&periphApu);
    IfxGeth_configureAccessToGeth(&periphApu);

    /* RMII data pins: CRSDV, RXD0, RXD1, REFCLK, TXEN, TXD0, TXD1 → P16 */
    IfxPort_configureAccessToPort(&MODULE_P16, (IfxPort_PadAccessGroup)0, &periphApu);
    IfxPort_configureAccessToPort(&MODULE_P21, (IfxPort_PadAccessGroup)0, &periphApu);

    /* LMU / DLMU access for DMA descriptors and buffers */
    IfxApApu_initConfig(&memApu);
    memApu.wraTagId = 0u;
    memApu.wrbTagId = 0u;
    memApu.rdaTagId = 0u;
    memApu.rdbTagId = 0u;

    allowTagRw(&memApu, IfxApProt_TagId_cpu2d);
    allowTagRw(&memApu, IfxApProt_TagId_cpu2p);
    allowTagRw(&memApu, IfxApProt_TagId_gethc0);

    memCfg.apuConfig = &memApu;
    memCfg.rgnlaVal  = 0u;
    memCfg.rgnuaVal  = 0u;

    IfxApApu_configureAccessToLmus(&memCfg);
    IfxApApu_configureAccessToDlmus(&memCfg);
}
