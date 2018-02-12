/*
Name: Jin Tao
ID: 11474660
Project 4: 
In this project you will implement a parallel random number series generator, using the Linear Congruential generating model. Here, the ith random number, denoted by xi, is given by:
        xi = (a*xi-1 + b) mod P,         where, a and b are some positive constants and P is a big constant (typically a large prime).
Your goal is to implement an MPI program for generating a random series up to the nth random number of the linear congruential series (i.e., we need all n numbers of the series, and not just the nth number). We discussed an algorithm that uses parallel prefix in the class. You are expected to implement this algorithm. Further explanation of the parallel algorithm can be found in the notes by Prof. Aluru (chapter 1). Operate under the assumption that n>>p. Your code should have your own explicit implementation the parallel prefix operation. Your code should also get parameter values {a,b,P} and the random seed to use (same as x0), from the  user.
It should be easy to test your code by writing your own simple serial code and comparing your output.
Performance analysis:
a) Generate speedup charts (speedups calculated over your serial implementation), fixing n to a large number such as a million and varying the number of processors from 2 to 64.
b) Study total runtime as a function of n. Vary n from a small number such as 16 and keep doubling until it reaches a large value (e.g., 1 million).  
Compile a brief report that presents and discusses your results/findings. 
*/
#include <stdio.h>
#include <mpi.h> 
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

typedef struct matrix {
   long data[2][2];
}mat;

int n;
int a;
int b;//a and b are some positive constants
int P;//a big constant (typically a large prime)
int randSeed;//the random seed to use (same as x0)

mat send_content;
mat recv_content;
MPI_Status status;
int rank,p;

mat buildUnitMat() {
   mat unit;
   unit.data[0][0]=1;
   unit.data[0][1]=0;
   unit.data[1][0]=0;
   unit.data[1][1]=1;
   return unit;
}

mat buildNormalMat(int x0_0, int x0_1, int x1_0, int x1_1) {
   mat result;
   result.data[0][0]=x0_0;
   result.data[0][1]=x0_1;
   result.data[1][0]=x1_0;
   result.data[1][1]=x1_1;
   return result;
}

//utility function: 2*2 matrix multiplication
mat multiplyMatrix(mat m1, mat m2) {
   mat result;
   result.data[0][0]=(m1.data[0][0])*(m2.data[0][0])+(m1.data[0][1])*(m2.data[1][0]);
   result.data[0][1]=(m1.data[0][0])*(m2.data[0][1])+(m1.data[0][1])*(m2.data[1][1]);
   result.data[1][0]=(m1.data[1][0])*(m2.data[0][0])+(m1.data[1][1])*(m2.data[1][0]);
   result.data[1][1]=(m1.data[1][0])*(m2.data[0][1])+(m1.data[1][1])*(m2.data[1][1]);
   return result;
}

//to avoid the integer overflow
mat modmultiplyMatrix(mat m1, mat m2,int mod) {
   mat result;
   result.data[0][0]=(((m1.data[0][0])*(m2.data[0][0]))%mod+(m1.data[0][1])*(m2.data[1][0]))%mod;
   result.data[0][1]=0;
   result.data[1][0]=(((m1.data[1][0])*(m2.data[0][0]))%mod+(m1.data[1][1])*(m2.data[1][0]))%mod;
   result.data[1][1]=1;
   return result;
}

//print the matrix
void printMatrix(mat m, char* tag) {
   printf("%s rank=%d "
   	"***[%ld     %ld], "
        "[%ld     %ld]\n", tag, rank, m.data[0][0], m.data[0][1], m.data[1][0],m.data[1][1]);
}

//arr=[unit matrix,A,A,....A^(n-1)], A is 2*2 matrix
mat parallelPrefix(mat *arr, int NumOfProc,int mod) {
  //initialize each rank  
   int d=0;
   int i=0,j=0;
   
   mat unit=buildUnitMat(); 
   mat l=unit;   
   if(rank!=0) {	
	l=unit;
   	for(j=0;j<n/NumOfProc;j++) {	       
		l=modmultiplyMatrix(l,arr[j],mod);	
   	}
   }  
   // printMatrix(l);
   mat g=l;
   struct timeval t1,t2;
   gettimeofday(&t1,NULL);
   for(d=0;(1<<d)<NumOfProc;d++) {
         int mate=rank^(1<<d);//use bit shift which is more efficient instead of rank^((int)pow(2.0,(double)d));
         //printf("***rank=%d mate=%d\n",rank,mate);
         MPI_Sendrecv(&(g.data[0][0]),4 * sizeof(long),MPI_CHAR,mate,0,&(recv_content.data[0][0]),4 * sizeof(long),MPI_CHAR,mate,0,MPI_COMM_WORLD,&status);        
         g=modmultiplyMatrix(g, recv_content,mod);
         if(rank>mate) {
             l=modmultiplyMatrix(l, recv_content,mod);
		//printMatrix(recv_content, "recv");
   		//printMatrix(l, "local");
	 }
   }
   gettimeofday(&t2,NULL);
   double Time = (t2.tv_sec-t1.tv_sec) + (double)(t2.tv_usec-t1.tv_usec)/1000000.0;//unit=second
   //printf("Time=%f\n", Time);
   return l;
}

int main(int argc, char *argv[]) 
{
   
   //struct timeval t1,t2;
   
   MPI_Init(&argc,&argv);
   //get parameter values {a,b,P} and the random seed to use (same as x0), from the  user. 
   a=atoi(argv[1]);
   b=atoi(argv[2]);
   P=atoi(argv[3]);
   randSeed=atoi(argv[4]);
   n=atoi(argv[5]);//number of random number the user wants
   
/*	
   printf("rank %d: a=%s; ",rank, argv[1]);
   printf("rank %d: b=%s; ",rank, argv[2]);
   printf("rank %d: P=%s; ",rank, argv[3]);
   printf("rank %d: random seed to use (same as x0)=%s; \n",rank, argv[4]);
   printf("rank %d: n=%s; \n",rank, argv[5]);
*/
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   MPI_Comm_size(MPI_COMM_WORLD,&p); 
   assert(p>=2);
   mat initArr[p];
   //initArr[0]=buildUnitMat();//buildNormalMat(a, 0, b, 1);//buildUnitMat();
   int i=0;
   for(i=0;i<p;i++) {
	initArr[i]=buildNormalMat(a, 0, b, 1);
   } 
   MPI_Barrier(MPI_COMM_WORLD);
   struct timeval t1,t2;
   gettimeofday(&t1,NULL);
   //for each rank, use parallel prefix to get the final local
   mat finalLocal=parallelPrefix(initArr,p,P);
   //printMatrix(finalLocal, "final");
   //for each rank, calculate the random number for each
   int j=0;
   long rand[n/p];
   mat x=buildNormalMat(a, 0, b, 1);
   mat r=finalLocal;
   
   for(j=0;j<n/p;j++) {	
	rand[j]=(randSeed*(r.data[0][0])+r.data[1][0])%P;
	//print the group of random numbers in each rank        
        //printf("^^^rank=%d   random number rand[%d]=%ld\n", rank, j,rand[j]);
	
	r=modmultiplyMatrix(r,x,P);
	//printMatrix(r);	
   } 
   
   gettimeofday(&t2,NULL);
   double Time = (t2.tv_sec-t1.tv_sec) + (double)(t2.tv_usec-t1.tv_usec)/1000000.0;//unit=second
   //printf("Time=%f\n", Time);
   MPI_Finalize();
}
