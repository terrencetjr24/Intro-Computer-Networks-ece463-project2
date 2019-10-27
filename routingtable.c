#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;


////////////////////////////////////////////////////////////////
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
  //Initializing the routing table with thte info from InitRespons
  //myID is recieved from the command line

  //ASSUMPTION: That I'll load the single router's table into this single data structure
  //and that the other routers will be running on different ports
  int i;
  numRoutes = 0;
  for(i = 0; i< InitResponse->no_nbr; i++) {
    routingTable[i].dest_id = InitResponse->nbrcost[i].nbr;
    routingTable[i].cost = InitResponse->nbrcost[i].cost;
    routingTable[i].next_hop = InitResponse->nbrcost[i].nbr; //Might not need this
    numRoutes++;
  }
  routingTable[i].dest_id = myID;
  routingTable[i].cost = 0;
  routingTable[i].next_hop = myID; //Might not need this either

  numRoutes++;  //Not sure if the router pointing back to itself is a route
	return;
}


////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
  int i;
  int q;
  //Comparing the id's to see if theres a shorter distance
  for(i = 0; i<RecvdUpdatePacket->no_routes, i++) {
    for(q = 0; q <no_routes; q++) {

    }
  }
  
	return 0;
}


////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
  UpdatePacketToSend->sender_id = myID;
  //POTENTIAL ERROR
  //How wil I send this to every neighbor (potential threading oppurtunity)
  UpdatePacketToSend->dest_id = routingTable[0].dest_id;

  UpdatePacketToSend->no_routes = numRoutes;
  UpdatePacketToSend->route = routingTable;
	return;
}


////////////////////////////////////////////////////////////////
//It is highly recommended that you do not change this function!
void PrintRoutes (FILE* Logfile, int myID){
	/* ----- PRINT ALL ROUTES TO LOG FILE ----- */
	int i;
	int j;
	for(i = 0; i < NumRoutes; i++){
		fprintf(Logfile, "<R%d -> R%d> Path: R%d", myID, routingTable[i].dest_id, myID);

		/* ----- PRINT PATH VECTOR ----- */
		for(j = 1; j < routingTable[i].path_len; j++){
			fprintf(Logfile, " -> R%d", routingTable[i].path[j]);	
		}
		fprintf(Logfile, ", Cost: %d\n", routingTable[i].cost);
	}
	fprintf(Logfile, "\n");
	fflush(Logfile);
}


////////////////////////////////////////////////////////////////
void UninstallRoutesOnNbrDeath(int DeadNbr){
	/* ----- YOUR CODE HERE ----- */
	return;
}
