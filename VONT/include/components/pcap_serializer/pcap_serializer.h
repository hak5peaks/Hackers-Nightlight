#pragma once
#include <stdint.h>
#include <Arduino.h>

typedef struct {
            uint32_t magic_number;   /* magic number */
            uint16_t version_major;  /* major version number */
            uint16_t version_minor;  /* minor version number */
            int32_t  thiszone;       /* GMT to local correction */
            uint32_t sigfigs;        /* accuracy of timestamps */
            uint32_t snaplen;        /* max length of captured packets, in octets */
            uint32_t network;        /* data link type */
} pcap_global_header_t;


typedef struct {
        uint32_t ts_sec;         /* timestamp seconds */
        uint32_t ts_usec;        /* timestamp microseconds */
        uint32_t incl_len;       /* number of octets of packet saved in file */
        uint32_t orig_len;       /* actual length of packet */
} pcap_record_header_t;


class pcap_serializer
{
public:
    uint8_t *pcap_serializer_init();
    void pcap_serializer_append_frame(const uint8_t *buffer, unsigned size, unsigned ts_usec);
    void pcap_serializer_deinit();
    unsigned pcap_serializer_get_size();
    uint8_t *pcap_serializer_get_buffer();
    unsigned int pcap_size = 0;
    uint8_t *pcap_buffer = NULL;
};
