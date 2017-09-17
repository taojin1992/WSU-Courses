//rmdir
//Assume: command line = "rmdir pathname",rm==unlink

//1. Extract cmd, pathname from line and save them as globals.
int IsEmpty(MINODE *mip) {
/*
First, check link count (links_count > 2 means not empty);
     However, links_count == 2 may still have FILEs, so go through its data 
     block(s) to see whether it has any entries in addition to . and ..
*/	
	char buffer[BLKSIZE];
	int i=0,count=0;
	char *cp;
	DIR *dptr;
	if(mip->INODE.i_links_count>2) {
		return 1;//not empty
	}
	else if(mip->INODE.i_links_count==2) {
		//traverse the data block to check total number of entries
		for(i=0;i<12 && mip->INODE.i_block[i]!=0;i++) {
			get_block(mip->dev, mip->INODE.i_block[i], buffer);
			dptr = (DIR *)buffer;
    		cp = buffer;
    		// step to LAST entry in block:
    		while (cp< buffer + BLKSIZE){
				count++;
          		cp += dptr->rec_len;
          		dptr = (DIR *)cp;			
     		}  
		}
		if(count>2) {//because of . and ..
			return 1;//include files too, not empty
		}
		return 0;	
	}
}
     
int rmdir()
{
  int i=0;
  //2. get inumber of pathname: determine dev, then
  char pathname_copy[256];
  char pathname_copy1[256];
  strcpy(pathname_copy,pathname);
  strcpy(pathname_copy1,pathname);
  char *parent=dirname(pathname_copy); 
  int pino= getino(&dev, parent);
  int ino = getino(&dev, pathname);
  if(ino==0) {
	 printf("The dir doesn't exist\n");
	 return -1;
  } 
  //3. get its minode[ ] pointer:
  MINODE *mip = iget(dev, ino);
  /*4. check ownership 
       super user : OK
       not super user: uid must match
 */
  if((running->uid!=mip->INODE.i_uid)&&(running->uid!=0))
  {
	printf("No permission\n");
    iput(mip);
	return -1;
  }
  /*
  5. check DIR type (HOW?) AND not BUSY (HOW?) AND is empty:
     int IsEmpty(MINODE *mip):
     HOWTO check whether a DIR is empty:
     First, check link count (links_count > 2 means not empty);
     However, links_count == 2 may still have FILEs, so go through its data 
     block(s) to see whether it has any entries in addition to . and ..

     if (NOT DIR || BUSY || not empty): iput(mip); return -1;
  */
  if(!S_ISDIR(mip->INODE.i_mode) || mip->refCount!=1 || IsEmpty(mip)) {
		iput(mip);
		if(!S_ISDIR(mip->INODE.i_mode)) {
			printf("%s is not a direcory\n",pathname);
		}
		if(mip->refCount!=1) {
			printf("%s is busy\n",pathname);	
		}
		if(IsEmpty(mip)) {
			printf("dir %s is non_empty\n",pathname);
		}
		return -1;	    
  }
  
/*	
  6. ASSUME passed the above checks.
     Deallocate its block and inode*/
  for (i=0; i<12; i++){
      if (mip->INODE.i_block[i]==0)
          continue;
      bdealloc(mip->dev, mip->INODE.i_block[i]);
  }
  idealloc(mip->dev, mip->ino);
  iput(mip); //(which clears mip->refCount = 0);

//  7. get parent DIR's ino and Minode (pointed by pip);
//        MINODE* pip = iget(mip->dev, parent's ino); 
  MINODE* pip = iget(mip->dev, pino); 

/*
  8. remove child's entry from parent directory by

        rm_child(MINODE *pip, char *name);
        pip->parent Minode, name = entry to remove
*/
  char *name  = basename(pathname_copy1);
  if(strcmp(name,"..")==0 || strcmp(name,".")==0 || strcmp(name,"/")==0) {
		printf("Cannot remove the parent/itself/root. Permission denied.\n");
		return -1;
  }
  
  rm_child(pip, name);
  
/*
  9. decrement pip's link_count by 1; 
     touch pip's atime, mtime fields;
     mark pip dirty;
     iput(pip);
     return SUCCESS;
*/
	pip->INODE.i_links_count--;//decrement pip's link_count by 1; 
    pip->INODE.i_atime=time(0L);
	pip->INODE.i_mtime=time(0L);//touch pip's atime, mtime fields;
    pip->dirty=1;
    iput(pip);
	return 0;
}

