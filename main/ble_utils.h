#pragma once

#include "stdint.h"
#include "host/ble_hs.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Utility function to log an array of bytes.
 */
void print_bytes(const uint8_t *bytes, int len);

void print_addr(const void *addr);

/**
 * Logs information about a connection to the console.
 */
void bleprph_print_conn_desc(struct ble_gap_conn_desc *desc);

#ifdef __cplusplus
}
#endif