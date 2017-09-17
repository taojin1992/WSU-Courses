
//1. link oldFileName newFileName
/*
creates a file newFileName which has the SAME inode (number) as that of
   oldFileName.


   Example: link     /a/b/c                      /x/y/z ==>
                     /a/b/   datablock           /x/y    data block
                 ------------------------        -------------------------
                .. .|ino rlen nlen c|...        ....|ino rlen nlen z| ....
                ------|-----------------        ------|------------------
                      |                               |
                    INODE <----------------------------

                i_links_count = 1  <== INCrement i_links_count to 2
*/
void link() {
/*
(1). get the INODE of /a/b/c into memory: mip->minode[ ]
                                               INODE of /a/b/c
                                               dev,ino
                                               .......
*/
	char newpathname[256];//compare the link of pathname
	strcpy(newpathname,parameter);
	if (pathname[0]=='/') 
		dev = root->dev;          // root INODE's dev
    else                  
		dev = running->cwd->dev;  
	int inumber=getino(&dev, pathname);
	if(inumber==0) {
		printf("Error: the file to link doesn't exist\n");
		return;
	}
	MINODE *mip=iget(dev, inumber);	
	printf("before the link, link count of the original file=%d\n",mip->INODE.i_links_count);
	char pathname_copy[256];
	char pathname_copy1[256];
	strcpy(pathname_copy,pathname);
	strcpy(pathname_copy1,pathname);
	char *parent,*child;
	parent=dirname(pathname_copy);//dirname, basename will destroy the string passed in 
	child=basename(pathname_copy1);
	
	char newpathname_copy[256];
	char newpathname_copy1[256];
	strcpy(newpathname_copy,newpathname);
	strcpy(newpathname_copy1,newpathname);
	char *newparent,*newchild;
	newparent=dirname(newpathname_copy);//dirname, basename will destroy the string passed in 
	newchild=basename(newpathname_copy1);
	
//(2). check /a/b/c is a REG or LNK file (link to DIR is NOT allowed).
	if(S_ISDIR(mip->INODE.i_mode)) {
		printf("link to DIR is NOT allowed\n");
		return;
	}
	else {
//(3). check /x/y  exists and is a DIR but 'z' does not yet exist in /x/y/
		int newpino=getino(&dev,newparent);
		MINODE *newpip=iget(dev, newpino);
		if(!S_ISDIR(newpip->INODE.i_mode)) {
			printf("cannot link because the parent dir doesn't exist\n");
			return;
		}
		int newchildino=search(newpip, newchild);
		if(newchildino!=0) {
			printf("link : %s already exists in dir\n",newchild);
			return;
		}
		if (newpip->dev!=mip->dev) {
			printf("Error: cannot create link between two different devices!\n");
			return;
		}
		if (S_ISDIR(newpip->INODE.i_mode) && newchildino==0) {
			/*(4). Add an entry [ino rec_len name_len z] to the data block of /x/y/
     				This creates /x/y/z, which has the SAME ino as that of /a/b/c
					(NOTE: both /a/b/c and /x/y/z must be on the SAME device; 
       				link can not be across different devices).
			*/
			/* enter name ENTRY into parent's directory by 
            enter_name(pip, ino, name);*/
  			int sign=enter_name(newpip,inumber,newchild);
			
			//(5). increment the i_links_count of INODE by 1
			mip->INODE.i_links_count++;
			mip->dirty=1;
			printf("After the link, link count of the original file=%d\n",mip->INODE.i_links_count);
			//(6). write INODE back to disk 
	   		iput(mip);
			iput(newpip);
		}
	}
}


//unlink pathname
//-unlink decrements the file's links_count by 1 and deletes the file name from its parent DIR.
//when a file's links_count==0, the file is truly removed by deallocating its data blocks and inode.

