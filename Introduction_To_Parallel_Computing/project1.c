/*
 *Cpts411 Parallel Computing
 *Jin Tao 11474660
 *Assignment 1:
 *The goal of this project is to empirically estimate the network parameters (latency and bandwidth constants)
 *for the network connecting the nodes of the compute cluster. To derive this estimate write a simple MPI send receive program involving only two processors (one sends and the other receives). Each MPI_Send should send a message of size m bytes to the other processor. By increasing the message size m from 1, 2, 4, 8, ... and so on, you are expected to plot a runtime curve with runtime for each send-recv communication on the Y-axis and message size (m) on the X-axis. For the message size, you may have to go on up to 1MB or so to observe a meaningful trend. Make sure you double m for each step.
 *From the curve derive the values for latency and bandwidth. To make sure the observed numbers are as precise and reliable as they can be, be sure to take an average over multiple iterations (at least 10) before reporting.
 *Rank 1 sends to rank 0 (all other ranks sit idle)
 */
#include <string.h>
#include <stdio.h>
#include <mpi.h> 
#include <assert.h>
#include <sys/time.h>

#define MAX 1048576 //message size(byte) range from 1,2,4,8...1mb

int main(int argc,char *argv[])
{

   int rank,p;
   struct timeval t1,t2;

   MPI_Init(&argc,&argv);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   MPI_Comm_size(MPI_COMM_WORLD,&p);
   assert(p>=2);
   int range=0;
   int NumOfIteration=10;

   int dest=0;
   MPI_Status status;
   
   for(range=1;range<=MAX;range=range*2) {
       //To make sure the observed numbers are as precise and reliable as they can be, taking average only over the reasonable values
       int iteration=0;
     
       char send_buffer[range];//'\0' will also get sent so the number of * will be 0, 1,3....,1023
       char recv_buffer[range];
       if(rank==1) {
           //fill in the send_buffer
           int index=0;
           for(index=0;index<range-1;index++) {
               send_buffer[index]='*';//1 char=1 byte, change the number of *'s for different sizes
           }
           send_buffer[index] = '\0';
       }
       
       for(iteration=0;iteration < NumOfIteration;iteration++) {
           MPI_Barrier(MPI_COMM_WORLD);
           //mpi_barrier recommended by the professor 
           if(rank==1) {
               dest = 0;
               gettimeofday(&t1,NULL);
               MPI_Send(send_buffer,range,MPI_CHAR,dest,0,MPI_COMM_WORLD);
               gettimeofday(&t2,NULL);
               long tSend = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);//unit=micro-second
           }
           else if (rank==0) {
                gettimeofday(&t1,NULL);
                MPI_Recv(recv_buffer,range,MPI_CHAR,1,0,MPI_COMM_WORLD,&status);
                gettimeofday(&t2,NULL);
                long tRecv = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);
                //printf("^^^^^^^^^range=%d bytes, iteration=%d, tRecv=%ld\n",range,iteration,tRecv);
                printf("%d,%d,%ld\n",range,iteration, tRecv);
           }
       }
/*
       if(rank==1) {
           printf("1:The current range is %d, namely, the current size=%d bytes\n", range,range);
           printf("1:Rank=%d: sent message to rank %d\n", rank,dest);
           printf("1:Below is the message content:%s\n\n",send_buffer);
       }
       else if(rank==0) {
           printf("0:The current range is %d, namely, the current size=%d bytes\n",range, range);
           printf("0:Rank=%d: received message from rank %d\n",rank,status.MPI_SOURCE);
           printf("0:Below is the message content:%s\n\n",recv_buffer);
       }
*/
   }
   MPI_Finalize();
}
