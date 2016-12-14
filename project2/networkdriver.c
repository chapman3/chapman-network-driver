
/*
 * Author:    David Chapman
 * DuckID:    chapman3
 * Term:      Spring 2016
 *
 * Implemented driver that tests a small but complex software system
 * using PThreads to provide concurrency in C.
 *
 * Implemented in conjunction with design assistance from GTF, and online
 * pthread tutorials. See report.txt for more in depth information.
 */


#include <pthread.h>
#include "packetdescriptor.h"
#include "destination.h"
#include "pid.h"
#include "freepacketdescriptorstore__full.h"
#include "BoundedBuffer.h"
#include "networkdevice.h"
#include "networkdriver.h"
#include "packetdescriptorcreator.h"
#include <stdio.h>
#include <stdlib.h>

/*need bbin, bbout, networkdevice, fpds
 *buffers are named in regard to applications, out sends to apps,
 *in receives.
 */
BoundedBuffer *bbin;
BoundedBuffer *bbout[MAX_PID+1];
NetworkDevice *networkdevice;
FreePacketDescriptorStore *fpds;

/*global reference to pthread_t vars*/
pthread_t incoming_tid;
pthread_t outgoing_tid;

/*incoming thread function for use with pthread_create
 *networkdevice -> networkdriver
 */
void *incoming_thread(){
	while(1){	
		/*declare packet descriptor*/
		PacketDescriptor *pd;
		/*must use blocking, can't proceed until a packet is received*/
		nonblocking_get_pd(fpds, &pd);
		/*flush pd*/
		init_packet_descriptor(pd);
		/*register pd with networkdevice*/
		register_receiving_packetdescriptor(networkdevice, pd);
		/*await incoming packet to proceed*/
		await_incoming_packet(networkdevice);
	}	
}

/*outgoing thread function for use with pthread_create
 *networkdriver -> networkdevice
 */
void *outgoing_thread(){
	int j;
	while(1){
		/*declare pd*/
		PacketDescriptor *pd;
		/*get pd from bbin, blocking because can't proceed without*/
		pd = blockingReadBB(bbin);
		/*try to send 5 times*/
		for(j=0; j<5; j++){
			/*break if successfully sent*/
			if(send_packet(networkdevice, pd)==1){
				break;
			}
		}
		/*non blocking put pd so thread can move on quickly*/
		nonblocking_put_pd(fpds, pd);
	}	
}

void blocking_send_packet(PacketDescriptor *pd){
	/*write packet to networkdevice*/
	blockingWriteBB(bbin, pd);
}
int  nonblocking_send_packet(PacketDescriptor *pd){
	/*write packet to networkdevice, return status*/
	return(nonblockingWriteBB(bbin, pd));
}
/* These calls hand in a PacketDescriptor for dispatching */
/* The nonblocking call must return promptly, indicating whether or */
/* not the indicated packet has been accepted by your code          */
/* (it might not be if your internal buffer is full) 1=OK, 0=not OK */
/* The blocking call will usually return promptly, but there may be */
/* a delay while it waits for space in your buffers.                */
/* Neither call should delay until the packet is actually sent      */

void blocking_get_packet(PacketDescriptor **pd, PID pid){
	/*read packet from network*/
	*pd = blockingReadBB(bbout[pid]);
}
int  nonblocking_get_packet(PacketDescriptor **pd, PID pid){
	/*read packet from network, return status*/
	return(nonblockingReadBB(bbout[pid], (void**)pd));
}
/* These represent requests for packets by the application threads */
/* The nonblocking call must return promptly, with the result 1 if */
/* a packet was found (and the first argument set accordingly) or  */
/* 0 if no packet was waiting.                                     */
/* The blocking call only returns when a packet has been received  */
/* for the indicated process, and the first arg points at it.      */
/* Both calls indicate their process number and should only be     */
/* given appropriate packets. You may use a small bounded buffer   */
/* to hold packets that haven't yet been collected by a process,   */
/* but are also allowed to discard extra packets if at least one   */
/* is waiting uncollected for the same PID. i.e. applications must */
/* collect their packets reasonably promptly, or risk packet loss. */

void init_network_driver(NetworkDevice               *nd, 
                         void                        *mem_start, 
                         unsigned long               mem_length,
                         FreePacketDescriptorStore **fpds_ptr){
	/*init vars*/
	int i;
	networkdevice = nd;

	/*setup bounded buffers*/
	bbin = createBB(8);
	for(i = 0; i<MAX_PID+1; i++){
		bbout[i] = createBB(3);
	}

	/*init free packet descriptor store*/
	*fpds_ptr = create_fpds();
	fpds = *fpds_ptr;
	/*if create returns 0, was unsuccessful, lemme know!*/
	if(create_free_packet_descriptors(fpds, mem_start, mem_length) 
		< 1){
		printf("Failed to create any free packet descriptors");
	}
	
	/*create processing threads*/
	pthread_create(&outgoing_tid, NULL, &outgoing_thread, NULL);
	pthread_create(&incoming_tid, NULL, &incoming_thread, NULL);

}
/* Called before any other methods, to allow you to initialise */
/* data structures and start any internal threads.             */ 
/* Arguments:                                                  */
/*   nd: the NetworkDevice that you must drive,                */
/*   mem_start, mem_length: some memory for PacketDescriptors  */
/*   fpds_ptr: You hand back a FreePacketDescriptorStore into  */
/*             which you have put the divided up memory        */
/* Hint: just divide the memory up into pieces of the right size */
/*       passing in pointers to each of them                     */ 

