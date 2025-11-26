#include "connection.h"
#include "app_errors.h"
#include "fujinet-network.h"
#include "system.h"
#include <string.h>
#include <stdio.h>

// Extern from fujinet-lib if available, or define locally if needed
// But fujinet-network.h should handle it. 
// In the example, they use 'err' global.

bool connect_service(const char* url) {
    uint8_t result = network_init();
    if (result != FN_ERR_OK) {
        err = result;
        handle_err("network_init");
        return false;
    }

    result = network_open(url, OPEN_MODE_RW, 0);
    if (result != FN_ERR_OK) {
        err = result;
        // Don't exit immediately on open failure, let caller handle? 
        // But handle_err exits.
        // For yail, we might want to return false and let the UI show error.
        // But handle_err is designed to exit.
        // I'll set err but maybe not call handle_err if I want soft failure?
        // The example calls handle_err.
        return false;
    }
    return true;
}

void disconnect_service(const char* url) {
    network_close(url);
}

bool send_data(const char* url, const uint8_t* data, uint16_t len) {
    uint8_t result = network_write(url, data, len);
    if (result != FN_ERR_OK) {
        err = result;
        return false;
    }
    return true;
}

bool read_response_wait(const char* url, uint8_t *buf, uint16_t len) {
    uint16_t total_read = 0;
    uint32_t retries = 0;
    const uint32_t MAX_RETRIES = 400000L; // Wait a while for server, but allow abort

    while (total_read < len) {
        int16_t n = network_read(url, buf + total_read, len - total_read);
        
        if (n < 0) {
            err = -n;
            return false;
        }
        
        if (n == 0) {
            // No data yet
            retries++;
            if (retries > MAX_RETRIES) {
                // Timeout
                return false;
            }

            // Check for user abort
            if (sys_key_pressed()) {
                sys_get_key(); // Consume the key
                // Maybe set a specific error for abort?
                return false;
            }

            continue;
        }
        
        // Got data
        total_read += n;
        // Optional: reset retries if we want "idle timeout" vs "total timeout". 
        // For "server generating image", we want total wait to be long initially.
        // If we get partial data, we probably don't want to timeout immediately after.
        retries = 0; 
    }
    return true;
}
