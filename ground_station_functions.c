

#include "ground_station.h"

#include <stdio.h>
#include <time.h>




void display_received_packet(const ReceivedPacket *packet){
    printf("\n     TELEMETRY PACKET     \n");
    printf("Sequence number : %u\n",packet->sequence_number);
    time_t timestamp=(time_t)packet->timestamp;
    printf("Timestamp       : %s", ctime(&timestamp));
    printf("Mode            : %s\n",packet->mode);
    printf("Temperature     : %.2f C\n",packet->temperature);
    printf("Battery         : %.2f %%\n",packet->battery);
    printf("Voltage         : %.2f V\n",packet->voltage);
    printf("Orientation     : X=%.2f, Y=%.2f, Z=%.2f\n",packet->orientation_x,packet->orientation_y,packet->orientation_z);
    printf("Over temperature: %d\n",packet->over_temperature);
    printf("Low battery     : %d\n",packet->low_battery);
    printf("Low voltage     : %d\n\n",packet->low_voltage);
    if(packet->over_temperature){printf("WARNING: SATELLITE OVERHEATING\n");}
    if(packet->low_battery){printf("WARNING: LOW BATTERY\n");}
    printf("--------------------------------------\n\n");
}

void log_packet(const ReceivedPacket *packet){
    FILE *file = fopen("telemetry_log.csv", "a");

    if (file == NULL) {
        perror("fopen");
        return;
    }

    fprintf(
        file,
        "%u,%ld,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d,%d\n",
        packet->sequence_number,
        packet->timestamp,
        packet->mode,
        packet->temperature,
        packet->battery,
        packet->voltage,
        packet->orientation_x,
        packet->orientation_y,
        packet->orientation_z,
        packet->over_temperature,
        packet->low_battery,
        packet->low_voltage
    );

    fclose(file);
}


void parse_packet(const char *message, ReceivedPacket *packet){
    int message_read=sscanf(
        message,
        "%u,%ld,%15[^,],%f,%f,%f,%f,%f,%f,%d,%d,%d",
        &packet->sequence_number,
        &packet->timestamp,
        packet->mode,
        &packet->temperature,
        &packet->battery,
        &packet->voltage,
        &packet->orientation_x,
        &packet->orientation_y,
        &packet->orientation_z,
        &packet->over_temperature,
        &packet->low_battery,
        &packet->low_voltage);
        if (message_read == 12) {
        display_received_packet(packet);
        log_packet(packet);
    }
}