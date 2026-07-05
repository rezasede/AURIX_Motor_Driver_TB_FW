/**
 * \file Eth_Core.c
 * \brief Minimal Ethernet + IPv4/UDP transport for TC4D7 GETH0 on CPU2
 *
 * Changes from original:
 *   - Eth_Core2_discardPendingRxFrames() replaced by Eth_Core2_processRxFrames()
 *     which parses incoming Ethernet/IPv4/UDP packets and calls registered callback
 *   - Added Eth_Core2_sendUdpTo() with explicit port parameters
 *   - Added Eth_Core2_registerUdpRxCallback()
 *   - Original Eth_Core2_sendUdp() now calls sendUdpTo() internally
 */
#include "Eth_Core.h"

#include <string.h>

#include "Boards/board.h"
#include "pin.h"

#include "Clock/Std/IfxClock.h"
#include "Port/Std/IfxPort.h"

#include "Geth/Eth/IfxGeth_Eth.h"
#include "IfxGeth_regdef.h"

#include "pdl/ifx_hsphy.h"
#include "phy/dp83825i.h"

#include "Geth/Std/IfxGeth.h"
#include "Vmt/Std/IfxVmt.h"

#define ETH_CORE2_TX_BUFFER_SIZE    (1528u)
#define ETH_CORE2_RX_BUFFER_SIZE    (1528u)
#define ETH_CORE2_IPV4_HEADER_LEN   (20u)
#define ETH_CORE2_UDP_HEADER_LEN    (8u)
#define ETH_CORE2_ETH_HEADER_LEN    (14u)
#define ETH_CORE2_MIN_FRAME_LEN     (60u)

#define ETH_CORE2_MDIO_CMD_READ     VAL2FLD(IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CMD, 3)
#define ETH_CORE2_MDIO_CMD_WRITE    VAL2FLD(IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CMD, 1)
#define ETH_CORE2_MDIO_CMD_BUSY     GENMASK(IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SBUSY)

#define ETH_CPU2_BSS      __attribute__((section(".zbss_cpu2")))
#define ETH_CPU2_LMU_BSS  __attribute__((section(".lmubss_cpu2")))

static ETH_CPU2_BSS IfxGeth_Eth g_eth;
static ETH_CPU2_BSS phy_t g_phy0;
static ETH_CPU2_BSS boolean g_initialized = FALSE;
static ETH_CPU2_BSS boolean g_linkUp = FALSE;
static ETH_CPU2_BSS uint32 g_msCounter = 0u;
static ETH_CPU2_BSS uint16 g_ipIdent = 0u;
static ETH_CPU2_BSS Eth_Core2_Config g_cfg;

/* >>> NEW: UDP receive callback <<< */
static ETH_CPU2_BSS Eth_Core2_UdpRxCallback g_udpRxCallback;

static const IfxHsphy_Geth_RmiiPins g_geth0RmiiPins = {.crsDiv = &BOARD_GETH0_P0_CSRDV,
        .refClk = &BOARD_GETH0_P0_REFCLK, .rxd0 = &BOARD_GETH0_P0_RXD0, .rxd1 = &BOARD_GETH0_P0_RXD1, .txd0 =
                &BOARD_GETH0_P0_TXD0, .mdc = &BOARD_GETH0_P0_MDC, .txd1 = &BOARD_GETH0_P0_TXD1, .txEn =
                &BOARD_GETH0_P0_TXEN, .mdio = &BOARD_GETH0_P0_MDIO, };

IFX_ALIGN(8) static ETH_CPU2_LMU_BSS IfxGeth_TxDescr g_txDesc[IFXGETH_MAX_TX_DESCRIPTORS];
IFX_ALIGN(8) static ETH_CPU2_LMU_BSS IfxGeth_RxDescr g_rxDesc[IFXGETH_MAX_RX_DESCRIPTORS];

IFX_ALIGN(8) static ETH_CPU2_LMU_BSS uint8 g_txBuffer[IFXGETH_MAX_TX_DESCRIPTORS][ETH_CORE2_TX_BUFFER_SIZE];
IFX_ALIGN(8) static ETH_CPU2_LMU_BSS uint8 g_rxBuffer[IFXGETH_MAX_RX_DESCRIPTORS][ETH_CORE2_RX_BUFFER_SIZE];

