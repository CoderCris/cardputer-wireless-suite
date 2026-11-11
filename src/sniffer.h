// sniffer.h — Sniffer WiFi del hito 3, pieza 1: contadores por tipo de trama.
// Ver sniffer.cpp y docs/04-protocols/wifi-radio-esp32s3.md.
#pragma once
#include <stdint.h>
#include <esp_err.h>

// Copia de los contadores en un instante. Los valores solo crecen desde el
// arranque; la tasa por segundo la calcula quien lee, restando dos copias.
struct SnifferCuentas {
    uint32_t mgmt;   // gestión: beacons, probes, auth, deauth...
    uint32_t data;   // datos (cifrados o no; aquí solo se cuentan)
    uint32_t misc;   // lo que el PHY recibe pero no entrega como trama 802.11
};

// Arranca el radio en estación sin asociarse, fija el canal y activa el modo
// promiscuo. Devuelve el primer error de ESP-IDF, o ESP_OK.
esp_err_t sniffer_begin(uint8_t canal);

// Lee los contadores. Se puede llamar desde loop() (núcleo 1) mientras el
// callback escribe en el núcleo 0.
SnifferCuentas sniffer_leer();
