#include <stdio.h>
#include <mpi.h> 
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define BIGPRIME 93563
#define ALIVE 1
#define DEAD 0


int n, G;//user specify parameters n, G: n= matrix column num, G=num of running iterations during each simulation
int rank,p;

int livingNeighborsNum;
int deadNeighborsNum;

void DisplayGoL(int mat[][n]);
//mat here is the submatrix (n/p) *n
void GenerateInitialGoL(int mat[][n]) {
/*
Rank 0 first generates p random numbers (in the interval of 1 and BIGPRIME2) and
distributes them such that the ith random number is handed over to rank i.
MPI_Scatter(
    void* send_data,
    int send_count,
    MPI_Datatype send_datatype,
    void* recv_data,
    int recv_count,
    MPI_Datatype recv_datatype,
    int root,
    MPI_Comm communicator)
*/
	int current_content=0;
	int send_data[p];
	struct timeval t1,t2;   	
    	long totalComTime=0L; 
	if(rank==0) {
		int i=0;
		
		for(i=0;i<p;i++) {
			send_data[i]=rand()%BIGPRIME+1;//[1,BIGPRIME]
			//printf("******send_data[%d]=%d\n",i,send_data[i]);
		}		
	}
	gettimeofday(&t1,NULL);
	MPI_Scatter(send_data,1,MPI_INT,&current_content,1,MPI_INT,0, MPI_COMM_WORLD);
	gettimeofday(&t2,NULL);
	totalComTime= (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);//unit=micro-second
    	printf("*****Rank %d: Communication time for each initialize=%ld micro-second\n",rank,totalComTime);
	//printf("******rank %d cur=%d\n",rank,current_content);
//code below is optimized by MPI_Scatter
/*	int current_content=0;
	if(rank==0) {
		int i=0;
		current_content=rand()%BIGPRIME+1;///for rank 0
		int randNum;
		for(i=1;i<p;i++) {
			randNum=rand()%BIGPRIME+1;//[1,BIGPRIME]
			MPI_Send(&randNum,1,MPI_INT,i,0,MPI_COMM_WORLD);
		}
	}
	else {
		MPI_Status status;
		int recv_content=0;
		MPI_Recv(&recv_content,1,MPI_INT,0,0,MPI_COMM_WORLD,&status);//receive from rank 0
		current_content=recv_content;
	}
*/
/*
Next, using the assigned random number as the “seed”, each rank (locally) generates a
distinct sequence of n2/p random values, again in the same interval. Each generated
random number is used in the following manner to fill the initial matrix: if the kth
random number is even, then the kth cell being filled in the local portion of the matrix is
marked with status=Alive; otherwise its state=Dead. (This is one way of generating the
matrix randomly. If you prefer to do it other ways, that’s fine too but make sure you use
different random seeds in different processes to avoid the problem of generating matrix
replicas across processes.) Also note that if you initialize this way, the initially filled
matrix is not necessarily compliant with the rules of the GoL. That’s okay. You will fix it
in the first iteration of your simulation. Read on…
*/
	//each rank produces the submatrix n/p *n
	srand(time(NULL)+current_content);
	int j=0,random=0;
	for(j=0;j<n*n/p;j++) {
		random=rand()%BIGPRIME+1;
		if(random%2==0) {
			mat[j/n][j%n]=ALIVE;
		}
		else {
			mat[j/n][j%n]=DEAD;
		}
		printf("rank %d:mat[%d][%d]=%d\n",rank,j/n,j%n,mat[j/n][j%n]);
	}
	printf("\n");
	//******
	//DisplayGoL(mat);
}