/*=============================================================================
 *  MDIO (unchanged from original)
 *===========================================================================*/

static sint32 Eth_Core2_initMdio (void)
{
    uint32 divider;
    IfxGeth_Mdc_Out *mdc;
    IfxGeth_Mdio_InOut *mdio;
    uint32 csrClkRate = IfxClock_getXGeth0Frequency();

    MODULE_GETH0.MACEN.U |= 1U << IfxGeth_PortIndex_0;

    if ((csrClkRate >= 40000000u) && (csrClkRate <= 50000000u))
        divider = 5u;
    else if (csrClkRate >= 350000000u)
        divider = 4u;
    else if (csrClkRate >= 300000000u)
        divider = 3u;
    else if (csrClkRate >= 250000000u)
        divider = 2u;
    else if (csrClkRate >= 150000000u)
        divider = 1u;
    else if (csrClkRate >= 100000000u)
        divider = 0u;
    else
        return -1;

    mdc = g_geth0RmiiPins.mdc;
    mdio = g_geth0RmiiPins.mdio;

    IfxPort_setPinModeInput(mdio->pin.port, mdio->pin.pinIndex, IfxPort_InputMode_noPullDevice);
    IfxPort_setPinPadDriver(mdio->pin.port, mdio->pin.pinIndex, IfxPort_PadDriver_cmosAutomotiveSpeed3);

    IfxPort_setPinModeOutput(mdc->pin.port, mdc->pin.pinIndex, IfxPort_OutputMode_pushPull, mdc->select);
    IfxPort_setPinPadDriver(mdc->pin.port, mdc->pin.pinIndex, IfxPort_PadDriver_cmosAutomotiveSpeed3);

    MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_CONTROL_DATA.U = VAL2FLD(
            IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CR, divider);

    {
        uint32 timeout = 100000u;
        while (MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_CONTROL_DATA.B.SBUSY)
        {
            if (--timeout == 0u)
                return -2;
        }
    }

    return 0;
}

static uint16 Eth_Core2_mdioRead (uint8 phyAddr, uint8 devAddr, uint16 regAddr)
{
    uint32 commandAddr;
    uint32 commandData;

    while (MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_CONTROL_DATA.B.SBUSY)
    {
    }

    MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.CLAUSE_22_PORT.U |= BIT(phyAddr);

    commandAddr = ((uint32) phyAddr << IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_ADDRESS_PA_OFF)
            | ((uint32) regAddr << IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_ADDRESS_RA_OFF)
            | ((uint32) devAddr << IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_ADDRESS_DA_OFF);

    commandData = MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_CONTROL_DATA.U
            & (uint32) ~GENMASK(IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CMD);

    commandData |= ETH_CORE2_MDIO_CMD_READ | ETH_CORE2_MDIO_CMD_BUSY;

    MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_ADDRESS.U = commandAddr;
    MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_CONTROL_DATA.U = commandData;

    while (MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_CONTROL_DATA.B.SBUSY)
    {
    }

    return (uint16) MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_CONTROL_DATA.B.SDATA;
}

static void Eth_Core2_mdioWrite (uint8 phyAddr, uint8 devAddr, uint16 regAddr, uint16 regVal)
{
    uint32 commandAddr;
    uint32 commandData;

    while (MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_CONTROL_DATA.B.SBUSY)
    {
    }

    MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.CLAUSE_22_PORT.U |= BIT(phyAddr);

    commandAddr = ((uint32) phyAddr << IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_ADDRESS_PA_OFF)
            | ((uint32) regAddr << IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_ADDRESS_RA_OFF)
            | ((uint32) devAddr << IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_ADDRESS_DA_OFF);

    commandData = MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_CONTROL_DATA.U
            & ((uint32) ~GENMASK(IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_CMD)
                    & (uint32) ~GENMASK(IFX_GETH_PORT_CORE_MDIO_SINGLE_COMMAND_CONTROL_DATA_SDATA));

    commandData |= ETH_CORE2_MDIO_CMD_WRITE | ETH_CORE2_MDIO_CMD_BUSY | regVal;

    MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_ADDRESS.U = commandAddr;
    MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_CONTROL_DATA.U = commandData;

    while (MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MDIO.SINGLE_COMMAND_CONTROL_DATA.B.SBUSY)
    {
    }
}

