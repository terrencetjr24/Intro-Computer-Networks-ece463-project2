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
#include <pthread.h>
#include <time.h>
//Need a header fle for threads

int initializeNE(struct sockaddr_in *, int);
int initializeRouter(struct sockaddr_in*, int);
void* polling(void*);
void* timing(void*);

//Global Variables (in addition to the two in routingtable.c)
int nefd, routerfd;
struct sockaddr_in neAddr, routerAddr;
pthread_mutex_t lock;
int routerID;
struct pkt_INIT_RESPONSE initialResp;

clock_t initialStart;
clock_t lastTableUpdate;
clock_t lastRouteUpdate[MAX_ROUTERS];
clock_t lastUpdateSent;
int converged;

int main(int argc, const char*argv[]) {
  int nePort;
  int routerPort;
  int i;
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
  if (bind(routerfd, (const struct sockaddr*) &routerAddr, sizeof(routerAddr)) < 0){    printf("error binding the router fd\n");    return EXIT_FAILURE;  }
  //Checking file descriptors
  if((nefd == -1) | (routerfd == -1))
    return EXIT_FAILURE;
  //Sending the initial request and recieving the initial response
  struct pkt_INIT_REQUEST send;
  send.router_id = htonl(routerID);
  sendto(nefd, (struct pkt_INIT_REQUEST*)&send, sizeof(send), MSG_CONFIRM, (const struct sockaddr *) &neAddr, (socklen_t *)sizeof(neAddr));
  //************//
  //After turning the router off and turning it back on, I get stuck right below this line, FIX THIS LATER
  //Might need to change the flag, currently its MSG_WAITALL
  //if I set it non-blocking it will continuously return a -1
  recvfrom(nefd, (struct pkt_INIT_RESPONSE*)&initialResp, sizeof(initialResp), MSG_WAITALL, (const struct sockaddr *) &neAddr, (socklen_t *)sizeof(neAddr));
  //***********//
  //"Starting time" of router (and the first time it's sent an "update"
  initialStart = clock();
  lastUpdateSent = clock();
  //Converting the response to host endian format
  ntoh_pkt_INIT_RESPONSE(&initialResp);
  //Inserting info into the routers table (so updating the table, and because of this update obviously the table hasn't converged)
  InitRoutingTbl(&initialResp, routerID);
  lastTableUpdate = initialStart;
  converged = 0;
  //Saying that each neighbor sent an update on initialization (to give each init value)
  for(i = 0; i < initialResp.no_nbr; i++)
    lastRouteUpdate[i] = initialStart; 
  //Initial router#.log write, all the rest will be appends (so that I don't overwrite data)
  char file[16];
  sprintf(file, "router%d.log", routerID);
  FILE *fp = fopen(file, "w");
  PrintRoutes(fp, routerID);
  fclose(fp);
 
  //starting the thread nonsense  
  pthread_t polling_thread_id;
  pthread_t timing_thread_id;
  //Initializing the lock
  pthread_mutex_init(&lock, NULL);
  //Creating the polling and timing threads
  if(pthread_create(&polling_thread_id, NULL, polling, NULL)){    perror("Error creating thread for polling:\n");    return EXIT_FAILURE;  }
  if(pthread_create(&timing_thread_id, NULL, timing, NULL)){perror("Error creating thread for timing:\n");return EXIT_FAILURE; }
  //Joining the threads after "completion" (might be able to manipulate this to get turning the router off and then on to work)
  pthread_join(polling_thread_id, NULL);
  pthread_join(timing_thread_id, NULL);

  return EXIT_SUCCESS;
}

