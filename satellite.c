#include "satellite.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 5000



int main(){
    srand((unsigned int)time(NULL));

    char message[256];

    SensorData data={
    .temperature=59.9,
    .battery=25.0,
    .voltage=0.0,
    .orientation={.x=0.0,.y=0.0,.z=0.0}};

    data.voltage=6.0+2.4*data.battery/100.0;
    SatelliteMode mode=MODE_BOOT;

    FaultStatus faults;

    TelemetryPacket packet;

 
   
    int satellite_socket=socket(AF_INET,SOCK_STREAM,0);
    if(satellite_socket<0){
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in ground_station_address = {
    .sin_family=AF_INET,
    .sin_port=htons(PORT),
    .sin_addr.s_addr=htonl(INADDR_LOOPBACK)};

    printf("Connecting to ground station...\n");

    if(connect(satellite_socket,(struct sockaddr *)&ground_station_address,sizeof(ground_station_address))<0){
        perror("connect");
        close(satellite_socket);
        return EXIT_FAILURE;
    }

    printf("Connected to the ground station.\n\n");

    //initializing variables
    int telemetry_enabled=0;
    SatelliteMode requested_mode=MODE_IDLE;
    int sequence_number=0;
    SatelliteMode previous_mode=mode;



    while(1){
        
        char command_buffer[32];
        ssize_t bytes_received=recv(satellite_socket,command_buffer,sizeof(command_buffer)-1,MSG_DONTWAIT);
        if(bytes_received > 0){
            command_buffer[bytes_received]='\0';
            printf("Command received: %s\n", command_buffer);
            handle_command(command_buffer,&telemetry_enabled,&requested_mode,satellite_socket);
        }

        else if(bytes_received == 0) {
            printf("Ground station disconnected.\n");
            break;
        }
        //ignore EAGAIN because they only mean no data is available yet
        else if(errno!=EAGAIN){ 
            perror("recv");
            break;
        }

        if(telemetry_enabled){
            update_sensors(&data,mode);
            faults=detect_faults(&data);
            if(mode==MODE_SAFE && data.battery<25.0){
                faults.low_battery=1; //to override Active mode til battery reaches 25%
            }

            if(mode==MODE_SAFE && data.temperature>55.0){
                faults.over_temperature=1; //to override Active mode til temperature reaches below 55
            }

            
            mode=check_system(faults,requested_mode);

            if(mode!=previous_mode){
                printf("MODE CHANGED : %s -> %s\n",mode_to_string(previous_mode),mode_to_string(mode));
                previous_mode=mode;
            }


            packet=build_packet(sequence_number,&data,mode,faults);
            int length=serialize_packet(&packet,message,sizeof(message));

            if (length<0) {
                printf("Serialization error\n");
            }
            else if ((size_t)length>=sizeof(message)) {
            printf("Buffer too small\n");
            }
            else {
            printf("Serialized packet:\n%s\n", message);

            printf("starting serialized packet transmission :\n");
            ssize_t bytes_sent=send(satellite_socket,message,strlen(message),0);

            
            if (bytes_sent < 0) {
                perror("send packets");
                break;
            }
            printf("Packet %u sent: %s\n\n", packet.sequence_number, message);
            sequence_number++;
            }

            if(mode==MODE_ACTIVE){
                sleep(1);
            }
            else if(mode==MODE_LOW_POWER){
                sleep(2);
            }
            else if(mode==MODE_SAFE){
                sleep(3);
            }
            else if(mode==MODE_IDLE){
                sleep(4);
            }
        }

        else{
            sleep(1);
        }

        
    
    }

    close(satellite_socket);

    return 0;
}