/*=============================================================================
 *  Helpers (unchanged)
 *===========================================================================*/

static void Eth_Core2_writeU16Be (uint8 *dst, uint16 value)
{
    dst[0] = (uint8) ((value >> 8) & 0xFFu);
    dst[1] = (uint8) (value & 0xFFu);
}

static uint16 Eth_Core2_readU16Be (const uint8 *src)
{
    return (uint16) (((uint16) src[0] << 8) | (uint16) src[1]);
}

static uint16 Eth_Core2_ipv4Checksum (const uint8 *data, uint16 len)
{
    uint32 sum = 0u;
    uint16 i;

    for (i = 0u; i < len; i += 2u)
    {
        uint16 word = (uint16) (((uint16) data[i] << 8) | (uint16) data[i + 1u]);
        sum += word;

        while ((sum >> 16u) != 0u)
        {
            sum = (sum & 0xFFFFu) + (sum >> 16u);
        }
    }

    return (uint16) (~sum);
}

/*=============================================================================
 *  RX Processing (NEW - replaces discardPendingRxFrames)
 *===========================================================================*/

/**
 * \brief Process all pending received Ethernet frames
 *
 * Parses Ethernet → IPv4 → UDP headers and calls the registered callback
 * with the UDP payload. Non-UDP and non-IPv4 frames are silently discarded.
 *
 * Replaces the original Eth_Core2_discardPendingRxFrames() which threw
 * away all received data.
 */
static void Eth_Core2_processRxFrames (void)
{

    if (g_udpRxCallback == NULL)
    {
        return;
    }

    volatile IfxGeth_RxDescr *rxDesc;

    rxDesc = IfxGeth_Eth_getActualRxDescriptor(&g_eth, IfxGeth_RxDmaChannel_0);

    while (rxDesc->RDES3.W.OWN == 0u)
    {
        /* Only process if we have a callback registered */
        if (g_udpRxCallback != NULL)
        {
            /* Get packet length from descriptor (bits [14:0] of RDES3) */
            uint16 frameLen = (uint16) (rxDesc->RDES3.U & 0x7FFFu);

            /* Calculate which RX buffer this descriptor corresponds to */
            uint32 descIdx = (uint32) (((uint32) rxDesc - (uint32) g_rxDesc) / sizeof(IfxGeth_RxDescr));

            if (descIdx < IFXGETH_MAX_RX_DESCRIPTORS)
            {
                const uint8 *frame = g_rxBuffer[descIdx];

                /* Minimum: Eth(14) + IPv4(20) + UDP(8) = 42 bytes */
                if (frameLen >= (ETH_CORE2_ETH_HEADER_LEN + ETH_CORE2_IPV4_HEADER_LEN + ETH_CORE2_UDP_HEADER_LEN))
                {
                    /* Parse Ethernet header */
                    uint16 etherType = Eth_Core2_readU16Be(&frame[12]);

                    if (etherType == 0x0800u) /* IPv4 */
                    {
                        const uint8 *ipHdr = &frame[ETH_CORE2_ETH_HEADER_LEN];

                        /* IP header length from IHL field (lower nibble of byte 0) × 4 */
                        uint8 ipHdrLen = (uint8) ((ipHdr[0] & 0x0Fu) * 4u);
                        uint8 protocol = ipHdr[9]; /* 17 = UDP */

                        if ((protocol == 17u) && (ipHdrLen >= ETH_CORE2_IPV4_HEADER_LEN))
                        {
                            const uint8 *udpHdr = &ipHdr[ipHdrLen];
                            uint16 srcPort = Eth_Core2_readU16Be(&udpHdr[0]);
                            uint16 dstPort = Eth_Core2_readU16Be(&udpHdr[2]);
                            uint16 udpTotalLen = Eth_Core2_readU16Be(&udpHdr[4]);

                            if (udpTotalLen > ETH_CORE2_UDP_HEADER_LEN)
                            {
                                uint16 payloadLen = udpTotalLen - ETH_CORE2_UDP_HEADER_LEN;
                                const uint8 *payload = &udpHdr[ETH_CORE2_UDP_HEADER_LEN];

                                /* Sanity check: payload must fit within frame */
                                uint16 expectedLen = ETH_CORE2_ETH_HEADER_LEN + ipHdrLen + ETH_CORE2_UDP_HEADER_LEN
                                        + payloadLen;

                                if (expectedLen <= frameLen)
                                {
                                    g_udpRxCallback(payload, payloadLen, srcPort, dstPort);
                                }
                            }
                        }
                    }
                }
            }
        }

        /* Give descriptor back to DMA for next receive */
        IfxGeth_Eth_shuffleRxDescriptor(&g_eth, IfxGeth_RxDmaChannel_0);
        rxDesc = IfxGeth_Eth_getActualRxDescriptor(&g_eth, IfxGeth_RxDmaChannel_0);
    }
}

