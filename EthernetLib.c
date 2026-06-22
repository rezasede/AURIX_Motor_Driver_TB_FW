#include "EthernetLib.h"

#include "IfxApApu.h"
#include "IfxApProt.h"
#include "Geth/Std/IfxGeth.h"
#include "Hsphy/Std/IfxHsphy.h"
#include "pin.h"

void Cpu0_PrepareEthernetForCpu2 (void)
{


    /* 1. Power rails */
    IfxPmsEvr_enableVoltageRail(&MODULE_PMS, IfxPmsEvr_PrimaryMonitorVoltageSource_vddphphy0);
    IfxPmsEvr_enableVoltageRail(&MODULE_PMS, IfxPmsEvr_PrimaryMonitorVoltageSource_vddphy0);
    IfxPmsEvr_enableVoltageRail(&MODULE_PMS, IfxPmsEvr_PrimaryMonitorVoltageSource_vddphphy1);
    IfxPmsEvr_enableVoltageRail(&MODULE_PMS, IfxPmsEvr_PrimaryMonitorVoltageSource_vddphy1);
    IfxPmsEvr_enableVoltageRail(&MODULE_PMS, IfxPmsEvr_PrimaryMonitorVoltageSource_vddphphy2);
    IfxPmsEvr_enableVoltageRail(&MODULE_PMS, IfxPmsEvr_PrimaryMonitorVoltageSource_vddphy2);
    IfxPmsEvr_enableVoltageRail(&MODULE_PMS, IfxPmsEvr_PrimaryMonitorVoltageSource_vddhsif);

    /* 2. HSPHY init */
    HSPHY_Init();

    const Ifx_HSPHY_ETH_Bits hsphyEthCfg = {.EPR = HSPHY_ETH_EPR_RMII, .MDIOEN = TRUE, .MDIO =
    BOARD_GETH0_P0_MDIO.inSelect};
    HSPHY_ETH_Init(0u, hsphyEthCfg);

    /* 3. Enable GETH module */
    IfxGeth_enableModule(&MODULE_GETH0);

    /* 4. Skip VMT SRAM clears for now — version mismatch with iLLD v2.4.0 */

    /* 5. Grant CPU2 access */
    grantCpu2PeripheralAccess();
}

void grantCpu2PeripheralAccess (void)
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
    IfxPort_configureAccessToPort(&MODULE_P16, (IfxPort_PadAccessGroup) 0, &periphApu);
    IfxPort_configureAccessToPort(&MODULE_P21, (IfxPort_PadAccessGroup) 0, &periphApu);

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
    memCfg.rgnlaVal = 0u;
    memCfg.rgnuaVal = 0u;

    IfxApApu_configureAccessToLmus(&memCfg);
    IfxApApu_configureAccessToDlmus(&memCfg);
}

