// config/btstack_config.h
// Minimal config for a BLE Central (scanner + GATT client) application.
// Based on pico-examples/pico_w/bt/config/btstack_config.h

#ifndef BTSTACK_CONFIG_H
#define BTSTACK_CONFIG_H

// Enable BLE central role
#define ENABLE_BLE
#define ENABLE_LE_CENTRAL

// Logging (remove ENABLE_LOG_DEBUG for less noise)
#define ENABLE_LOG_ERROR
#define ENABLE_LOG_INFO
// #define ENABLE_LOG_DEBUG
#define ENABLE_PRINTF_HEXDUMP

// Buffers
#define HCI_ACL_PAYLOAD_SIZE            (255 + 4)
#define MAX_NR_HCI_CONNECTIONS          1
#define MAX_NR_LE_DEVICE_DB_ENTRIES     4
#define MAX_NR_GATT_CLIENTS             1
#define MAX_NR_SM_LOOKUP_ENTRIES        3
#define MAX_NR_WHITELIST_ENTRIES        1
#define MAX_NR_RESOLVING_LIST_ENTRIES   1

// ATT MTU — minimum 23; printer won't negotiate higher but that's fine
#define ATT_DEFAULT_MTU                 23

// We don't need pairing/bonding
// #define ENABLE_LE_SECURE_CONNECTIONS

#endif // BTSTACK_CONFIG_H
