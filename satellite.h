
#pragma once
#include <stddef.h>
#include <time.h>

typedef struct{
    float x,y,z;
} Orientation;

typedef struct{
    float temperature;
    float battery;
    float voltage;
    Orientation orientation;
} SensorData;

typedef enum {
    MODE_BOOT,
    MODE_IDLE,
    MODE_ACTIVE,
    MODE_LOW_POWER,
    MODE_SAFE
} SatelliteMode;

typedef struct{
    int over_temperature;
    int low_battery;
    int low_voltage;
} FaultStatus;


typedef struct {
    unsigned int sequence_number;
    time_t timestamp;
    SatelliteMode mode;
    SensorData sensors;
    FaultStatus faults;
} TelemetryPacket;



float random_float(const float min,const float max);
void update_sensors(SensorData *data, SatelliteMode mode);
SatelliteMode check_system(const FaultStatus faults,const SatelliteMode requested_mode);
const char *mode_to_string(const SatelliteMode mode);
void display_data(const SensorData data);
FaultStatus detect_faults(const SensorData *data);
void display_faults(const FaultStatus faults);
TelemetryPacket build_packet(unsigned int sequence_number,const SensorData *data,SatelliteMode mode,FaultStatus faults);
void display_packet(const TelemetryPacket packet);
int serialize_packet(const TelemetryPacket *packet,char *buffer,size_t buffer_size);
void handle_command(const char *command,int *telemetry_enabled,SatelliteMode *requested_mode,int satellite_socket);