#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

int dev;
int nblocks;
int ninodes;
int bmap;
int imap;
int iblock;
int n;//the number of tokens of input dir

/****get_block/put_block, tst/set/clr bit functions******/
int get_block(int dev, int blk, char buffer[])
{
  //printf("get_block: fd=%d blk=%d buf=%x\n",fd,blk,buf);
  lseek(dev, (long)blk*BLKSIZE, 0);
  read(dev, buffer, BLKSIZE);
}

int put_block(int dev,int blk,char buffer[])
{
	lseek(dev, (long)blk*BLKSIZE, 0);
  	write(dev, buffer, BLKSIZE);
}

int tst_bit(char *buffer, int bit)
{
  int i, j;
  i = bit / 8;  j = bit % 8;
  if (buffer[i] & (1 << j))
     return 1;
  return 0;
}

int set_bit(char *buffer, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buffer[i] |= (1 << j);
}

int clr_bit(char *buffer, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buffer[i] &= ~(1 << j);
}

int get_iblock_value() {
	char buffer[BLKSIZE];
  	get_block(dev, 2, buffer);  
  	gp = (GD *)buffer;
  	return gp->bg_inode_table;//inodes begin block number
}
/*************************************
FS Level-1 Data Structures

PROC* running           MINODE *root                          
      |                          |              
      |                          |               ||*********************
      V                          |  MINODE       || 
    PROC[0]                      V minode[100]   ||         Disk dev
 =============  |-pointerToCWD-> ============    ||   ==================
 |nextProcPtr|  |                |  INODE   |    ||   |     INODEs   
 |pid = 1    |  |                | -------  |    ||   ================== 
 |uid = 0    |  |                | (dev,2)  |    || 
 |cwd --------->|                | refCount |    ||*********************
 |           |                   | dirty    |
 |fd[16]     |                   |          |         
 | ------    |       
 | - ALL 0 - |                   |==========|         
 | ------    |                   |  INODE   |          
 | ------    |                   | -------  |   
 =============                   | (dev,ino)|   
                                 |  refCount|  
   PROC[1]          ^            |  dirty   |   
    pid=2           |
    uid=1           |  
    cwd ----> root minode        |==========|  

**************************************/
//MINODE *mip = iget(dev, ino)
//int iput(MINDOE *mip)
//int ino = getino(int *dev, char *pathname)

// load INODE at (dev,ino) into a minode[]; return mip->minode[]
MINODE *iget(int dev, int ino)
{
  int i, blk, disp;
  char buffer[BLKSIZE];
  MINODE *mip;
  INODE *ip;
/*(1). search minode[ ] array for an item pointed by mip with the SAME (dev,ino)
     if (found){
        mip->refCount++;  // inc user count by 1
        return mip;
     }
*/
  for (i=0; i < NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount>0 && mip->dev == dev && mip->ino == ino){
       (mip->refCount)++;
   //    printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
/*
(2). search minode[ ] array for an item pointed by mip whose refCount=0:
       mip->refCount = 1;   // mark it in use
       assign it to (dev, ino); 
       initialize other fields: dirty=0; mounted=0; mountPtr=0
*/
  for (i=0; i < NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
  //     printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev; mip->ino = ino;  // assign to (dev, ino)
       mip->dirty = mip->mounted = mip->mptr = 0;
/*
(3). use mailman to compute

       blk  = block containing THIS INODE
       disp = which INODE in block
    
       load blk into buf[ ];
       INODE *ip point at INODE in buf[ ];

       copy INODE into minode.INODE by
       mip->INODE = *ip;
*/
       // get INODE of ino into buf[ ]
	   iblock=get_iblock_value();      
       blk  = (ino-1)/8 + iblock;  // iblock = Inodes start block #
       disp = (ino-1) % 8;
       //printf("iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);
       get_block(dev, blk, buffer);
       ip = (INODE *)buffer + disp;
       // copy INODE to mp->INODE
       mip->INODE = *ip;
//(4). return mip;
       return mip;
    }
  }   
	
  printf("PANIC: no more free minodes\n");
  return 0;
}

int iput(MINODE *mip)  // dispose of a minode[] pointed by mip
{
	char buffer[BLKSIZE];
//(1). mip->refCount--;
	(mip->refCount)--;
/*(2). if (mip->refCount > 0) return;
     if (!mip->dirty)       return;*/
	if (mip->refCount > 0) 
		return;
    if (!mip->dirty)      
		return;
//(3).write INODE back to disk
	printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino); 
