/*************************************************************************************************************
*Jin Tao 11474660 Assignment2
The goal of this programming project is for you to implement, test and compare the shift, ring, and hypercubic permutations and their effectivess in solving the "sum of n numbers" problem.

Let p denote the number of processes.  For this project, we will assume that p is a power of 2. 

You will write three functions in your code:
Sum_byShift()
Sum_byRing()
Sum_byHypercube()

The input to all these three functions is a set of p numbers, and the output expected is the sum of those p numbers that should be available at all processes at the end of the function call. How the sum is computed is what will be different between these functions. Here is the pseudocode:

Step 1. Each proc. generates a single random number in the interval of [0, 99]. You can use your favorite random number generating function of choice. 
Step 2. Each proc. calls the Sum_{X} function, where {X} is one of "byShift" or "byRing" or "byHypercube". 

                Sum_byShift(): This function will be a two-pass code. First, it does a right shift permutation of the values so that the final sum gets accumulated at process rank p-1. This means, the final sum will be available only at rank p-1 at the end of this pass. To get the sum to all the other processes, a second pass (left shift) is necessary.

                Sum_byRing(): This function does a ring permutation of the values so that the final sum gets accummulated at all proceses. It should be farily easy to see how this can be done in just one pass. 

                Sum_byHypercube(): This function does a hypercubic permutation of the values so that the final sum gets accumulated at all proceses. It should be farily easy to see how to do this based on the lecture materials. 

In all your implementations, you are only allowed to use point to point MPI communication primitives (e.g., MPI_Sendrecv, MPI_Send, MPI_Recv, and the nonblocking variants). I will let you pick which ones to use. You are not allowed to use any collective primitives such as Reduce or Allreduce. 

Report: For your report, please experiment with varying values of p from 1 through 64 on the cluster. Record the timings for all the three variants of your Sum_{X} function calls (excluding any print or I/O statements). Tabulate your results and plot the results on a runtime chart. Also, compute and plot the speedup and efficiency achieved.   
****************************************************************************************************************/

#include <stdio.h>
#include <mpi.h> 
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

int send_content;//send and receive one int[0,99]
int recv_content;
MPI_Status status;
int rank,p;

//The input to all these three functions is a set of p numbers, and the output expected is the sum of those p numbers that should be available at all processes at the end of the function call.
//rand()%100  [0,99]
void Sum_byShift(int NumOfProc) {
    
    //Step 1. Each proc. generates a single random number in the interval of [0, 99].
    int current_content=rand()%100;
    int sum=current_content;
    //printf("###Sum_byShift****rank %d init num=%d\n",rank,current_content);
    struct timeval t1,t2;
    gettimeofday(&t1,NULL);
    //First, it does a right shift permutation of the values so that the final sum gets accumulated at process rank p-1. This means, the final sum will be available only at rank p-1 at the end of this pass.
    if(rank==0) {
        MPI_Send(&current_content,1,MPI_INT,rank+1,0,MPI_COMM_WORLD);
    }
    else if(rank==p-1) {
        MPI_Recv(&recv_content,1,MPI_INT,rank-1,0,MPI_COMM_WORLD,&status);
        current_content += recv_content;//add up
    }
    else {
        MPI_Recv(&recv_content,1,MPI_INT,rank-1,0,MPI_COMM_WORLD,&status);
        current_content += recv_content;//add up
        MPI_Send(&current_content,1,MPI_INT,rank+1,0,MPI_COMM_WORLD);
    }
    //To get the sum to all the other processes, a second pass (left shift) is necessary.
    //only rank (p-1) to 1 sends to the previous proc
    //only rank 0-(p-2) receives from the next proc
    if(rank==p-1) {//the last proc only sends its content backward(left shift)
        MPI_Send(&current_content,1,MPI_INT,rank-1,0,MPI_COMM_WORLD);
    }
    else if(rank==0) {
        MPI_Recv(&recv_content,1,MPI_INT,rank+1,0,MPI_COMM_WORLD,&status);
        current_content = recv_content;
    }
    else {
        MPI_Recv(&recv_content,1,MPI_INT,rank+1,0,MPI_COMM_WORLD,&status);
        current_content = recv_content;
        MPI_Send(&current_content,1,MPI_INT,rank-1,0,MPI_COMM_WORLD);
    }
    gettimeofday(&t2,NULL);
    long totalTime = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);//unit=micro-second
    printf("Shift,%ld\n",totalTime);
    //verify the result on each proc, will be commented later to get more accurate time
    //printf("###Sum_byShift****The final result on rank %d = %d\n", rank, current_content);
}

