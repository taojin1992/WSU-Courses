// adjacency list representation of graphs
/*
g++ -c -O3 -std=c++11 pageRank.cpp
g++ -o exe -fopenmp pageRank.o
./exe 32 0.15 875713 web-Google_sorted.txt 1 #threads

*/
#include <iostream>
#include <unordered_map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <math.h>
#include <assert.h>
#include <vector>
#include <queue>
using namespace std;

#define TOTAL 875713//4039//875713//need to modify by each case

//create map(nodeNum-index)
unordered_map<int, int> NodenumToIndex;

//create map(index-nodeNum)
unordered_map<int, int> IndexToNodenum;//m["foo"] = 42;
//int IndexToNodenum[TOTAL];

//utility 2d vector
vector<vector<int>> fastWalkMatrix;

//count of visits for each node
long int count[TOTAL];//use an array of size V to record the num of visits for a mapped node num

// A structure to represent an adjacency list node

struct AdjListNode {

    int dest;
    struct AdjListNode* next;

};

     
// A structure to represent an adjacency liat

struct AdjList {

     struct AdjListNode *head; // pointer to head node of list
	 int du;//the number of neighbors
};


//A graph is an array of adjacency lists.

// Size of array will be V (number of vertices in graph)
class Graph

{

    private:

        int V;

        

    public:
		struct AdjList* array;
		Graph(int V)

        {

            this->V = V;

            array = new AdjList [V];

            for (int i = 0; i < V; ++i)

                array[i].head = NULL;

        }
		int getSize() {
			return V;
		}

        void addEdge(int src, int dest,int directedflag)

        {
			//use the mapped value based on NodenumToIndex
            int src1=NodenumToIndex[src];
			int dest1=NodenumToIndex[dest];

			AdjListNode* newNode = newAdjListNode(dest1);

            newNode->next = array[src1].head;

            array[src1].head = newNode;
			// Since graph is undirected, add an edge from dest to src also
			if(directedflag==0) {
            	newNode = newAdjListNode(src1);

            	newNode->next = array[dest1].head;

            	array[dest1].head = newNode;
			}

        }
		void constructGraph(int V, FILE* fp, int directedFlag)
        {

            this->V = V;

            array = new AdjList [V];

            for (int i = 0; i < V; ++i) {

                array[i].head = NULL;
				array[i].du=0;
			}
			//read from the document, skip the comment lines
			const int bsz = 32; 
			char buf[bsz];
			while((fgets(buf,bsz,fp))!=NULL) {
  			if (buf[0] == '#') continue;
  			int x,y;
  			sscanf(buf, "%d %d\n", &x,&y);
  			addEdge(x, y, directedFlag);

			}
    		
			fclose(fp);
			
        }

        AdjListNode* newAdjListNode(int dest)

        {

            AdjListNode* newNode = new AdjListNode;

            newNode->dest = dest;

            newNode->next = NULL;

            return newNode;

        }

        void printGraph()//update du field as well

        {

            int v;
			int count=0;

            for (v = 0; v < V; ++v)

            {
				count=0;
                AdjListNode* pCrawl = array[v].head;

                //cout<<"\n Adjacency list of vertex "<<v<<"\n head ";

                while (pCrawl)

                {

                    //cout<<"-> "<<pCrawl->dest;

                    pCrawl = pCrawl->next;
					count++;

                }
				array[v].du=count;
				//printf("\n node %d neighbor num = %d",v,array[v].du);
                //cout<<endl;

            }

        }

};

void creatTwoMaps(FILE *fp) {
	//read from the document, skip the comment lines
	const int bs = 32; 
	char buffer[bs];
	int position=-1;
	int index=0;
	while((fgets(buffer,bs,fp))!=NULL) {
  		if (buffer[0] == '#') 
			continue;
		//puts(buffer);
  		int x,y;
  		sscanf(buffer, "%d %d\n", &x,&y);
		//printf("%d %d\n", x,y);
		//if map contains x, then no need for anything
		//if map doesn't have x, then pick the index
		std::unordered_map<int,int>::const_iterator got =NodenumToIndex.find(x);
		if (got == NodenumToIndex.end()) {
    		//std::cout << "not found";
			NodenumToIndex[x]=index;
			//printf("*%d --- %d \n",x, index);
			index++;
		}
  		std::unordered_map<int,int>::const_iterator got1 =NodenumToIndex.find(y);
		if (got1 == NodenumToIndex.end()) {
    		//std::cout << "not found";
			NodenumToIndex[y]=index;
			//printf("#%d --- %d \n",y, index);
			index++;
		}
	}
	//fill in IndexToNodenum map
	for (std::unordered_map<int,int>::iterator i = NodenumToIndex.begin(); i != NodenumToIndex.end(); ++i)
    	IndexToNodenum[i->second] = i->first;

	fclose(fp);	
} 

