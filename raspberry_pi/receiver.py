#!/usr/bin/env python3
"""
receiver.py — Récepteur UDP pour données capteurs STM32H747I-DISCO
Écoute sur le port 5005, affiche les données JSON reçues

Usage :
    python3 receiver.py

Données reçues :
    {"speed":120.5,"temp":23.1,"fuel":75.0,"lidar":45}
"""

import socket
import json
import datetime

# Configuration
HOST = "0.0.0.0"   # Écoute sur toutes les interfaces
PORT = 5005         # Même port que configuré sur la STM32

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((HOST, PORT))
    print(f"[UDP Receiver] Écoute sur {HOST}:{PORT} ...")
    print(f"[UDP Receiver] En attente de données de la STM32 (192.168.137.15)...\n")

    while True:
        try:
            data, addr = sock.recvfrom(1024)
            timestamp = datetime.datetime.now().strftime("%H:%M:%S")

            try:
                parsed = json.loads(data.decode('utf-8'))
                speed  = parsed.get("speed", 0)
                temp   = parsed.get("temp", 0)
                fuel   = parsed.get("fuel", 0)
                lidar  = parsed.get("lidar", None)

                lidar_str = f"{lidar:.0f} cm" if lidar is not None else "---"

                print(f"[{timestamp}] De {addr[0]}")
                print(f"  Vitesse    : {speed:.1f} km/h")
                print(f"  Température: {temp:.1f} °C")
                print(f"  Carburant  : {fuel:.1f} %")
                print(f"  LiDAR      : {lidar_str}")
                print()

            except json.JSONDecodeError:
                print(f"[{timestamp}] Données brutes : {data.decode('utf-8', errors='replace')}")

        except KeyboardInterrupt:
            print("\n[UDP Receiver] Arrêt.")
            break

    sock.close()

if __name__ == "__main__":
    main()
