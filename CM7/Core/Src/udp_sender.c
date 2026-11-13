/**
 * udp_sender.c — Envoi JSON via UDP vers Raspberry Pi 4
 * 
 * Format envoyé (exemple) :
 * {"speed":120.5,"temp":23.1,"fuel":75.0,"lidar":45}
 */

#include "udp_sender.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include <stdio.h>
#include <string.h>

static struct udp_pcb *udp_pcb = NULL;
static ip_addr_t dest_ip;

/**
 * @brief Initialise le PCB UDP et configure l'adresse de destination
 */
int UDP_Sender_Init(void)
{
    /* Créer le PCB UDP */
    udp_pcb = udp_new();
    if (udp_pcb == NULL) return -1;

    /* Configurer l'IP de destination : 192.168.137.13 (Raspberry Pi) */
    IP4_ADDR(&dest_ip,
             UDP_DEST_IP0,
             UDP_DEST_IP1,
             UDP_DEST_IP2,
             UDP_DEST_IP3);

    /* Lier sur le port local 0 (attribution automatique) */
    if (udp_bind(udp_pcb, IP_ADDR_ANY, 0) != ERR_OK) {
        udp_remove(udp_pcb);
        udp_pcb = NULL;
        return -1;
    }

    return 0;
}

/**
 * @brief Envoie les données capteurs sous forme JSON
 */
void UDP_SendSensorData(float speed, float temperature, float fuel, float lidar_cm)
{
    if (udp_pcb == NULL) return;

    /* Construire le JSON */
    char json_buf[128];
    int len;

    if (lidar_cm < 0.0f) {
        /* LiDAR non disponible */
        len = snprintf(json_buf, sizeof(json_buf),
            "{\"speed\":%.1f,\"temp\":%.1f,\"fuel\":%.1f,\"lidar\":null}",
            speed, temperature, fuel);
    } else {
        len = snprintf(json_buf, sizeof(json_buf),
            "{\"speed\":%.1f,\"temp\":%.1f,\"fuel\":%.1f,\"lidar\":%.0f}",
            speed, temperature, fuel, lidar_cm);
    }

    if (len <= 0) return;

    /* Allouer un pbuf LwIP */
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, (u16_t)len, PBUF_RAM);
    if (p == NULL) return;

    /* Copier les données JSON */
    memcpy(p->payload, json_buf, len);

    /* Envoyer via UDP */
    udp_sendto(udp_pcb, p, &dest_ip, UDP_DEST_PORT);

    /* Libérer le pbuf */
    pbuf_free(p);
}