// rm_child(): removes the entry [INO rlen nlen name] from parent's data block.
int rm_child(MINODE *parent, char *name)
{
    char buffer[BLKSIZE];
	char temp;
	char *cp,*previous;
	DIR *dptr;
    int i=0,index=0,count=0;
	int cover_len=0;
	int rm_entrylen=0;
	int move_entry_length=0;
	//1. Search parent INODE's data block(s) for the entry of name
	for(i=0;i<12 && parent->INODE.i_block[i]!=0;i++) {
			count=0;
			printf("i=%d i_block[%d]=%d\n",i,i,parent->INODE.i_block[i]);
    		//printf(" i_number rec_len name_len   name\n");
			get_block(parent->dev, parent->INODE.i_block[i],buffer);
  			dptr=(DIR *)buffer;
 			cp=buffer;
  			while(cp<&buffer[BLKSIZE]){
			//a better way recommeded by kc without using a buffer
    			char temp=dptr->name[dptr->name_len];
    			dptr->name[dptr->name_len]=0;
    			//printf("%8d %8d %4d %10s\n",dptr->inode, dptr->rec_len,dptr->name_len,dptr->name);
				if(strcmp(dptr->name,name)==0) {
					//2. Erase name entry from parent directory by
					/*(2). if (first and only entry in a data block){
         			 deallocate the data block; modify parent's file size;

          				-----------------------------------------------
          				|INO Rlen Nlen NAME                           | 
         			 	-----------------------------------------------
          
          			Assume this is parent's i_block[i]:
          			move parent's NONZERO blocks upward, i.e. 
               		i_block[i+1] becomes i_block[i]
               		etc.
          			so that there is no HOLEs in parent's data block numbers
      				}*/
					if(count==0 && dptr->rec_len==BLKSIZE) {	//first one and only one in that block				
						bdealloc(parent->dev, parent->INODE.i_block[i]);
						parent->INODE.i_size=parent->INODE.i_size-BLKSIZE;
						int j=0;
						for(j=i;j<11 && parent->INODE.i_block[j]!=0;j++) {
							parent->INODE.i_block[j]=parent->INODE.i_block[j+1];
						}
						/*  3. Write the parent's data block back to disk;
     					mark parent minode DIRTY for write-back
						*/
						put_block(parent->dev,parent->INODE.i_block[i],buffer);
						parent->dirty=1;
						return 0;						
					}
					/*  (1). if LAST entry in block{
                                         					|remove this entry   |
          					-----------------------------------------------------
          					xxxxx|INO rlen nlen NAME |yyy  |zzz                 | 
          					-----------------------------------------------------

                  					becomes:
          					-----------------------------------------------------
          					xxxxx|INO rlen nlen NAME |yyy (add zzz len to yyy)  |
          					-----------------------------------------------------

      					}
					*/
					else if(cp+dptr->rec_len>=&buffer[BLKSIZE]) {
						DIR *last=(DIR *)cp;
						DIR *pre=(DIR *)previous;
						pre->rec_len+=last->rec_len;
			//remove the last entry-everything AUTOMATICALLY becomes the garbage of namefield of the pre.						
						/*  3. Write the parent's data block back to disk;
     					mark parent minode DIRTY for write-back
						*/
						put_block(parent->dev,parent->INODE.i_block[i],buffer);
						parent->dirty=1;
						return 0;		
					}
					/*
  					(3). if in the middle of a block{
          				move all entries AFTER this entry LEFT;
          				add removed rec_len to the LAST entry of the block;
         				 no need to change parent's fileSize;

               					| remove this entry   |
          					-----------------------------------------------
          					xxxxx|INO rlen nlen NAME   |yyy  |zzz         | 
          					-----------------------------------------------

                  			becomes:
          					-----------------------------------------------
          					xxxxx|yyy |zzz (rec_len INC by rlen)          |
          					-----------------------------------------------
					      }
						*/
					else {
						rm_entrylen=dptr->rec_len;
						//current entry to be deleted is pointed by dptr
						DIR *next=cp+dptr->rec_len;
						move_entry_length=BLKSIZE-cover_len-rm_entrylen;
						//void *memcpy(void *dest, const void *source, size_t n)
						memcpy(dptr,next,move_entry_length);
						//find the last entry
						cp=buffer;
						dptr=(DIR*)buffer;
						while(cp+dptr->rec_len<&buffer[BLKSIZE]-rm_entrylen) {
							cp+=dptr->rec_len;
	    					dptr=(DIR *)cp;
						}
						dptr->rec_len+=rm_entrylen;
						/*  3. Write the parent's data block back to disk;
    					 mark parent minode DIRTY for write-back
						*/
						put_block(parent->dev,parent->INODE.i_block[i],buffer);
						parent->dirty=1;	
						return 0;					
					}
				}
    			dptr->name[dptr->name_len]=temp;
				previous=cp;
   	    		cp+=dptr->rec_len;
	    		dptr=(DIR *)cp;
				count++;//count the number of records in that i_block[i]
				cover_len+=dptr->rec_len;
		 	}
			
	}
}


