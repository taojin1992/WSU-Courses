/*
Project5-Jin Tao 11474660
In this project you will implement an OpenMP multithreaded PI value estimator using the algorithm discussed in class. This algorithm essentially throws a dart n times into a unit square and computes the fraction of times that dart falls into the embedded unit circle. This fraction multiplied by 4 gives an estimate for PI.
Here is the classroom scribe for further details: PDF

Your code should expect two arguments: <n> <number of threads>.
Your code should output the PI value estimated at the end.  Note that the precision of the PI estimate could potentially vary with the number of threads (assuming n is fixed).
Your code should also output the total time (calculated using omp_get_wtime function).

Experiment for different values of n (starting from 1024 and going all the way up to a billion or more) and p (1,2,4..). 

Please do two sets of experiments as instructed below:

1) For speedup - keeping n fixed and increase p (1,2,4, 8). You may have to do this for large values of n to observe meaningful speedup. Calculate relative speedup. Note that the Pleiades nodes have 8 cores per node. So there is no need to increase the number of threads beyond 8. In your report show the run-time table for this testing and also the speedup chart.
PS: If you are using Pleiades (which is what I recommend) you should still use the Queue system (SLURM) to make sure you get exclusive access to a node.   For this you need to run sbatch with -N1 option.

2) For precision testing - keep n/p fixed, and increase p (1,2,.. up to 16 or 32). For this you will have to start with a good granularity (n/p) value which gave you some meaningful speedup from experiment set 1. The goal of this testing is to see if the PI value estimated by your code improves in precision with increase in n. Therefore, in your report make a table that shows the PI values estimated (up to say 20-odd decimal places) with each value of n tested.

To run the code, gcc -o project5 -fopenmp project5.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <math.h>
#include <assert.h>

int main(int argc,char *argv[])
{
	long long int n;
    int p;
    long long int count=0;
    long double PI,k,Time,total_time;
    
	//input {number of iterations} [number of threads]
	if(argc!=3) {
		printf("Usage: loop {number of iterations} [number of threads]\n");
		exit(1);
	}
	n=atoll(argv[1]);
    p=atoi(argv[2]);
   
	//printf("number of iterations = %lld\n",n);
	assert(p>=1);
	//printf("number of requested threads = %d\n",p);
	omp_set_num_threads(p);
    Time = omp_get_wtime();
    
    long long int i = 0;
    //To share the LEAST amount of variables possible here
    #pragma omp parallel shared(n) reduction(+:count)
    {
        unsigned int seed = (omp_get_thread_num() + 1)*i;
		#pragma omp for schedule(static) 
		for (i=0; i<n; i++)
        {
            //simulation of randomly throwing a dart s.t. dart is in the unit square           
            //to declare and use all local variables INSIDE parallel for loop
            double X, Y, d;
            //the random_r function requires a pointer to an unsigned int. And we must use the random_r for a threadsafe random number
                    
            //to keep ALL variables used INSIDE parallel for loop.
            //the only variables that should be shared is returning Sum result (count), and the iterator (n)
            X = (double)rand_r(&seed)/RAND_MAX;
            Y = (double)rand_r(&seed)/RAND_MAX;
            d = (X-0.5)*(X-0.5)+(Y-0.5)*(Y-0.5);//d=squared distance to (0.5,0.5)
            //d'=sqrt(d)<=0.5, d<=0.25
            if (d<=0.25)//Dart is inside the circle
            {
                count++;				
            }		
        }
    }

	k = (long double)count/n;
	PI=4.0*k;
	//output the total time (calculated using omp_get_wtime function). 
	total_time = omp_get_wtime() - Time;
	//printf("Total Time=%lf seconds\n",total_time);//unit in seconds
	//printf("Estimated Pi: %.20lf\n", PI);
    /****for better analysis, print in csv form: n,p,time,pi****/
    printf("%lld,%d,%LF,%.20LF\n",n,p,total_time,PI);
    
	return 0;
}