//This function does a ring permutation of the values so that the final sum gets accummulated at all proceses. It should be farily easy to see how this can be done in just one pass.
void Sum_byRing(int NumOfProc) {
    
    //Step 1. Each proc. generates a single random number in the interval of [0, 99].
    int current_content=rand()%100;
    //printf("###Sum_byRing****rank %d init num=%d\n",rank,current_content);
    int sum=current_content;
    //p-1 loops
    int i=0;
    struct timeval t1,t2;
    gettimeofday(&t1,NULL);
    for(i=0;i<NumOfProc-1;i++) {
        MPI_Send(&current_content,1,MPI_INT,(rank+1) % NumOfProc,0,MPI_COMM_WORLD);
        MPI_Recv(&current_content,1,MPI_INT,(rank-1+NumOfProc) % NumOfProc,0,MPI_COMM_WORLD,&status);
        sum += current_content;
    }
    gettimeofday(&t2,NULL);
    long totalTime = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);//unit=micro-second
    printf("Ring,%ld\n",totalTime);
    //verify the result on each proc, will be commented later to get more accurate time
    //printf("###Sum_byRing****The final result on rank %d = %d\n",rank,sum);
}

//This function does a hypercubic permutation of the values so that the final sum gets accumulated at all proceses. It should be farily easy to see how to do this based on the lecture materials.
//Note: log2(x)=log10(x)/log10(2)
void Sum_byHypercube(int NumOfProc) {
    
    //Step 1. Each proc. generates a single random number in the interval of [0, 99].
    int current_content=rand()%100;
    //printf("###Sum_byHypercube****rank %d init num=%d\n",rank,current_content);
    
    int d=0;
    int range=log(NumOfProc)/log(2);
    struct timeval t1,t2;
    gettimeofday(&t1,NULL);
    for(d=0;d<range;d++) {
            int mate=rank^((int)pow(2.0,(double)d));
            MPI_Sendrecv(&current_content,1,MPI_INT,mate,0,&recv_content,1,MPI_INT,mate,0,MPI_COMM_WORLD,&status);
            current_content += recv_content;
    }
    gettimeofday(&t2,NULL);
    long totalTime = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);//unit=micro-second
    printf("Hypercube,%ld\n",totalTime);
    //verify the result on each proc, will be commented later to get more accurate time
    //printf("###Sum_byHypercube****The final result on rank %d = %d\n", rank, current_content);
}


int main(int argc, char *argv[]) 
{
   
   //struct timeval t1,t2;
   
   MPI_Init(&argc,&argv);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   MPI_Comm_size(MPI_COMM_WORLD,&p);
   assert(p>=2);
   srand(time(NULL)+rank);//generate random num for each proc

    
    MPI_Barrier(MPI_COMM_WORLD);
    //Step 2. Each proc. calls the Sum_{X} function, where {X} is one of "byShift" or "byRing" or "byHypercube". 
    //gettimeofday(&t1,NULL);
    
    Sum_byHypercube(p);
    Sum_byRing(p);
    Sum_byShift(p);
    
    //gettimeofday(&t2,NULL);
    //long totalTime = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);//unit=micro-second
    //printf("*******The time=%ld\n",totalTime);


    MPI_Finalize();
}




