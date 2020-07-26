#include "utils.h"
#include <sys/socket.h>
#include <arpa/inet.h> /*for inet_ntop & inet_pton*/
#include <stdint.h>
#include <string.h>

void 
apply_mask(char *prefix, char mask, char *str_prefix)
{
    uint32_t binary_prefix = 0;
    uint32_t subnet_mask = 0xFFFFFFFF;

    if (mask == 32) {
        strncpy(str_prefix, prefix, 16);
        str_prefix[15] = '\0';
        return;
    }

    /* Convert string ip addr to binary */
    inet_pton(AF_INET, prefix, &binary_prefix);
    /* Convert host byte addr to network byte (little to big endian) */
    binary_prefix = htonl(binary_prefix);

    subnet_mask = subnet_mask << (32 - mask);
    binary_prefix = binary_prefix & subnet_mask;
    
    /* Convert final binary IP back to string */
    binary_prefix = htonl(binary_prefix);
    inet_ntop(AF_INET, &binary_prefix, str_prefix,16);
    str_prefix[15] = '\0';
}

void
layer2_fill_with_broadcast_mac(char *mac_array)
{
    mac_array[0] = 0xFF;
    mac_array[1] = 0xFF;
    mac_array[2] = 0xFF;
    mac_array[3] = 0xFF;
    mac_array[4] = 0xFF;
    mac_array[5] = 0xFF;
}