//Write a truncate(INODE) function, which deallocates ALL the data blocks
//of INODE. This is similar to printing the data blocks of INODE.
int truncate(MINODE *mip)
{
  int i=0;
  if(!S_ISLNK(mip->INODE.i_mode)) {
	printf("****************  DATA BLOCKS TO BE DEALLOCATED*******************\n");
    for(i=0;mip->INODE.i_block[i]!=0 && i<14;i++) {
		printf("i_block[%d]=%d\n",i,mip->INODE.i_block[i]);				
	}		
	printf("================ DEALLOCATING Direct Blocks ===================\n");
	int m=0;
	for(m=0;m<12 && mip->INODE.i_block[m]!=0;m++) {
		//printf("%d  ",inodePtr->i_block[m]);
		bdealloc(dev, mip->INODE.i_block[m]);			
	}
	printf("\n");
	if(mip->INODE.i_block[12]!=0) {
		printf("===============  DEALLOCATING Indirect blocks   ===============\n");
		int buffer1[256];
		get_block(fd,mip->INODE.i_block[12],buffer1);
		int n=0;
		while(buffer1[n]!=0 && n<256){
				//printf("%d  ",buffer1[n]);
				bdealloc(dev, buffer1[n]);
				n++;
		} 
        printf("\n");    			
	}
	if(mip->INODE.i_block[13]!=0) {
					
     	printf("===============  DEALLOCATING Double Indirect blocks   ===============\n");
		int buffer2[256];
		get_block(fd,mip->INODE.i_block[13],buffer2);
		int buffer3[256];
		int index=0;
	 	while(buffer2[index]!=0) {
			get_block(fd,buffer2[index],buffer3);
			int t=0;
			while(buffer3[t]!=0){
				//printf("%d  ",buffer3[t]);
				bdealloc(dev, buffer3[t]);
				t++;
			}
			index++; 
		}						
	}
	mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);  // set to current time,update time field
	mip->INODE.i_size=0;//set i_size to be 0 and mark it dirty
	mip->dirty=1;
	for(i=0;i<14;i++) {
		if(mip->INODE.i_block[i]!=0)
			return 1;
		else 
			return 0;
	}
  }
  else{
	printf("deallocating all the char content in the 15 datablocks\n");
 	for(i=0;i<15;i++) {
		mip->INODE.i_block[i]=0;
	}
	mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);  // set to current time,update time field
	mip->INODE.i_size=0;//set i_size to be 0 and mark it dirty
	mip->dirty=1;
	for(i=0;i<15;i++) {
		if(mip->INODE.i_block[i]!=0)
			return 1;
		else 
			return 0;
	}
	
  }
}

//removal of symlink is also included
//if the file has other links, rm from the parent too,but keep the inode
void unlink() {
	char pathname_copy[256];
	strcpy(pathname_copy,pathname);
	char pathname_copy1[256];
	strcpy(pathname_copy1,pathname);
	char *child=basename(pathname_copy);
	char *parent=dirname(pathname_copy1);
//(1). get pathname's INODE into memory
	if (pathname[0]=='/') 
		dev = root->dev;          // root INODE's dev
    else                  
		dev = running->cwd->dev;  
	int ino=getino(&dev, pathname);// ino=>the ino of link to remove, which is the same with the file it points to
	if(ino==0) {
		printf("Error: the link to remove doesn't exist\n");
		return;
	}
	int pino=getino(&dev, parent);
	MINODE *mip=iget(dev,ino);//get the inode of the link/file into memory
	printf("Before the unlink, link count of the original file %s=%d\n",pathname,mip->INODE.i_links_count);
	MINODE *pmip=iget(dev,pino);
//(2). verify it's a FILE (REG or LNK), can not be a DIR; 
	if(S_ISDIR(mip->INODE.i_mode)) {
		iput(mip);
		iput(pmip);
		printf("unlink to DIR is NOT allowed\n");
		return;
	}
//(3). decrement INODE's i_links_count by 1;
	mip->INODE.i_links_count--;
//(5)remove the basename from parent dir    rm_child(MINODE *parent, char *name)
/*Remove childName = basename(pathname) from the parent directory by

        rm_child(parentInodePtr, childName)

     which is the SAME as that in rmdir or unlink file operations.
*/
//******if unlink the original file(still with some file linking to it), just remove it from its parent.
	rm_child(pmip,child);
	pmip->dirty=1;
	iput(pmip); 
	if(mip->INODE.i_links_count>0) {
		mip->dirty=1;
		printf("After the unlink, link count of the original file=%d\n",mip->INODE.i_links_count);
		iput(mip);
		return;
	}

/*(4). if i_links_count == 0 ==> rm pathname by

        deallocate its data blocks by:

     Write a truncate(INODE) function, which deallocates ALL the data blocks
     of INODE. This is similar to printing the data blocks of INODE.

        deallocate its INODE;
*/
	if(!S_ISLNK(mip->INODE.i_mode)&&(mip->INODE.i_links_count==0)) { //assume LINK HAS no data block
		printf("deallocate its data blocks\n");
		int sign=truncate(mip);//deallocate all the data blocks.Note:indirect and double indirect
		if(sign==0) {
			printf("deallocate all the data blocks successfully\n");
		}
 		else {
			printf("Cannot deallocate all the data blocks successfully\n");
		}
		printf("deallocate its inode\n");
		idealloc(dev, mip->ino);
		printf("After the link, link count of the original file=%d\n",mip->INODE.i_links_count);
		iput(mip);
		return;
	}
	if(S_ISLNK(mip->INODE.i_mode)&&(mip->INODE.i_links_count==0)) { //assume LINK HAS no data block
		printf("deallocate its data blocks\n");
		int sign=truncate(mip);//deallocate all the data blocks.Note:indirect and double indirect
		if(sign==0) {
			printf("deallocate all the data blocks successfully\n");
		}
 		else {
			printf("Cannot deallocate all the data blocks successfully\n");
		}
		printf("deallocate its inode\n");
		idealloc(dev, mip->ino);
		printf("After the link, link count of the original file=%d\n",mip->INODE.i_links_count);
		iput(mip);
		return;
	}
	
}




