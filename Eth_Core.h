#ifndef ETH_CORE_H_
#define ETH_CORE_H_

#include "Ifx_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Minimal Ethernet + IPv4/UDP transport for TC4D7 GETH0 on CPU2.
 *
 * Important scope:
 * - This is NOT a full IP stack.
 * - It sends a manually-built IPv4/UDP frame through GETH DMA.
 * - No ARP is implemented here.
 * - For the easiest bring-up, use broadcast destination MAC/IP.
 *
 * Practical implication for One Eye:
 * - Configure One Eye to listen on UDP port remotePort.
 * - The payload bytes are exactly the bytes you pass to Eth_Core2_sendUdp().
 * - If you pass a C struct, One Eye must interpret the same binary layout.
 */

#define ETH_CORE2_MAX_UDP_PAYLOAD   (1472u)
#define ETH_CORE2_LINK_MONITOR_MS   (200u)

typedef struct
{
    uint8   localMac[6];
    uint8   remoteMac[6];
    uint8   localIp[4];
    uint8   remoteIp[4];
    uint16  localPort;
    uint16  remotePort;
} Eth_Core2_Config;

void    Eth_Core2_getDefaultConfig(Eth_Core2_Config *cfg);
boolean Eth_Core2_init(const Eth_Core2_Config *cfg);
void    Eth_Core2_service1ms(void);
boolean Eth_Core2_isLinkUp(void);
boolean Eth_Core2_sendUdp(const void *payload, uint16 payloadLength);

#ifdef __cplusplus
}
#endif

#endif /* ETH_CORE_H_ */
