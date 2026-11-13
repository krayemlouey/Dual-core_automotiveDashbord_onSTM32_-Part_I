/**
 * udp_sender.h — Envoi de données capteurs via UDP vers Raspberry Pi 4
 * STM32 IP  : 192.168.137.15
 * Pi IP     : 192.168.137.13
 * Port UDP  : 5005
 * Format    : JSON
 */
#ifndef __UDP_SENDER_H__
#define __UDP_SENDER_H__

#include "lwip/netif.h"

/* Configuration réseau */
#define UDP_DEST_IP0   192
#define UDP_DEST_IP1   168
#define UDP_DEST_IP2   137
#define UDP_DEST_IP3   13   // IP du Raspberry Pi

#define UDP_DEST_PORT  5005

/**
 * @brief Initialise le socket UDP (à appeler une fois après LwIP init)
 * @return 0 si succès, -1 si erreur
 */
int UDP_Sender_Init(void);

/**
 * @brief Envoie les données capteurs au format JSON vers le Pi
 * @param speed      Vitesse en km/h
 * @param temperature Température en °C
 * @param fuel       Carburant en %
 * @param lidar_cm   Distance LiDAR en cm (-1 si invalide)
 */
void UDP_SendSensorData(float speed, float temperature, float fuel, float lidar_cm);

#endif /* __UDP_SENDER_H__ */