void * polling(void* arg){
  int q;
  struct pkt_RT_UPDATE recieved;
  char file[16];
  FILE* fp;
  //Continuously looping through this process
  while(1){
    //Waiting until I recieve a packet from the NE (the MSG_WAITALL flag does the waiting)
    recvfrom(nefd, (struct pkt_RT_UPDATE*)&recieved, sizeof(recieved), MSG_WAITALL, (const struct sockaddr *) &neAddr, (socklen_t *)sizeof(neAddr));
    //Converting the packet from network to host format
    ntoh_pkt_RT_UPDATE(&recieved);

    //Incrementing through my routing table to find the neighbor that sent the UPDATE, and updating the last time they sent an update
    q=0;
    while(initialResp.nbrcost[q].nbr != recieved.sender_id)
      q++;

    pthread_mutex_lock(&lock);
    lastRouteUpdate[q] = clock();
    pthread_mutex_unlock(&lock);
    
    //if the routes are actually updated I will update the time of the most recent update and update the log file
    if(UpdateRoutes(&recieved, initialResp.nbrcost[q].cost, routerID)){
      converged = 0;
      pthread_mutex_lock(&lock);
      lastTableUpdate = clock();
      pthread_mutex_unlock(&lock);
      sprintf(file, "router%d.log", routerID);
      fp = fopen(file, "a");
      PrintRoutes(fp, routerID);
      fclose(fp);
      pthread_mutex_unlock(&lock);
    }
  }
}

void * timing(void* arg){
  //Need to be checking for:
  //UPDATE INTERVAL, for when to re send my router's packet to all neighbors
  //CONVERGE TIMEOUT, to see when my router converges onto the proper routes
  //FAILURE DETECTION, to see if anotehr router has timed out, an need to be "unistalled" 
  clock_t dummy;
  struct pkt_RT_UPDATE sending;
  char file[16];
  int i;
  //Looping through the whole process
  while(1){
    pthread_mutex_lock(&lock);
    dummy = clock();
    pthread_mutex_unlock(&lock);
    //After it's been "UPDATE_INTERVAL" seconds, send another update to each neighbor 
    if( ( ( (float)(dummy-lastUpdateSent) )/CLOCKS_PER_SEC) >= UPDATE_INTERVAL){
      for(i=0; i<initialResp.no_nbr; i++){
	printf("R%d Sending packet to R%d\n",routerID, initialResp.nbrcost[i].nbr);
	ConvertTabletoPkt(&sending, routerID);
	sending.dest_id = initialResp.nbrcost[i].nbr;
	hton_pkt_RT_UPDATE (&sending);
	sendto(nefd, (struct pkt_RT_UPDATE*)&sending, sizeof(sending), MSG_CONFIRM, (const struct sockaddr *) &neAddr, (socklen_t)sizeof(neAddr));
      }
      lastUpdateSent = clock();
    }
    pthread_mutex_lock(&lock);
    dummy = clock();
    pthread_mutex_unlock(&lock);
    //After it's been "CONVERGE_TIMEOUT" seconds, check if the table has been updated. If not, set converged to TRUE and update log file
    if( ( ( (float)(dummy-lastTableUpdate) )/CLOCKS_PER_SEC) >= CONVERGE_TIMEOUT){    
      //If the table wasn't already converged (don't want to continously update the table if it's been converged)
      if(!converged){
	//Updating routing log and "converged" variable
	pthread_mutex_lock(&lock);
	converged = 1;
	sprintf(file, "router%d.log", (int)routerID);
	FILE* fp = fopen(file, "a");
	fprintf(fp, "%d: Converged\n", (int)((dummy-initialStart)/CLOCKS_PER_SEC) );
	fclose(fp);
	pthread_mutex_unlock(&lock);
      }
    }
    pthread_mutex_lock(&lock);
    dummy = clock();
    pthread_mutex_unlock(&lock);
    //Iterating through each neighbor to make sure that it's sent an UPDATE PACKET
    for(i = 0; i<initialResp.no_nbr; i++){
      //if it's been more than "FAILURE_DETECTION" seconds
      if(( ( (float)(dummy-lastRouteUpdate[i]) )/CLOCKS_PER_SEC) >= FAILURE_DETECTION){
	//calling the function to say that this node is "dead"
	UninstallRoutesOnNbrDeath(initialResp.nbrcost[i].nbr);
      }
    }
  }
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