/*To determine the new state of a cell,write a function called DetermineState()that takes a cell coordinate and returns its new state (alive or dead)based on theGoLrules described above.For this to happen, however, you need to first make sure all cell states have access to their neighboring entries–i.e.,do the necessary communication to satisfy these dependencies apriori.You also need to ensure that all processes are executing the same generation at any given time. This can be done using a simpleMPI_Barrier at the start of every generation.
*/
//note: ghost rows should be constructed
int DetermineState(int r, int c,int mat[][n])//(r,c) is the coordinate(index)in big matrix n*n, mat is submat (n/p)*n
{
	
	MPI_Barrier(MPI_COMM_WORLD);
        struct timeval t1,t2;   	
    	long totalComTime=0L; 
	int livingNeighborsNum;
	int subr, subc,rankVal;
	int current_content;
	int i=0;
	MPI_Status status;
	//construct the submatrix with ghost rows
	int ghostRow1[n];
	int ghostRow2[n];
	//new row 0 = the previous proc's last row
	//new row (n/p)+1=the next proc's first row, in ring

	//each proc sends the last row to the next proc as the first ghost row using ring structure
	//the next proc will receive 
	//for(i=0;i<p-1;i++) {
	gettimeofday(&t1,NULL);
        	MPI_Send(mat[n/p-1],n,MPI_INT,(rank+1) % p,0,MPI_COMM_WORLD);//send to the next
        	MPI_Recv(ghostRow1,n,MPI_INT,(rank-1+p) % p,0,MPI_COMM_WORLD,&status);//receive from the previous
    	//}
	//each proc sends the first row to the previous proc as the last ghost row using ring structure
	//the previous proc will receive
	//for(i=0;i<p-1;i++) {
        	MPI_Send(mat[0],n,MPI_INT,(rank-1+p) % p,0,MPI_COMM_WORLD);//send to the previous
        	MPI_Recv(ghostRow2,n,MPI_INT,(rank+1) % p,0,MPI_COMM_WORLD,&status);//receive from the next proc
    	gettimeofday(&t2,NULL);
	totalComTime= (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);//unit=micro-second
    	printf("*****Rank %d: Communication time for each determinestate=%ld micro-second\n",rank,totalComTime);
	//}
	//by this step, finish constructing ghost rows
	//mat[subr][(subc-1+n)%n], mat[subr][(subc+1)%n] can be used directly
	//rest 6 neighbors need to be considered
	//build a new matrix including the ghost rows newMatrix[n/p+2][n]
	int newMatrix[n/p+2][n];
	//try to optimize this part using memcpy(destAddr,srcAddr,n)
	/*
	memcpy(newMatrix[0],ghostRow1,n);
	memcpy(newMatrix[n/p+1],ghostRow2,n);
	for(i=1;i<=n/p;i++) {
		memcpy(newMatrix[i],mat[i-1],n);			
	}
	*/
	int j=0;
	for(j=0;j<n;j++) {
		newMatrix[0][j]=ghostRow1[j];
		newMatrix[n/p+1][j]=ghostRow2[j];	
	}
	for(i=1;i<=n/p;i++) {
		for(j=0;j<n;j++) {
			newMatrix[i][j]=mat[i-1][j];
		}
	}
	
	rankVal=r/(n/p);
	//subr [1,n/p]
	subr=r%(n/p)+1;//row num in the submatrix including the ghost rows,+1 because of the ghostrows
	subc=c;//col num in the submatrix
	if(rank==rankVal) {
		livingNeighborsNum=newMatrix[subr][(subc-1+n)%n]+newMatrix[subr][(subc+1)%n]+newMatrix[subr-1][subc]+newMatrix[subr-1][(subc-1+n)%n]+newMatrix[subr-1][(subc+1)%n]+newMatrix[subr+1][subc]+newMatrix[subr+1][(subc-1+n)%n]+newMatrix[subr+1][(subc+1)%n];		
	}
	if(newMatrix[subr][subc]==ALIVE) {
		if(livingNeighborsNum<2 || livingNeighborsNum>3) {
			return DEAD;
		}
		if(livingNeighborsNum==3 || livingNeighborsNum==2) {
			return ALIVE;
		}
	}
	else {
		if(livingNeighborsNum==3) {
			return ALIVE;
		}
	}
	return DEAD;
}

/*
DisplayGoL():As the simulation proceeds it is desirable to display the contents of the entirematrix to visualize the evolution of the cellular automaton. However, doing this after every step of simulation could be cumbersome for large n.Therefore, we will do this infrequently–i.e.,display once after every x number of generations.I will let you decide the value of x based on the timings you see for a specific input size.To do the display,write a function called DisplayGoL()which first aggregates(i.e., gathers)the entire matrix in rank 0(“root”)and then displays it.(Note, displaying from every separate rank independently is possible but could possibly shuffle up the prints out of order.) As an alternative to each process sending its entire local matrix to the root you could choose to send only the alive entries. This approach wouldprovide some communication savings if the matrix is sparse with very few alive entries. However for simplicity if you want to just send the whole local matrix that is OK for this assignment. 
MPI_Gather(
    void* send_data,
    int send_count,
    MPI_Datatype send_datatype,
    void* recv_data,
    int recv_count,
    MPI_Datatype recv_datatype,
    int root,
    MPI_Comm communicator)
*/
void DisplayGoL(int mat[][n]) { //mat is the submatrix in each proc
    int arr[n][n];
    int r = n, c = n, i, j; 
    struct timeval t1,t2;   	
    long totalComTime=0L; 
 	
    MPI_Barrier(MPI_COMM_WORLD);
    //rank 0 gather from other ranks--verify
    gettimeofday(&t1,NULL);
    if(rank == 0) {
    	MPI_Gather(mat[0], n*(n/p), MPI_INT, arr[0], n*(n/p), MPI_INT, 0, MPI_COMM_WORLD);
    } else {
    	MPI_Gather(mat[0], n*(n/p), MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);
    }
    gettimeofday(&t2,NULL);
    totalComTime= (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);//unit=micro-second
    printf("*****Rank %d: Communication time for each display=%ld micro-second\n",rank,totalComTime);

    if(rank==0) {
	//display
	printf("========Display=========\n");
	printf("Processor %d has data:\n", rank);
    	for (i = 0; i <  r; i++) {
      		for (j = 0; j < c; j++) {
         		printf("%d ", arr[i][j]);
		}
		printf("\n");
	}
	printf("========================\n");
     }
}