static void Eth_Core2_updateLinkState (void)
{
    if (dp83825i_update_link(&g_phy0))
    {
        if (g_phy0.link_status.is_up == false)
        {
            MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MAC_RX_CONFIGURATION.B.RE = 0u;
            MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MAC_TX_CONFIGURATION.B.TE = 0u;
            g_linkUp = FALSE;
        }
        else
        {
            MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MAC_RX_CONFIGURATION.B.RE = 1u;
            MODULE_GETH0.PORT[IfxGeth_PortIndex_0].CORE.MAC_TX_CONFIGURATION.B.TE = 1u;
            g_linkUp = TRUE;
        }
    }
}

static void Eth_Core2_initGethModule (void)
{
    IfxGeth_Eth_Config cfg;

    IfxHsphy_Geth_setupRmiiInputPins(&MODULE_HSPHY, IfxHsphy_EthIndex_0, &g_geth0RmiiPins);

    IfxGeth_Eth_initModuleConfig(&cfg, &MODULE_GETH0);

    cfg.port[0].phyInterfaceMode = IfxGeth_PhyInterfaceMode_rmii_100;
    cfg.port[0].mac.disableCrcCheck = FALSE;

    cfg.port[0].mtl.rxQueue[0].enable = TRUE;
    cfg.port[0].mtl.rxQueue[0].enableDynamicDmaChannelMap = TRUE;
    cfg.port[0].mtl.rxQueue[0].rxQueueSize = (4096u >> 8u) - 1u;

    cfg.port[0].mtl.txQueue[0].enable = TRUE;
    cfg.port[0].mtl.txQueue[0].txQueueSize = (4096u >> 8u) - 1u;

    memcpy(cfg.port[0].mac.macAddress, g_cfg.localMac, sizeof(g_cfg.localMac));

    cfg.dma.addressAlignedBeatsEnabled = TRUE;
    cfg.dma.burstLength = IfxGeth_DmaBurstLength_16;
    cfg.dma.undefinedBurstLength = FALSE;
    memset(cfg.dma.burstLengthMultiplierEnable, FALSE, sizeof(cfg.dma.burstLengthMultiplierEnable));

    cfg.dma.txChannel[0].channelEnable = TRUE;
    cfg.dma.txChannel[0].maxBurstLength = IfxGeth_TxBurstLength_16;
    cfg.dma.txChannel[0].txDescrList = (IfxGeth_TxDescrList*) g_txDesc;
    cfg.dma.txChannel[0].txBuffer1Size = ETH_CORE2_TX_BUFFER_SIZE;
    cfg.dma.txChannel[0].txBuffer1StartAddress = (uint32*) g_txBuffer;

    cfg.dma.rxChannel[0].channelEnable = TRUE;
    cfg.dma.rxChannel[0].maxBurstLength = IfxGeth_RxBurstLength_16;
    cfg.dma.rxChannel[0].rxDescrList = (IfxGeth_RxDescrList*) g_rxDesc;
    cfg.dma.rxChannel[0].rxBuffer1Size = ETH_CORE2_RX_BUFFER_SIZE;
    cfg.dma.rxChannel[0].rxBuffer1StartAddress = (uint32*) g_rxBuffer;

    cfg.dma.rxInterrupt[IfxGeth_DmaChannel_0].priority = 0u;
    cfg.dma.txInterrupt[IfxGeth_DmaChannel_0].priority = 0u;

    cfg.bridge.mode = IfxGeth_BridgePortMode_singlePort0;

    IfxGeth_Eth_initModule(&g_eth, &cfg);

    if (MODULE_GETH0.CLC.B.DISS != 0u)
    {
        while (1)
            ;
    }

    /* Accept all packets (promiscuous) — needed since we don't have ARP */
    MODULE_GETH0.PORT[0].CORE.MAC_PACKET_FILTER.U = 0u;

    MODULE_GETH0.DMA.CH[IfxGeth_RxDmaChannel_0].RX_CONTROL.B.SR = 1u;
    MODULE_GETH0.DMA.CH[IfxGeth_TxDmaChannel_0].TX_CONTROL.B.ST = 1u;

    IfxHsphy_Geth_setupRmiiOutputPins(&MODULE_HSPHY, &g_geth0RmiiPins);
}