void updatefastWalkMatrix(Graph gh) {
	int v;

    for (v = 0; v < gh.getSize(); ++v)

    {
		vector<int> row;
		
        AdjListNode* pCrawl = gh.array[v].head;
		if(pCrawl==NULL) {
			row.push_back(-1);//NO neighbors
			fastWalkMatrix.push_back(row);
			continue;
		}

        //cout<<"\n Adjacency list of vertex "<<v<<"\n head ";

        while (pCrawl)

        {

             row.push_back(pCrawl->dest);

             pCrawl = pCrawl->next;
		     

        }
		
		fastWalkMatrix.push_back(row);
		
        //cout<<endl;

     }
}

void printfastWalkMatrix() {
	for (int i = 0; i < fastWalkMatrix.size(); i++)
	{
    	for (int j = 0; j < fastWalkMatrix[i].size(); j++)
    	{
        	cout << fastWalkMatrix[i][j] << ' ';
    	}
		cout<<endl;
	}
}

void printTopNodes(int k, int length) {
  	std::priority_queue<std::pair<int, int>> q;
  	for (int i = 0; i < TOTAL; ++i) {
    	q.push(std::pair<long int, int>(count[i], i));
  	}
  	for (int i = 0; i < k; ++i) {
    	int ki = q.top().second;
    	//std::cout << "index[" << i << "] = " << ki << std::endl;
		//use  IndexToNodenum map back, ki is the index
		std::cout << IndexToNodenum[ki] << "," << count[ki] << ","<< (long double)count[ki]/(long double)(TOTAL*length) << std::endl;
    	q.pop();
  	}

}

// Driver program to test above functions
int main(int argc, char *argv[]) {
	int p;
	long double pageRank[5];//get top 5 pageranks
	long double Time,total_time;

    // create the graph given in above fugure
	//argv[1]=K, length of random walk
	int K = atoi(argv[1]);
	//argv[2]=D, damping ratio
	double D = atof(argv[2]);
    //argv[3] = number of total nodes
	int V=atoi(argv[3]);
	//argv[4] = filename
	FILE *fp, *fp1;
	fp = fopen(argv[4],"r");//for populating the two maps
	fp1 = fopen(argv[4],"r");//for adding edges to the graph
	if(fp ==NULL || fp1 ==  NULL) {
		printf("Cannot open the file. Error!\n");
		return -1;
	}
	//argv[5]-directed (1) or undirected (0) flag
	int directedFlag = atoi(argv[5]);
	//num of threads
	p=atoi(argv[6]);
	Graph gh(V);

	//read from fp and create two maps
	creatTwoMaps(fp);
	//all node number can be mapped within [0,V-1]
	//when adding edges, using the mapping node value [0, V-1] 
	gh.constructGraph(V, fp1, directedFlag);
    // print the adjacency list representation of the above graph
	gh.printGraph();
	//update fastWalkMatrix
	updatefastWalkMatrix(gh);
	//printfastWalkMatrix();
	

	assert(p>=1);
	//printf("number of requested threads = %d\n",p);
	omp_set_num_threads(p);
 

    //for loop to start from each node i, K length, D damping ratio (jumping out)
	//walk based on fastWalkMatrix[v][which neighbor] and update count[TOTAL];
	int i=1;	
	Time = omp_get_wtime();
	
	
	#pragma omp parallel for num_threads(p) schedule(static)
	{
		for(i=0;i<V;i++) {
			unsigned int seed = time(NULL)+omp_get_thread_num();//(omp_get_thread_num() + 1)*i;
			int start=i;
			int totalWalk=K;
			double r;
			int num, index;
			while(totalWalk > 0) {
				r=(double)rand_r(&seed)/(double)RAND_MAX;
				//jump out to V possible nodes
				if(r<D) {
					num=rand_r(&seed)%V;
				}
				else { //go to neighbors based on fastWalkMatrix
					//if no neighbors, go to V possible nodes
					if(fastWalkMatrix[start][0]==-1) {
						num=rand_r(&seed)%V;
					}
					else {
						index=rand_r(&seed)%(fastWalkMatrix[start].size());//which neighbor to go
						num=fastWalkMatrix[start][index];
					}
				
				}	
				start=num;
				totalWalk--;
				#pragma omp atomic
				count[num]++;				
			}
			//printf("%d    %d\n", omp_get_thread_num(), IndexToNodenum[num]);
		}
	}
	total_time = omp_get_wtime() - Time;
	printf("%d,%d,%LF\n",K,p,total_time);//walklength,#thread,time
	printTopNodes(5,K);//nodenum,count,pagerankvalue
	//printf("------------------------------------\n");
    return 0;

}
