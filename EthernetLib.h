#ifndef ETHERNETLIB_H_
#define ETHERNETLIB_H_

#include "Bsp.h"

#include "Pms/Std/IfxPmsEvr.h"
#include "Vmt/Std/IfxVmt.h"
#include "Geth/Std/IfxGeth.h"
#include "pdl/ifx_hsphy.h"
#include "pdl/ifx_hsphy.h"
#include "phy/dp83825i.h"

/* ---------- Local MDIO helpers for CPU0 pre-sequence ---------- */
#define CPU0_ETH_MDIO_CMD_READ   VAL2FLD(IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CMD, 3)
#define CPU0_ETH_MDIO_CMD_WRITE  VAL2FLD(IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CMD, 1)
#define CPU0_ETH_MDIO_CMD_BUSY   GENMASK(IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SBUSY)

void Cpu0_PrepareEthernetForCpu2(void);
void grantCpu2PeripheralAccess (void);

#endif