/*=============================================================================
 *  Public API
 *===========================================================================*/

void Eth_Core2_getDefaultConfig (Eth_Core2_Config *cfg)
{
    static const Eth_Core2_Config defaultCfg = {.localMac = {0x02u, 0x00u, 0x00u, 0x00u, 0x4Du, 0x70u},
    /* Keep broadcast MAC since this minimal stack lacks ARP,
     or change it to your computer's actual MAC address */
    .remoteMac = {0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu},

    /* AURIX Board IP (Must be unique on the subnet) */
    .localIp = {192u, 168u, 178u, 10u},

    /* Computer Static IP */
    .remoteIp = {192u, 168u, 178u, 100u},

    .localPort = 9870u, .remotePort = 9870u, };

    if (cfg != NULL_PTR)
    {
        *cfg = defaultCfg;
    }
}

boolean Eth_Core2_init (const Eth_Core2_Config *cfg)
{
    if (cfg == NULL_PTR)
    {
        return FALSE;
    }

    g_cfg = *cfg;
    g_msCounter = 0u;
    g_ipIdent = 0u;
    g_linkUp = FALSE;
    g_initialized = FALSE;
    g_udpRxCallback = NULL; /* NEW */

    Eth_Core2_initGethModule();

    if (Eth_Core2_initMdio() != 0)
    {
        return FALSE;
    }

    if (dp83825i_init(&g_phy0,
    BOARD_GETH0_P0_PHYADR, Eth_Core2_mdioRead, Eth_Core2_mdioWrite, 0u) != 0)
    {
        return FALSE;
    }

    dp83825i_cfg_link(&g_phy0, LINK_FULL_100BASE_T);

    g_initialized = TRUE;

    Eth_Core2_updateLinkState();

    /* >>> CHANGED: Process (don't discard) any frames already in the buffer <<< */
    Eth_Core2_processRxFrames();

    return TRUE;
}

void Eth_Core2_service1ms (void)
{
    if (!g_initialized)
    {
        return;
    }

    g_msCounter++;

    if ((g_msCounter % ETH_CORE2_LINK_MONITOR_MS) == 0u)
    {
        Eth_Core2_updateLinkState();
    }

    /* >>> CHANGED: Process received frames instead of discarding them <<< */
    Eth_Core2_processRxFrames();
}

boolean Eth_Core2_isLinkUp (void)
{
    return (boolean) (g_initialized && g_linkUp);
}

void Eth_Core2_registerUdpRxCallback (Eth_Core2_UdpRxCallback callback)
{
    g_udpRxCallback = callback;
}

/*-----------------------------------------------------------------------------
 *  Internal: build and send a UDP frame with explicit ports
 *---------------------------------------------------------------------------*/
