/*
Open File Data Structures:

    running
      |                                                  
      |                                                    ||****************** 
    PROC[ ]              OFT[ ]              MINODE[ ]     ||      Disk dev
  ===========    |---> ===========    |--> ============    || ===============
  |ProcPtr  |    |     |mode     |    |    |  INODE   |    || |      INODE   
  |pid, ppid|    |     |refCount |    |    | -------  |    || =============== 
  |uid      |    |     |minodePtr|---->    | dev,ino  |    || 
  |cwd      |    |     |offset   |         | refCount |    ||******************
  |         |    |     ====|======         | dirty    |
  |  fd[10] |    |         |               | mounted  |         
  | ------  |    |         |               ============
0 |   ----->|--->|         |
  | ------  |              |   
1 |         |              |
  | ------  |             --------------------------------
2 |         |             |0123456.............
  | ------  |             --------------------------------    
  ===========        logical view of file: a sequence of bytes
                          
 */  

//open f1 0
int open_file()
{
  /*1. ask for a pathname and mode to open:
         You may use mode = 0|1|2|3 for R|W|RW|APPEND*/
	char smode[256];
	int mode=0,ino=0;
	strcpy(smode, parameter);
	if(strcmp(smode,"0") && strcmp(smode,"1") && strcmp(smode,"2") && strcmp(smode,"3"))
    {
		printf("invalid mode %s\n",mode);
		return;
	}  
	mode=atoi(smode);
//  2. get pathname's inumber:
    if (pathname[0]=='/') 
		dev = root->dev;          // root INODE's dev
    else                  
		dev = running->cwd->dev;  
    ino = getino(&dev, pathname);  // get ino of the file. NOTE: dev may change with mounting
	if(ino==0) {
		printf("the file does not exist\n");
		printf("no such file\n");
	}

//  3. get its Minode pointer
	MINODE *mip = iget(dev,ino);  
	
/*  4. check mip->INODE.i_mode to verify it's a REGULAR file and permission OK.
*/
	if(!S_ISREG(mip->INODE.i_mode)) {
		printf("Error:it's not a regular file.\n");
		iput(mip);
		return -1;
	}
	if((running->uid!=mip->INODE.i_uid)&&(running->uid!=0))
  	{
		printf("No permission\n");
		iput(mip);
		return -1;
	}
/*	 Check whether the file is ALREADY opened with INCOMPATIBLE mode:
           If it's already opened for W, RW, APPEND : reject.
           (that is, only multiple R are OK)   inside running, OFT         *fd[NFD];an array of pointers to OFT
	typedef struct oft{
  		int  mode;
  		int  refCount;
  		MINODE *mptr;//it points to the file's INODE in memory
  		int  offset;
	}OFT;

*/
	int i=0;////check oft[NOFT];;for different procs
	//for that specific file, all null, mode=whatever; all 0/null, mode=0
	for(i=0;i<NOFT;i++) {
		if(oft[i].refCount==0) 
			continue;
		if(oft[i].refCount!=0 && oft[i].mptr->ino==ino) {
			if(mode!=0) {
				printf("file %s already opened with incompatible mode\n",pathname);
				iput(mip);
				return -1;
			}
			else {
				if(oft[i].mode!=0) {
					printf("file %s already opened with incompatible mode\n",pathname);
					iput(mip);
					return -1;
				}
			}
		}
	}
	
/*  5. allocate a FREE OpenFileTable (OFT) and fill in values: 
 
         oftp->mode = mode;      // mode = 0|1|2|3 for R|W|RW|APPEND 
         oftp->refCount = 1;
         oftp->minodePtr = mip;  // point at the file's minode[]
*/
  int mark=0;
  for(i=0;i<NOFT;i++) {
	 if(oft[i].refCount==0) {
		mark=1;//there is oft available
		break;
	 }
  }
  if(mark==0) {
	  printf("Can't open new files because the open file table is full\n");
	  iput(mip);
	  return -1;
  }
  oft[i].mode = mode;      // mode = 0|1|2|3 for R|W|RW|APPEND 
  oft[i].refCount = 1;
  oft[i].mptr = mip;  // point at the file's minode[]	
  //6. Depending on the open mode 0|1|2|3, set the OFT's offset accordingly:
  switch(mode){
     case 0 : oft[i].offset = 0;     // R: offset = 0
              break;
     case 1 : truncate(mip);        // W: truncate file to 0 size
              oft[i].offset = 0;
              break;
     case 2 : oft[i].offset = 0;     // RW: do NOT truncate file
              break;
     case 3 : oft[i].offset =  mip->INODE.i_size;  // APPEND mode
              break;
     default: printf("invalid mode\n");
              return(-1);
  }
  int j=0;
/*   7. find the SMALLEST i in running PROC's fd[ ] such that fd[i] is NULL
      Let running->fd[i] point at the OFT entry*/
	for(j=0;j<NFD;j++) {
		if(running->fd[j]==NULL) {
			break;
		}
	}
	running->fd[j]=&oft[i];
   /*8. update INODE's time field
         for R: touch atime. 
         for W|RW|APPEND mode : touch atime and mtime
      mark Minode[ ] dirty
	use mode = 0|1|2|3 for R|W|RW|APPEND
	*/
	if(mode==0) {
		mip->INODE.i_atime = time(0L); 
	}
	else {
		mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
	}
	mip->dirty=1;
	return j;//9. return j as the file descriptor
}

