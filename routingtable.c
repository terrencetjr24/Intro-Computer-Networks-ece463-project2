#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;


////////////////////////////////////////////////////////////////
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
  int i=0;
  
  //Self node decleration
  routingTable[i].dest_id = myID;
  routingTable[i].cost = 0;
  routingTable[i].next_hop = 0; //ASSUMPTION: no router has an ID of zero
  routingTable[i].path_len = 1;
  routingTable[i].path[0] = myID;
  routingTable[i].path[1] = myID;
  NumRoutes = 1; 
  
  for(i = 1; i<= InitResponse->no_nbr; i++) {
    routingTable[i].dest_id = InitResponse->nbrcost[i-1].nbr;
    routingTable[i].cost = InitResponse->nbrcost[i-1].cost;
    routingTable[i].next_hop = InitResponse->nbrcost[i-1].nbr;
    NumRoutes++;
    //
    //ifdef thing inside the structure
    routingTable[i].path_len = 2;
    routingTable[i].path[0] = myID;
    routingTable[i].path[1] = InitResponse->nbrcost[i-1].nbr;
    //
    }
  return;
}


////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
  int i;
  int q;
  int z;
  int totalDistance;
  
  //Making sure that the packet was sent to the proper neighbor
  if(RecvdUpdatePacket->dest_id == myID){

    //Finding shortest distance
    for(i = 0; i<RecvdUpdatePacket->no_routes; i++) {
      totalDistance = costToNbr;
      totalDistance += RecvdUpdatePacket->route[i].cost;
      q = 0;
      
      //Finding the destination id (in current router's table) corresponding to the neighbors destination that we're looking at
      while((routingTable[q].dest_id != RecvdUpdatePacket->route[i].dest_id) && (q < NumRoutes))
	q++;

      //Adding a new router
      if(q == NumRoutes){
	routingTable[NumRoutes].dest_id = RecvdUpdatePacket->route[i].dest_id;
	routingTable[NumRoutes].next_hop = RecvdUpdatePacket->route[i].next_hop;

	q = 0;
	while(routingTable[q].dest_id != RecvdUpdatePacket->route[i].next_hop)
	  q++;
	
	routingTable[NumRoutes].cost = RecvdUpdatePacket->route[i].cost + routingTable[q].cost;
	routingTable[NumRoutes].path_len = RecvdUpdatePacket->route[i].path_len + 1;
	routingTable[NumRoutes].path[0] = myID;
	for(z=1; z < routingTable[NumRoutes].path_len; z++)
	  routingTable[NumRoutes].path[z] = RecvdUpdatePacket->route[i].path[z-1];
	
	  NumRoutes++;
      }

      //Checking if the next hop to the destination is the one sending the packet
      //or if the cost to get to that place is less than it was before
      //(and the current router is not the next hop to get there)
      else if( (routingTable[q].next_hop == RecvdUpdatePacket->sender_id) | ((totalDistance < routingTable[q].cost) & (myID != RecvdUpdatePacket->route[i].next_hop)) ){
	//Updating the cost
	routingTable[q].cost = totalDistance;
	//
	//ifdef stuff
	routingTable[q].path_len = RecvdUpdatePacket->route[i].path_len + 1;
	//Updating the route if it needs to be updated (path includes source node)
	for(z=1; z<RecvdUpdatePacket->route[i].path_len; z++)
	  routingTable[q].path[z+1] = RecvdUpdatePacket->route[i].path[z];
	//
      }
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
  UpdatePacketToSend->sender_id = myID;
  
  //POTENTIAL ERROR***
  //How wil I send this to every neighbor (potential threading/forking oppurtunity)
  UpdatePacketToSend->dest_id = routingTable[1].dest_id;

  UpdatePacketToSend->no_routes = NumRoutes;
  //Copying the routing table over to send
  int i;
  int q;
  for(i=0; i< NumRoutes;i++){
    UpdatePacketToSend->route[i].dest_id = routingTable[i].dest_id;
    UpdatePacketToSend->route[i].next_hop = routingTable[i].next_hop;
    UpdatePacketToSend->route[i].cost = routingTable[i].cost;
    //ifdef stuff
    UpdatePacketToSend->route[i].path_len = routingTable[i].path_len;
    for(q=0; q<routingTable[i].path_len; q++)
      UpdatePacketToSend->route[i].path[q] = routingTable[i].path[q];
  }
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