//i didn't use this function DisplayGoL1 using malloc in my project, put here for self-reference
void DisplayGoL1(int mat[][n]) { //mat is the submatrix in each proc, using malloc

    int *arr[n];
    int r = n, c = n, i, j;   	
    if(rank==0) {
    	
    	for (i=0; i<r; i++) {
         	arr[i] = (int *)malloc(c * sizeof(int));
    	}
 
    	// Note that arr[i][j] is same as *(*(arr+i)+j)
    	//initialize
    	for (i = 0; i <  r; i++) {
      		for (j = 0; j < c; j++) {
         		arr[i][j] = 0; 
      		}
   	 }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    //rank 0 gather from other ranks--verify
    if(rank == 0) {
    	MPI_Gather(mat[0], n*(n/p), MPI_INT, arr[0], n*(n/p), MPI_INT, 0, MPI_COMM_WORLD);
    } else {
    	MPI_Gather(mat[0], n*(n/p), MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);
    }
    if(rank==0) {
	//display
	printf("========Display=========\n");
	printf("Processor %d has data:\n", rank);
    	for (i = 0; i <  r; i++) {
      		for (j = 0; j < c; j++) {
         		printf("%d ", arr[i][j]);
		}
		printf("\n");
	}
	printf("========================\n");
	
	//free up 2d array in rank 0
	for (i = 0; i < r; i++)
	{
    		int* currentIntPtr = arr[i];
		free(currentIntPtr);		
	}
     }
}

/*Simulate():This functionactually runs the Game of Life. The simulation is done for G generations(i.e., iterations). Within each iteration,rank i determines the new states for all the cell that it owns. */
void Simulate(int numIteration, int mat[][n], int displayFreq) { //mat is the submatrix without the ghost rows
	int j=0;
        int k=0;
	int newVal[n*n/p];

	struct timeval t1,t2;
    	
    	
    	long totalTime=0L;

 	for(k=0;k<numIteration;k++) {
		gettimeofday(&t1,NULL);
		for(j=0;j<n*n/p;j++) {
			int r=rank*(n/p)+j/n;
			int c=j%n;
			//keep both new values and old values, at the end of each iteration, change everything
			newVal[j]=DetermineState(r, c,mat);	
		}
		//replace mat[j/n][j%n] with new values
		for(j=0;j<n*n/p;j++) { 
			mat[j/n][j%n]=newVal[j];
		}
		gettimeofday(&t2,NULL);
		totalTime += (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);//unit=micro-second
		if(k%displayFreq==0) {
			DisplayGoL(mat);
		}
	}
	printf("^^^^^Rank %d: Average runtime to run simulation for %d iterations=%f micro-second\n",rank,numIteration,(double)totalTime/numIteration);
}


int main(int argc, char *argv[]) 
{
   
   /*for time calculation
    struct timeval t1,t2;
    gettimeofday(&t1,NULL);
    gettimeofday(&t2,NULL);
    long totalTime = (t2.tv_sec-t1.tv_sec)*1000000 + (t2.tv_usec-t1.tv_usec);//unit=micro-second
   */
   MPI_Init(&argc,&argv);
   //printf("argc=%d\n",argc);
   //printf("argv[0]=%s\n",argv[0]);//project3
//user specify parameters n, G: n= matrix column num, G=num of running iterations during each simulation
   n=atoi(argv[1]);
   G=atoi(argv[2]);
   printf("rank %d: argv[1]=%s columns in matrix; ",rank, argv[1]);
   printf("rank %d: argv[2]=%s iterations in each simulation\n",rank,argv[2]);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   MPI_Comm_size(MPI_COMM_WORLD,&p);
   assert(p>=2);   
   srand(time(NULL));
   MPI_Barrier(MPI_COMM_WORLD);

   int matrix[n/p][n];
   int displayFreq=G;//which stands for x in the requirement, I will display once after each simulation
   GenerateInitialGoL(matrix);
   Simulate(G, matrix,displayFreq);
   MPI_Finalize();
}
