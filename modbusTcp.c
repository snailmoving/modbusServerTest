//
// Created by work on 19-5-10.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "modbus/modbus.h"
//#include "modbus/modbus-tcp.h"

modbus_t *ctx;
modbus_mapping_t *mb_mapping;
fd_set refset;
fd_set rdset;
int fdmax;
int server_socket = -1;
int master_socket;

#define NB_CONNECTIONS  INT_MAX

#define BITS_ADDRESS    1000
#define INPUT_ADDRESS   2000
#define HOLDING_REGISTERS_ADDRESS   3000
#define INPUT_REGISTERS_ADDRESS     4000

#define BITS_NB         65536
#define INPUT_NB        65536
#define HOLDING_REGISTERS_NB    65536
#define INPUT_REGISTERS_NB      65536

static void close_sigint(int dummy)
{
    if (server_socket != -1) {
        close(server_socket);
    }
    modbus_free(ctx);
    modbus_mapping_free(mb_mapping);

    exit(dummy);
}

int setDevice()
{
    int rc;

    ctx = modbus_new_tcp("10.89.13.207",1502);
    if(ctx == NULL)
    {
        fprintf(stderr,"unable to create the libmodbus context\n");
        return -1;
    }

    mb_mapping = modbus_mapping_new(BITS_NB,INPUT_NB,HOLDING_REGISTERS_NB,INPUT_REGISTERS_NB);
    if(mb_mapping==NULL){
        fprintf(stderr,"Failed to allocate the mapping: %s\n",modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }
    server_socket = modbus_tcp_listen(ctx,NB_CONNECTIONS);
    if(server_socket==-1){
        fprintf(stderr,"unable to listen TCP connection\n");
        modbus_free(ctx);
        return -1;
    }
    signal(SIGINT,close_sigint);

    FD_ZERO(&refset);
    FD_ZERO(&rdset);

    FD_SET(server_socket,&refset);

    fdmax = server_socket;
    return 0;
}

int closeDevice()
{
    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);
    ctx = NULL;
    return 0;
}


int modbusCycle()
{
    int rc;
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

    for(;;){
        rdset = refset;
        if(select(fdmax+1,&rdset,NULL,NULL,NULL)==-1){
            perror("Server select() failure.");
            close_sigint(1);
        }

        for(master_socket=0;master_socket<=fdmax;master_socket++){
            if(!FD_ISSET(master_socket,&rdset)){
                continue;
            }
            if(master_socket == server_socket){
                socklen_t addrlen;
                struct sockaddr_in clientaddr;
                int newfd;

                addrlen = sizeof(clientaddr);
                memset(&clientaddr,0, sizeof(clientaddr));
                newfd = accept(server_socket,(struct sockaddr *)&clientaddr,&addrlen);
                if(newfd == -1){
                    perror("Server accept() error");
                }else{
                    FD_SET(newfd,&refset);

                    if(newfd > fdmax){
                        fdmax = newfd;
                    }
                    printf("New connection from %s:%d on socket %d\n",inet_ntoa(clientaddr.sin_addr),clientaddr.sin_port,newfd);
                }
            }else{
                modbus_set_socket(ctx,master_socket);
                rc = modbus_receive(ctx,query);
                if(rc>0){
                    modbus_reply(ctx,query,rc,mb_mapping);
                } else if(rc == -1){
                    printf("Connection closed on socket %d\n",master_socket);
                    close(master_socket);
                    FD_CLR(master_socket,&refset);
                    if(master_socket==fdmax){
                        fdmax--;
                    }
                }
            }
        }
    }
}