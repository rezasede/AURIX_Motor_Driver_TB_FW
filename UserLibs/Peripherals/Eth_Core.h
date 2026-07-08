/**
 * \file Eth_Core.h
 * \brief Minimal Ethernet + IPv4/UDP transport for TC4D7 GETH0 on CPU2
 *
 * Changes from original:
 *   - Added Eth_Core2_sendUdpTo() with explicit port parameters
 *   - Added Eth_Core2_UdpRxCallback and Eth_Core2_registerUdpRxCallback()
 *   - service1ms now processes incoming frames instead of discarding them
 */
#ifndef ETH_CORE_H_
#define ETH_CORE_H_

#include "Ifx_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * \brief UDP receive callback type
 *
 * Called when a UDP packet is received.
 * \param payload    Pointer to UDP payload data
 * \param length     Payload length in bytes
 * \param srcPort    Source port from UDP header
 * \param dstPort    Destination port from UDP header
 */
typedef void (*Eth_Core2_UdpRxCallback)(const uint8* payload, uint16 length,
                                         uint16 srcPort, uint16 dstPort);

/* --- Existing API (unchanged) --- */
void    Eth_Core2_getDefaultConfig(Eth_Core2_Config *cfg);
boolean Eth_Core2_init(const Eth_Core2_Config *cfg);
void    Eth_Core2_service1ms(void);
boolean Eth_Core2_isLinkUp(void);

/** Send UDP using config ports (localPort → remotePort) */
boolean Eth_Core2_sendUdp(const void *payload, uint16 payloadLength);

/* --- New API --- */

/**
 * \brief Send UDP with explicit source and destination ports
 *
 * Same as Eth_Core2_sendUdp but allows specifying ports per-call.
 * Use this when you need different ports for different protocols
 * (e.g. OneEye on port 65000, raw data on port 50001).
 */
boolean Eth_Core2_sendUdpTo(const void *payload, uint16 payloadLength,
                             uint16 srcPort, uint16 dstPort);

/**
 * \brief Register a callback for received UDP packets
 *
 * The callback is invoked from Eth_Core2_service1ms() for each
 * received UDP packet. Only one callback can be registered at a time.
 *
 * \param callback  Function to call, or NULL to disable
 */
void Eth_Core2_registerUdpRxCallback(Eth_Core2_UdpRxCallback callback);

#ifdef __cplusplus
}
#endif

#endif /* ETH_CORE_H_ */
