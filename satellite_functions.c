
#include "satellite.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

float random_float(const float min,const float max){
    float ratio = (float)rand() / (float)RAND_MAX;
    return min + ratio * (max - min);
}



void update_sensors(SensorData *data,SatelliteMode mode){
    assert(data != NULL);


    if(mode==MODE_ACTIVE){
        data->temperature+=0.2;
    }
    else if(mode==MODE_LOW_POWER){
        data->temperature+=0.1;
    }
    else if(mode==MODE_SAFE){
        data->temperature-=0.1;
    }
    else if(mode==MODE_IDLE){
        data->temperature+=0.05;
    }

    else if(data->temperature < 0.0) {
    data->temperature = 0.0;
    }


    if(mode==MODE_ACTIVE){
        data->battery--;
    }
    else if(mode==MODE_LOW_POWER){
        data->battery-=0.5;
    }
    else if(mode==MODE_SAFE){
        data->battery+=0.5;  //start solar charging in safe mode
    }
    else if(mode==MODE_IDLE){
        data->battery-=0.1;
    }

    if(data->battery>100.0){
        data->battery=100.0;
    }
    if(data->battery<0.0) {
        data->battery=0.0;
    }

    data->voltage=6.0+2.4*data->battery/100.0;
    data->orientation.x=random_float(-5.0,5.0);
    data->orientation.y=random_float(-5.0,5.0);
    data->orientation.z=random_float(0.0,360.0);
}

SatelliteMode check_system(const FaultStatus faults,const SatelliteMode requested_mode){
    if(faults.over_temperature || faults.low_battery || faults.low_voltage){
        return MODE_SAFE;
    }
        return requested_mode;
    
}

const char *mode_to_string(const SatelliteMode mode){
    switch (mode){
        case MODE_BOOT : return "BOOT"; 
        case MODE_ACTIVE : return "ACTIVE"; 
        case MODE_SAFE : return "SAFE"; 
        case MODE_IDLE : return "IDLE";
        case MODE_LOW_POWER : return "LOW_POWER";
        default : return "UNKNOWN";
    }
}

void display_data(const SensorData data){
    printf("Temperature : %f C\n",data.temperature);
    printf("Battery : %f %%\n",data.battery);
    printf("Voltage : %f V\n",data.voltage);
    printf("Orientation : x=%f°, y=%f°, z=%f°\n",data.orientation.x,data.orientation.y,data.orientation.z);

}

FaultStatus detect_faults(const SensorData *data){
    assert(data != NULL);

    FaultStatus faults;

    faults.over_temperature=data->temperature>60;
    faults.low_battery=data->battery<20;
    faults.low_voltage=data->voltage<6.5;

    return faults;
}

void display_faults(const FaultStatus faults){
    int has_fault=0;

    printf("Faults: ");

    if(faults.over_temperature) {
        printf("OVER_TEMPERATURE ");
        has_fault=1;
    }

    if(faults.low_battery) {
        printf("LOW_BATTERY ");
        has_fault=1;
    }

    if(faults.low_voltage) {
        printf("LOW_VOLTAGE ");
        has_fault=1;
    }

    if(has_fault==0) {
        printf("NONE");
    }

    printf("\n");
}



TelemetryPacket build_packet(unsigned int sequence_number,const SensorData *data,SatelliteMode mode,FaultStatus faults){
    assert(data != NULL);

    TelemetryPacket packet;

    packet.sequence_number=sequence_number;
    packet.timestamp=time(NULL);
    packet.mode=mode;
    packet.sensors=*data;
    packet.faults=faults;

    return packet;

    
}

void display_packet(const TelemetryPacket packet){
    printf("----------------------------\n");
    printf("Displaying packet :\n\n");
    printf("Sequence number : %d\n",packet.sequence_number);
    printf("Time stamp : %s",ctime(&packet.timestamp));
    display_data(packet.sensors);
    display_faults(packet.faults);
    printf("Mode : %s\n",mode_to_string(packet.mode));
    printf("----------------------------\n");
}

int serialize_packet(const TelemetryPacket *packet,char *buffer,size_t buffer_size){

    assert(packet != NULL);
    assert(buffer != NULL);


    return snprintf(
        buffer,
        buffer_size,
        "%u,%ld,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d,%d\n",
        packet->sequence_number,
        (long)packet->timestamp,
        mode_to_string(packet->mode),
        packet->sensors.temperature,
        packet->sensors.battery,
        packet->sensors.voltage,
        packet->sensors.orientation.x,
        packet->sensors.orientation.y,
        packet->sensors.orientation.z,
        packet->faults.over_temperature,
        packet->faults.low_battery,
        packet->faults.low_voltage
    );
}


void handle_command(const char *command,int *telemetry_enabled,SatelliteMode *requested_mode,int satellite_socket){
    const char *response;
    if (strcmp(command,"START")==0) {
        *telemetry_enabled=1;
        *requested_mode=MODE_ACTIVE;
        printf("Responding with START to ground control\n");
        response="STARTING\n";
    }
    else if (strcmp(command,"STOP")==0) {
        *telemetry_enabled=0;
        *requested_mode=MODE_IDLE;
        printf("Responding with STOP to ground control\n");
        response="STOPPING\n";
    }
    else if (strcmp(command,"LOW_POWER")==0) {
        *telemetry_enabled=1;
        *requested_mode=MODE_LOW_POWER;
        printf("Responding with LOW_POWER to ground control\n");
        response="LOW_POWER\n";
    }
    else if (strcmp(command,"ACTIVE")==0) {
        *telemetry_enabled=1;
        *requested_mode=MODE_ACTIVE;
        printf("Responding with ACTIVE to ground control\n");
        response="ACTIVE\n";
    }
    else {
        response="UNKNOWN_COMMAND\n";
    }

    if (send(satellite_socket,response,strlen(response),0)<0){
        perror("send response");
}
}
