#ifndef CONNECTION_H
#define CONNECTION_H

#include "stdint_shim.h"
#include <stdbool.h>

// Initialize and open connection to the configured URL
bool connect_service(const char* url);

// Close connection
void disconnect_service(const char* url);

// Write data to the network
// Returns true on success, false on failure (sets err)
bool send_data(const char* url, const uint8_t* data, uint16_t len);

// Read data from the network, waiting for full length
// Returns true on success, false on failure (sets err)
bool read_response_wait(const char* url, uint8_t *buf, uint16_t len);

#endif // CONNECTION_H