static boolean Eth_Core2_sendUdpInternal (const void *payload, uint16 payloadLength, uint16 srcPort, uint16 dstPort)
{
    uint8 *txBuf;
    uint16 ipTotalLength;
    uint16 frameLength;
    uint8 *ipHeader;
    uint8 *udpHeader;

    if ((!g_initialized) || (!g_linkUp) || (payload == NULL_PTR))
    {
        return FALSE;
    }

    if ((payloadLength == 0u) || (payloadLength > ETH_CORE2_MAX_UDP_PAYLOAD))
    {
        return FALSE;
    }

    txBuf = (uint8*) IfxGeth_Eth_getTransmitBuffer(&g_eth, IfxGeth_TxDmaChannel_0);

    if (txBuf == NULL_PTR)
    {
        return FALSE;
    }

    memset(txBuf, 0, ETH_CORE2_TX_BUFFER_SIZE);

    /* Ethernet header */
    memcpy(&txBuf[0], g_cfg.remoteMac, sizeof(g_cfg.remoteMac));
    memcpy(&txBuf[6], g_cfg.localMac, sizeof(g_cfg.localMac));
    Eth_Core2_writeU16Be(&txBuf[12], 0x0800u);

    /* IPv4 header */
    ipHeader = &txBuf[ETH_CORE2_ETH_HEADER_LEN];
    ipTotalLength = (uint16) (ETH_CORE2_IPV4_HEADER_LEN + ETH_CORE2_UDP_HEADER_LEN + payloadLength);

    ipHeader[0] = 0x45u;
    ipHeader[1] = 0x00u;
    Eth_Core2_writeU16Be(&ipHeader[2], ipTotalLength);
    Eth_Core2_writeU16Be(&ipHeader[4], g_ipIdent++);
    Eth_Core2_writeU16Be(&ipHeader[6], 0x4000u);
    ipHeader[8] = 64u;
    ipHeader[9] = 17u;
    Eth_Core2_writeU16Be(&ipHeader[10], 0u);
    memcpy(&ipHeader[12], g_cfg.localIp, sizeof(g_cfg.localIp));
    memcpy(&ipHeader[16], g_cfg.remoteIp, sizeof(g_cfg.remoteIp));
    Eth_Core2_writeU16Be(&ipHeader[10], Eth_Core2_ipv4Checksum(ipHeader, ETH_CORE2_IPV4_HEADER_LEN));

    /* UDP header — using explicit ports */
    udpHeader = &ipHeader[ETH_CORE2_IPV4_HEADER_LEN];
    Eth_Core2_writeU16Be(&udpHeader[0], srcPort);
    Eth_Core2_writeU16Be(&udpHeader[2], dstPort);
    Eth_Core2_writeU16Be(&udpHeader[4], (uint16) (ETH_CORE2_UDP_HEADER_LEN + payloadLength));
    Eth_Core2_writeU16Be(&udpHeader[6], 0u);

    /* Payload */
    memcpy(&udpHeader[ETH_CORE2_UDP_HEADER_LEN], payload, payloadLength);

    frameLength = (uint16) (ETH_CORE2_ETH_HEADER_LEN + ipTotalLength);

    if (frameLength < ETH_CORE2_MIN_FRAME_LEN)
    {
        frameLength = ETH_CORE2_MIN_FRAME_LEN;
    }

    IfxGeth_Eth_sendTransmitBuffer(&g_eth, IfxGeth_PortIndex_0, frameLength, IfxGeth_TxDmaChannel_0);
    return TRUE;
}

boolean Eth_Core2_sendUdp (const void *payload, uint16 payloadLength)
{
    /* Original behavior: use config ports */
    return Eth_Core2_sendUdpInternal(payload, payloadLength, g_cfg.localPort, g_cfg.remotePort);
}

boolean Eth_Core2_sendUdpTo (const void *payload, uint16 payloadLength, uint16 srcPort, uint16 dstPort)
{
    return Eth_Core2_sendUdpInternal(payload, payloadLength, srcPort, dstPort);
}
