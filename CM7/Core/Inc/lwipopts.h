/**
 * lwipopts.h — Configuration LwIP minimaliste pour STM32H747I-DISCO
 * IP Statique : 192.168.137.15
 * Protocole   : UDP seulement (envoi de données capteurs vers Raspberry Pi)
 */
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/* Pas d'OS — mode polling simple */
#define NO_SYS                          1
#define SYS_LIGHTWEIGHT_PROT            0

/* Mémoire */
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (16 * 1024)   // 16 Ko heap LwIP
#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_UDP_PCB                4
#define MEMP_NUM_TCP_PCB                0
#define MEMP_NUM_TCP_PCB_LISTEN         0
#define MEMP_NUM_TCP_SEG                0
#define MEMP_NUM_SYS_TIMEOUT            6
#define PBUF_POOL_SIZE                  16
#define PBUF_POOL_BUFSIZE               1524

/* Protocoles activés */
#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_IPV4                       1
#define LWIP_UDP                        1
#define LWIP_TCP                        0    // TCP désactivé
#define LWIP_ICMP                       1    // Ping activé
#define LWIP_DHCP                       0    // IP statique — pas de DHCP
#define LWIP_DNS                        0
#define LWIP_SNMP                       0

/* Checksum matériel (STM32H7 ETH DMA) */
#define CHECKSUM_GEN_IP                 0
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0
#define LWIP_CHECKSUM_ON_COPY           0

/* MAC */
#define ETH_MAC_ADDR0                   0x02
#define ETH_MAC_ADDR1                   0x00
#define ETH_MAC_ADDR2                   0x00
#define ETH_MAC_ADDR3                   0x00
#define ETH_MAC_ADDR4                   0x00
#define ETH_MAC_ADDR5                   0x15   // Dernier octet = 15 (cohérent avec IP .15)

/* Divers */
#define LWIP_NETIF_HOSTNAME             1
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1
#define LWIP_STATS                      0
#define LWIP_PROVIDE_ERRNO              1

#endif /* __LWIPOPTS_H__ */