/* Use mip->ino to compute 
     blk containing this INODE
     disp of INODE in blk
*/
     iblock=get_iblock_value();
	 int blk  = (mip->ino-1)/8 + iblock;  // iblock = Inodes start block #
     int disp = (mip->ino-1) % 8;
     get_block(mip->dev, blk, buffer);
     ip = (INODE *)buffer + disp;
     *ip = mip->INODE;
     put_block(mip->dev, blk, buffer);	
} 

int search(MINODE *mip, char *target)
{
	char dbuf[BLKSIZE],ibuf[BLKSIZE];
	char *cp;
	int InodesBeginBlock=get_iblock_value();
	int num=0,result=0,i=0;//initialize result to mip->ino	
/*
Mailman's algorithm:
	 	blk    = InodeBeginBlock + (ino-1)/8
	 	offset = (ino-1)%8
		get_block(fd, blk, ibuf[]);         // get block containing this INODE
     	INODE *ip = (INODE *)ibuf + offset; // ip points at INODE of /a in ibuf[ ]
		 NOTE: the number 8 comes from : for FD, blockSize=1024 and inodeSize=128. 
         If BlockSize != 1024, you must adjust the number 8 accordingly.
*/
	int blockNum=InodesBeginBlock+(mip->ino-1)/8;
    int offset=(mip->ino-1)%8;
	get_block(mip->dev,blockNum,ibuf); 
	INODE *inodePtr=(INODE *)ibuf+offset;
	printf("search for %s in MINODE = [%d, %d]\n",target, mip->dev,mip->ino);
	for(i=0;i<12 && inodePtr->i_block[i]!=0;i++) {
			printf("i=%d i_block[%d]=%d\n",i,i,inodePtr->i_block[i]);
    		printf(" i_number rec_len name_len   name\n");
			get_block(mip->dev, inodePtr->i_block[i],dbuf);
  			dp=(DIR *)dbuf;
 			cp=dbuf;
  			while(cp<&dbuf[BLKSIZE]){
			//a better way recommeded by kc without using a buffer
    			char temp=dp->name[dp->name_len];
    			dp->name[dp->name_len]=0;
    			printf("%8d %8d %4d %10s\n",dp->inode, dp->rec_len,dp->name_len,dp->name);
				if(strcmp(dp->name,target)==0) {
					result=dp->inode;
					printf("found %s : ino = %d\n",target, result);
					return result;
				}
    			dp->name[dp->name_len]=temp;
   	    		cp+=dp->rec_len;
	    		dp=(DIR *)cp;
		 	}
			
	}
	return result;
}


int tokenize(char **tokenname, char *pathname) {
	printf("tokenize %s\n",pathname);
  	tokenname[0]=strtok(pathname,"/");	
  	int j=1,i=0;	
  	while(tokenname[j-1]!=NULL)
  	{
   		tokenname[j]=strtok(NULL,"/"); 
    	j++;
  	}	
	while(tokenname[i]!=0) {
		printf("%s  ",tokenname[i]);
		i++;
	}
	printf("\n");
  	n=j-1;
  	printf("n=%d\n",n);
	return n;
}


int getino(int *dev, char *pathname)
{
  int i=0, ino=0;
  char buffer[BLKSIZE];
  INODE *ip;
  MINODE *mip;
  char *tokenname[256];
  for(i=0;i<256;i++) {
	  tokenname[i]=0;
  }
  printf("getino: pathname=%s\n", pathname);
  
  if (strcmp(pathname, "/")==0)
      return 2;

  if (pathname[0]=='/')
     mip = iget(*dev, 2);
  else//get the ino of current working dir if pathname=""
     mip = iget(running->cwd->dev, running->cwd->ino);

  strcpy(buffer, pathname);
  int n=tokenize(tokenname,buffer); // n = number of token strings
/*  char temp2[n][256];
  for(i=0; i < n; i++){// use temp copy for weird reasons. tokenname will be changed. 
  	strcpy(temp2[i],tokenname[i]);
  }*/  
  for (i=0; i < n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, tokenname[i]);
      //ino = search(mip,temp2[i]);
	  ino = search(mip,tokenname[i]);
      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", tokenname[i]);
         return 0;
      }
      iput(mip);
      mip = iget(*dev, ino);
   }
   iput(mip); 
   return ino;
}





