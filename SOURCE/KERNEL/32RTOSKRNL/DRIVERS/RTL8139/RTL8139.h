#ifndef RTL8139_DRIVER_H
#define RTL8139_DRIVER_H
#include <STD/TYPEDEF.h>

BOOLEAN RTL8139_START();
BOOLEAN RTL8139_STATUS();
BOOLEAN RTL8139_STOP(); // NOTE: This will ALWAYS return FALSE

typedef struct {
    U8 receiver[6];
    U8 sender[6];
    U16 type;
    U32 header; // Additional information
    U32 id; // Id of packet
    U32 data_len;
    U8 data[1];
} ATTRIB_PACKED ETH_PACKET;

#define ETH_TYPE_IPV4 0x0800
#define ETH_TYPE_ARP  0x0806
/**
 * @brief Sends an Ethernet frame with a given payload to a destination MAC address.
 * * @param dst_mac The 6-byte destination MAC address.
 * @param payload The data to send (e.g., an IP packet).
 * @param payload_len The length of the payload data.
 * @param ether_type The 16-bit EtherType (e.g., ETH_TYPE_IPV4).
 * @return TRUE if the packet was successfully queued for transmission, FALSE otherwise.
 */
BOOL RTL8139_SEND_PACKET_TO_MAC(const U8 dst_mac[6], const U8 *payload, U32 payload_len, U16 ether_type);


#endif // RTL8139_DRIVER_H