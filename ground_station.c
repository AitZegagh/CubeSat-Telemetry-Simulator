#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <string.h>
#include <sys/select.h>

#include "ground_station.h"

#define PORT 5000



int main(void)
{

    int server_socket=socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket<0) {
        perror("socket");
        return 1;
    }
    printf("Ground station socket created successfully.\n");


    struct sockaddr_in server_address={0};
    server_address.sin_family=AF_INET;
    server_address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    server_address.sin_port=htons(PORT);


    if (bind(server_socket,(struct sockaddr *)&server_address,sizeof(server_address))<0) {
        perror("bind");
        close(server_socket);
        return 1;
    }
    printf("Ground station bound to port %d.\n", PORT);



    //socket in listening mode
    if (listen(server_socket,1)<0) {
        perror("listen");
        close(server_socket);
        return 1;
    }
    printf("Ground station listening on port %d...\n", PORT);


    struct sockaddr_in client_address = {0};
    socklen_t client_address_size = sizeof(client_address);
    printf("Waiting for satellite connection...\n");


    //wait until a connection occurs
    int client_socket=accept(server_socket,(struct sockaddr *)&client_address,&client_address_size);
    if(client_socket<0){
        perror("accept");
        close(server_socket);
        return 1;
    }
    printf("Satellite connected successfully.\n\n");

    printf("Available commands: START, STOP, ACTIVE, LOW_POWER, QUIT\n");

    //loop to handle commands and packets and responses received
    while(1){

        fd_set read_fds; //set containing the inputs that the function select monitors
        FD_ZERO(&read_fds); //clears the set
        FD_SET(STDIN_FILENO, &read_fds); //adds keyboard input to the set
        FD_SET(client_socket, &read_fds);//adds the satellite inputs to the set

        int activity=select(client_socket+1,&read_fds,0,0,0);
        if(activity < 0){
            perror("select");
            break;
        }

        //check if keyboard input
        if(FD_ISSET(STDIN_FILENO,&read_fds)){
            char command[16];
            if(fgets(command,sizeof(command),stdin)!=NULL){
                command[strcspn(command,"\n")]='\0'; //removes the "\n" thats auto added by fgets

                if(strcmp(command, "QUIT")==0) {    //mechanism to stop the simulation instead of ctrl+c
                    printf("Closing ground station...\n");
                    break;
                }
                if(strlen(command)>0){
                     ssize_t bytes_sent=send(client_socket,command,strlen(command),0);

                     if(bytes_sent<0){
                        perror("send command");
                        break;
                     }
                     printf("command sent: %s\n",command);
                }
            }    
        }

        //check if sat sent data
        if(FD_ISSET(client_socket,&read_fds)){
            char buffer[256];
            ssize_t bytes_received=recv(client_socket,buffer,sizeof(buffer)-1,0);
            if(bytes_received<0){
                perror("recv");
                break;}
            else if(bytes_received==0){
                printf("satellite disconnected\n");
                break;}
        
                
            buffer[bytes_received]='\0';
            //now we verify if we received a packet or a command response
            if(strchr(buffer,',')!=NULL){
                printf("Received chunk:\n%s\n", buffer);
                ReceivedPacket packet;
                parse_packet(buffer, &packet);
            }

            else{
                printf("Satellite response: %s\n",buffer);
            }
        }
    }
            
    

    close(server_socket);
    close(client_socket);

    return 0;
}