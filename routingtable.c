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
  routingTable[i].next_hop = myID; //ASSUMPTION: that this isn't an issue
  routingTable[i].path_len = 1;
  routingTable[i].path[0] = myID;
  routingTable[i].path[1] = myID;
  NumRoutes = 1; 
  
  for(i = 1; i<= InitResponse->no_nbr; i++) {
    routingTable[i].dest_id = InitResponse->nbrcost[i-1].nbr;
    routingTable[i].cost = InitResponse->nbrcost[i-1].cost;
    routingTable[i].next_hop = InitResponse->nbrcost[i-1].nbr;
    NumRoutes++;

    routingTable[i].path_len = 2;
    routingTable[i].path[0] = myID;
    routingTable[i].path[1] = InitResponse->nbrcost[i-1].nbr;

    }
  return;
}


////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
  int i;
  int q;
  int z;
  int totalDistance;
  int changed = 0;
  int currentRouterIsIntermediate;
  //Making sure that the packet was sent to the proper neighbor
  if(RecvdUpdatePacket->dest_id == myID){

    //Finding shortest distance
    for(i = 0; i<RecvdUpdatePacket->no_routes; i++) {
      totalDistance = costToNbr;
      totalDistance += RecvdUpdatePacket->route[i].cost;
      currentRouterIsIntermediate = 0;
      //Finding the destination id (in current router's table) corresponding to the neighbors destination that we're looking at
      q = 0;
      while((routingTable[q].dest_id != RecvdUpdatePacket->route[i].dest_id) && (q < NumRoutes))
	q++;
      //Adding a new router, if it's not already in the table
      if(q == NumRoutes){
	routingTable[NumRoutes].dest_id = RecvdUpdatePacket->route[i].dest_id;
	routingTable[NumRoutes].next_hop = RecvdUpdatePacket->sender_id; //Changed this 11/10
	routingTable[NumRoutes].cost = totalDistance;
	routingTable[NumRoutes].path_len = RecvdUpdatePacket->route[i].path_len + 1;
	//Ensuring that the first step on a path is the current router
	  routingTable[q].path[0] = myID;
	for(z=0; z < routingTable[NumRoutes].path_len; z++)
	  routingTable[NumRoutes].path[z+1] = RecvdUpdatePacket->route[i].path[z];
	//Incrementing the number of routes since we added one
	NumRoutes++;
	//Added a route, so it obviously changed
	changed = 1;
      }      
      //Checking if the next hop to the destination is the one sending the packet, or if the cost to get to that place is less than it was before (and the current router is not the next hop to get there)
      else if( (routingTable[q].next_hop == RecvdUpdatePacket->sender_id) | (totalDistance < routingTable[q].cost) ){
	//Reading through the route of to the destination to make sure that the current router isn't a step along the way (myID != any_of_the_next_hops)
	for(z=0; z<RecvdUpdatePacket->route[i].path_len; z++){
	  if(RecvdUpdatePacket->route[i].path[z] == myID)
	    currentRouterIsIntermediate = 1;
	}
	//If the current router IS NOT a next_hop to the destination do the update
	if(!currentRouterIsIntermediate){
	  //Updating the cost, but first checking if it's changed (and if it's less than infinity, otherwise I don't care about this sort of change)
	  if((routingTable[q].cost != totalDistance) && (totalDistance < INFINITY))
	    changed = 1;
	  
	  //checking if total Distance is greater than INFINITY, so that I can keep the max cost to INFINITY
	  if(totalDistance <= INFINITY)
	    routingTable[q].cost = totalDistance;
	  else
	    routingTable[q].cost = INFINITY;
	  
	  //Checking if it's changed before I change it
	  if(routingTable[q].path_len != (RecvdUpdatePacket->route[i].path_len + 1))
	    changed = 1;
	  routingTable[q].path_len = RecvdUpdatePacket->route[i].path_len + 1;
	  //Ensuring that the first step on a path is the current router
	  routingTable[q].path[0] = myID;
	  //Updating the route if it needs to be updated (path includes source node)
	  for(z=0; z<RecvdUpdatePacket->route[i].path_len; z++){
	    //checking if things are changing before changing them
	    if(routingTable[q].path[z+1] != RecvdUpdatePacket->route[i].path[z])
	      changed = 1;
	    routingTable[q].path[z+1] = RecvdUpdatePacket->route[i].path[z];
	  }
	
	  if(routingTable[q].next_hop != routingTable[q].path[1])
	    changed = 1;
	  routingTable[q].next_hop =  routingTable[q].path[1];
	}
      }
    }
  }
  /*
  //Setting costs back to INFINITY if they get higher
  for(i=0; i<NumRoutes; i++){
    if(routingTable[i].cost > INFINITY)
      routingTable[i].cost = INFINITY;
  }
  */
  return changed; //A zero indicates that nothing updated
}


////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
  UpdatePacketToSend->sender_id = myID;
  
  //POTENTIAL ERROR***
  //This doesn't need to be filled until it's updated by the polling function
  //UpdatePacketToSend->dest_id = routingTable[1].dest_id;

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

  int i;
  
  for(i=0; i<NumRoutes; i++){
    if(routingTable[i].next_hop == DeadNbr)
      routingTable[i].cost = INFINITY;
  }
  return;
}
