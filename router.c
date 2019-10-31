#include "ne.h"
#include "router.h"
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <inttypes.h>

//Need a header fle for threads

int initializeNE(struct sockaddr_in *, int);
int initializeRouter(struct sockaddr_in*, int);

int main(int argc, const char*argv[]) {
  
  int nefd, routerfd;
  int routerID;
  int nePort;
  int routerPort;
  struct sockaddr_in neAddr, routerAddr;
  //Checking the arguments
  if(argc != 5){printf("./router <router #> <host> <network port> <router port>\n"); return EXIT_FAILURE;}
  //Assigning the user inptus to relevant variables
  routerID = atoi((char*)argv[1]);
  nePort =  atoi((char*)argv[3]);
  routerPort =  atoi((char*)argv[4]);
  //Initializing the router and NE info (including binding the router fd) 
  nefd = initializeNE(&neAddr, nePort);
  routerfd = initializeRouter(&routerAddr, routerPort);
  //Binding router fd
  if (bind(routerfd, (const struct sockaddr*) &routerAddr, sizeof(routerAddr)) < 0){
    printf("error binding the router fd\n");
    return EXIT_FAILURE;
  }
  //Checking file descriptors
  if((nefd == -1) | (routerfd == -1))
    return EXIT_FAILURE;
  //Sending the initial request and recieving the initial response
  struct pkt_INIT_RESPONSE buf;
  struct pkt_INIT_REQUEST send;
  send.router_id = htonl(routerID);

  sendto(nefd, (struct pkt_INIT_REQUEST*)&send, sizeof(send), MSG_CONFIRM, (const struct sockaddr *) &neAddr, (socklen_t *)sizeof(neAddr));
  recvfrom(nefd, (struct pkt_INIT_RESPONSE*)&buf, sizeof(buf), MSG_WAITALL, (const struct sockaddr *) &neAddr, (socklen_t *)sizeof(neAddr));
  //Converting the respons to host endian format
  ntoh_pkt_INIT_RESPONSE(&buf);
  
  //Checking that I recieved the response correctly (delete after I'm done checking)
  printf("The number of neighbors: %u\n", buf.no_nbr);
  int i;
  for(i = 0; i< buf.no_nbr; i++)
    printf("The %d neighbor ID and cost to it: %d - %d\n", i, buf.nbrcost[i].nbr, buf.nbrcost[i].cost);
    //starting the thread nonsense
     
  return EXIT_SUCCESS;
}

int initializeNE(struct sockaddr_in * addr, int port){
  int fd;
  //Creating socket file descriptor for network emulator
  if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        printf("Network Emulator socket creation failed\n"); 
        return -1;
    }
  memset(addr, 0, sizeof(*addr));
  addr->sin_family = AF_INET;
  addr->sin_port = htons((unsigned short)port);
  addr->sin_addr.s_addr = htonl(INADDR_ANY);

  return fd;
}


int initializeRouter(struct sockaddr_in * addr, int port){
  int fd;
  //Creating socket file descriptor for router
  if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        printf("Router socket creation failed\n"); 
        return -1; 
    }
  memset(addr, 0, sizeof(*addr));
  addr->sin_family = AF_INET;
  addr->sin_port = htons((unsigned short)port);
  addr->sin_addr.s_addr = htonl(INADDR_ANY);

  return fd;
}