//close fd     
int close_file(int fd)
{
  //1. verify fd is within range.
  int i=0;
  if(fd>=NFD || fd<0) {
	printf("invalid fd\n");
	return -1;
  }
  //2. verify running->fd[fd] is pointing at a OFT entry
	if(running->fd[fd]==NULL) {
		printf("invalid fd\n");
		return -1;
	}

  //3. The following code segments should be fairly obvious:
     OFT *oftp = running->fd[fd];
     running->fd[fd] = 0;
     oftp->refCount--;
     if (oftp->refCount > 0) 
	 	return 0;

     // last user of this OFT entry ==> dispose of the Minode[]
     MINODE *mip = oftp->mptr;
     iput(mip);
	 return 0; 
}

int pfd()
{
/*This function displays the currently opened files as follows:

        fd     mode    offset    INODE
       ----    ----    ------   --------
         0     READ    1234   [dev, ino]  
         1     WRITE      0   [dev, ino]
      --------------------------------------
  to help the user know what files has been opened.

===========   pid = 0   =============
fd   mode  count  offset  [dev,ino] 
--   ----  -----  ------  --------- 
0    WRITE   1        0    [3, 12]     
======================================

typedef struct oft{
  int  mode;
  int  refCount;
  MINODE *mptr;//it points to the file's INODE in memory
  int  offset;
}OFT;

mode = 0|1|2|3 for R|W|RW|APPEND
*/
	printf("fd   mode     count  offset  [dev,ino] \n");
	printf("--------------------------------------\n");
	int i=0;
	for(i=0;i<NOFT;i++) {
	  if(oft[i].refCount==0) 
		  continue;
	  char modestr[10];
	  switch(oft[i].mode) {
     	  case 0 : strcpy(modestr,"READ");
                   break;
     	  case 1 : strcpy(modestr,"WRITE");
              	   break;
     	  case 2 : strcpy(modestr,"RDWR");
              	   break;
     	  case 3 : strcpy(modestr,"APPEND");
              	   break;
  		}		
	  printf("%d    %s      %3d     %3d    [%d, %d]\n",i,modestr,oft[i].refCount,oft[i].offset,oft[i].mptr->dev,oft[i].mptr->ino); 
	}	
	printf("--------------------------------------\n");	
}

int mylseek(int fd, int position)
{
  //From fd, find the OFT entry. running->fd[fd]
  //change OFT entry's offset to position but make sure NOT to over run either end of the file.
	if(running->fd[fd]->mptr->INODE.i_size>=position) {
		if(position>=0)
			running->fd[fd]->offset=position;
		else 
			running->fd[fd]->offset=0;
	}
	else 
		running->fd[fd]->offset=running->fd[fd]->mptr->INODE.i_size;
	//return originalPosition
	return running->fd[fd]->offset;
}


