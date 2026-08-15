

#pragma once


typedef struct {
    unsigned int sequence_number;
    long timestamp;
    char mode[16];
    float temperature;
    float battery;
    float voltage;
    float orientation_x;
    float orientation_y;
    float orientation_z;
    int over_temperature;
    int low_battery;
    int low_voltage;
}ReceivedPacket;


void display_received_packet(const ReceivedPacket *packet);
void log_packet(const ReceivedPacket *packet);
void parse_packet(const char *message, ReceivedPacket *packet);
