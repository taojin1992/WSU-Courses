/*
================ development HELPS =============================

1. Your mkdir/creat may trash the disk iamge (by writing to wrong inode or data
   blocks), which will cause problems later even if your program is correct. So,
   it's better to use a FRESH disk image each time during development.

   Write a sh script "run" or "mkdisk" containing: 

         mkfs -b 1024 mydisk 1440  # renew the disk image file
         ./a.out

   Enter run or mkdisk to test YOUR new a.out, so that you use a NEW disk image 
   file each time until YOUR a.out no longer traches the disk image.

2. After running YOUR mkdir/creat commands, it's better to check the results 
   under LINUX. Write a sh script "s" containing
       
         mount -o loop mydisk /mnt
         ls -l /mnt
         umount /mnt

   so that s will show the disk contents under LINUX.
*/
#include <time.h>
//Assume: command line = "mkdir pathname" OR  "creat pathname"

void make_dir()
{
	char *parent, *child;
	char pathname_copy[256];
	char pathname_copy1[256];
	int pino=0,result=0;
/*1. pahtname = "/a/b/c" start mip = root;         dev = root->dev;
            =  "a/b/c" start mip = running->cwd; dev = running->cwd->dev;*/
	MINODE *mip;
	if(pathname[0]=='/') {
		mip=root;
		dev=root->dev;
	}
	else {
		mip=running->cwd;
		dev=running->cwd->dev;
	}
/*
2. Let  
     parent = dirname(pathname);   parent= "/a/b" OR "a/b"
     child  = basename(pathname);  child = "c"
*/
	strcpy(pathname_copy,pathname);
	strcpy(pathname_copy1,pathname);
    parent=dirname(pathname_copy1);//dirname, basename will destroy the string passed in 
	child=basename(pathname_copy);
	pino=getino(&dev, parent);
/*	
3. Get the In_MEMORY minode of parent:

         pino  = getino(&dev, parent);
         pip   = iget(dev, pino); 

   Verify : (1). parent INODE is a DIR (HOW?)   AND
            (2). child does NOT exists in the parent directory (HOW?):use search(pip, childname);
*/
	
	if (pino==0) {
		printf("invalid dir pathname\n");
		return;
	}
	MINODE *pip=iget(dev, pino);
	if(!S_ISDIR(pip->INODE.i_mode)) {
		printf("mode=%x %s is not a DIR\n",pip->INODE.i_mode, parent);
		iput(pip);
		return;
	}
	int childino=search(pip, child);
	if(childino!=0) {
		printf("mkdir : %s already exists in dir\n",child);
		iput(pip);
		return;
	}
	else {
		printf("%s does not yet exist\n",child);
	}

	/*         
	4. call mymkdir(pip, child);
	*/
	int success=mymkdir(pip,child);//0 success, -1 fail
	
	if(success==0) {
	/*
	5. inc parent inodes's link count by 1; 
   touch its atime and mark it DIRTY*/
		pip->INODE.i_links_count++;
		pip->INODE.i_atime=time(0L);//update the access time
		pip->dirty=1;
	/*
	6. iput(pip);
	*/
		result=search(pip, child);
		iput(pip);
    }
	else {
		printf("mkdir failed\n");
		return;
	}    
} 

int mymkdir(MINODE *pip, char *name)
{
//1. pip points at the parent minode[] of "/a/b", name is a string "c") 

/*2. allocate an inode and a disk block for the new directory;
        ino = ialloc(dev);    
        bno = balloc(dev);
*/
	 int ino = ialloc(pip->dev);    
     int bno = balloc(pip->dev);
	 printf("ialloc: ino=%d\n",ino);
	 printf("balloc: bno=%d\n",bno);
	 printf("making INODE\n");

/*
3. mip = iget(dev, ino) to load the inode into a minode[] (in order to
   wirte contents to the INODE in memory).
*/
	MINODE *mip=iget(pip->dev, ino);

/*
4. Write contents to mip->INODE to make it as a DIR.
5. iput(mip); which should write the new INODE out to disk.
*/
  INODE *ip = &mip->INODE;//Use ip-> to acess the INODE fields:

  ip->i_mode = 0x41ED;		// OR 040755: DIR type and permissions
  ip->i_uid  = running->uid;	// Owner uid 
  //ip->i_gid  = running->gid;	// Group Id
  ip->i_size = BLKSIZE;		// Size in bytes 
  ip->i_links_count = 2;	        // Links count=2 because of . and ..
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
  ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
  ip->i_block[0] = bno;             // new DIR has one data block  
  int index=0;
  for(index=1;index<=14;index++) {
	 ip->i_block[index]=0;
  }  
  
  mip->dirty = 1;               // mark minode dirty
  iput(mip);                    // write INODE to disk

//***** create data block for new DIR containing . and .. entries ******
/*
6. Write . and .. entries into a buf[ ] of BLKSIZE

   | entry .     | entry ..                                             |
   ----------------------------------------------------------------------
   |ino|12|1|.   |pino|1012|2|..                                        |
   ----------------------------------------------------------------------

   Then, write buf[ ] to the disk block bno;
*/
  printf("making data block ......\n"); 
  char BUF[BLKSIZE];
  get_block(dev,bno,BUF);
  char *cp=BUF;	
  DIR   *dptr=(DIR*) BUF;
  dptr->inode=ino;
  dptr->name_len=1;
  dptr->rec_len=12;
  strncpy(dptr->name,".",1);
  cp=cp+dptr->rec_len;
  dptr=(DIR*)cp; 
  dptr->inode=pip->ino;
  dptr->name_len=2;
  dptr->rec_len=BLKSIZE-12;
  strncpy(dptr->name,"..",2);
  printf("writing data block %d to disk\n",bno);
  put_block(dev,bno,BUF);
/*
7. Finally, enter name ENTRY into parent's directory by 
            enter_name(pip, ino, name);
*/
 int sign=enter_name(pip,ino,name);
 return sign;
}

int enter_name(MINODE *pip, int myino, char *myname)
{
  printf("Enter:  parent = (%d %d)  name = %s\n",pip->dev,myino,myname);
  printf("---------- ENTER_NAME ------------\n");
  char *cp;
  char buffer[BLKSIZE];
  DIR *dptr;
  //For each data block of parent DIR do  // assume: only 12 direct blocks
  int i=0;
  for(i=0;i<12;i++) {
	if (pip->INODE.i_block[i]==0) {
		break;
	}
	//(1). get parent's data block into a buf[];
	get_block(pip->dev, pip->INODE.i_block[i], buffer);
   
	/*(2). EXT2 DIR entries: Each DIR entry has rec_len and name_len. Each entry's
     ideal length is   
                     IDEAL_LEN = 4*[ (8 + name_len + 3)/4 ]. 
     All DIR entries in a data block have rec_len = IDEAL_LEN, except the last
     entry. The rec_len of the LAST entry is to the end of the block, which may
     be larger than its IDEAL_LEN.

  --|-4---2----2--|----|---------|--------- rlen ->------------------------|
    |ino rlen nlen NAME|.........|ino rlen nlen|NAME                       |
  --------------------------------------------------------------------------
	*/
	int n_len=strlen(myname);
	/*(3). To enter a new entry of name with n_len, the needed length is
        need_length = 4*[ (8 + n_len + 3)/4 ]  // a multiple of 4
	*/
	int need_length = 4*((8 + n_len + 3)/4);	
	printf("need_len for %s = %d\n",myname,need_length);
	//(4). Step to the last entry in a data block (HOW?). 
    dptr = (DIR *)buffer;
    char *cp = buffer;
    // step to LAST entry in block: 
	int blk = pip->INODE.i_block[i];
	printf("parent data blk[%d] = %d\n",i,blk);
    printf("step to LAST entry in parent data block %d\n", blk);
    char temp;
    while (cp + dptr->rec_len < buffer + BLKSIZE){
          // print DIR entries to see what they are
		  temp=dptr->name[dptr->name_len];
		  dptr->name[dptr->name_len]='\0';		
		  printf("%s  ",dptr->name);
		  dptr->name[dptr->name_len]=temp;
          cp += dptr->rec_len;
          dptr = (DIR *)cp;
     } 
      // dp NOW points at last entry in block, 26 960 5 test1
	 temp=dptr->name[dptr->name_len];
	 dptr->name[dptr->name_len]='\0';		
	 printf("%s\n",dptr->name);
	 dptr->name[dptr->name_len]=temp;
     //Let remain = LAST entry's rec_len - its IDEAL_LENGTH;
	 int last_ideal_length = 4*((8 + dptr->name_len + 3)/4);
	 int remain=dptr->rec_len-last_ideal_length;
     if (remain >= need_length)	{
		printf("found enough space: ");
	  	temp=dptr->name[dptr->name_len];
		dptr->name[dptr->name_len]='\0';		
		printf("name=%s ideal=%d rlen=%d remain=%d\n",dptr->name,last_ideal_length,dptr->rec_len,remain);
		dptr->name[dptr->name_len]=temp;//dptr points to the last entry
		dptr->rec_len=last_ideal_length;
        /*enter the new entry as the LAST entry and trim the previous entry
        to its IDEAL_LENGTH;*/
/*
                          EXAMPLE:

                                 |LAST entry 
  --|-4---2----2--|----|---------|--------- rlen ->------------------------|
    |ino rlen nlen NAME|.........|ino rlen nlen|NAME                       |
  --------------------------------------------------------------------------
                                                    |     NEW entry
  --|-4---2----2--|----|---------|----ideal_len-----|--- rlen=remain ------|
    |ino rlen nlen NAME|.........|ino rlen nlen|NAME|myino rlen nlen myname|
  --------------------------------------------------------------------------
*/
		cp+=last_ideal_length;
		dptr=(DIR*)cp;
		dptr->inode=myino;
  		dptr->name_len=strlen(myname);
  		dptr->rec_len=remain;
  		strcpy(dptr->name,myname);
		//print all the records
/*		cp=buffer;
		dptr=(DIR*)buffer;
		while (cp < buffer + BLKSIZE){		  
		  temp=dptr->name[dptr->name_len];
		  dptr->name[dptr->name_len]='\0';		
		  printf("%d %d %d %s\n",dptr->inode, dptr->rec_len,dptr->name_len,dptr->name);
		  dptr->name[dptr->name_len]=temp;
          cp += dptr->rec_len;
          dptr = (DIR *)cp;

     	} */
        //(6).Write data block to disk;
		put_block(pip->dev, pip->INODE.i_block[i],buffer);	
		printf("writing back parent data block[%d]= %d\n",i,pip->INODE.i_block[i]);
		printf("done enter_name()\n");
		return 0;
     } 

}
/*
	(5).// Reach here means: NO space in existing data block(s)

  Allocate a new data block; INC parent's isze by 1024;
  Enter new entry as the first entry in the new data block with rec_len=BLKSIZE.

  |-------------------- rlen = BLKSIZE -------------------------------------
  |myino rlen nlen myname                                                  |
  --------------------------------------------------------------------------
*/
	//work on pip->INODE.i_block[i], which is 0 now
	printf("Cannot find enough space. Allocate a new data block.\n");
	int newbno = balloc(pip->dev);
	if(newbno==0) {
		printf("Cannot allocate any new data block.\n");
		return -1;
	}
	pip->INODE.i_block[i]=newbno;
	pip->INODE.i_size=pip->INODE.i_size+BLKSIZE;
	get_block(pip->dev, pip->INODE.i_block[i], buffer);
	cp=buffer;
	dptr=(DIR*)buffer;
	dptr->inode=myino;
  	dptr->name_len=strlen(myname);
  	dptr->rec_len=BLKSIZE;
  	strcpy(dptr->name,myname);
	//(6).Write data block to disk;
	put_block(pip->dev, pip->INODE.i_block[i],buffer);	
	printf("writing parent data block back %d %d\n",i,pip->INODE.i_block[i]);
	printf("done enter_name()\n");
	return 0; 
}        


void creat_file()
{
/*  This is ALMOST THE SAME AS mkdir() except : 
   (1). its inode's mode field is set to a REGULAR file, 
        permission bits to (default) rw-r--r--, 
   (2). No data block, so size = 0
   (3). links_count = 1;
   (4). Do NOT increment parent's links_count
*/
	char *parent, *child;
	char pathname_copy[256];
	char pathname_copy1[256];
    int pino=0,result=0;
/*1. pahtname = "/a/b/c" start mip = root;         dev = root->dev;
            =  "a/b/c" start mip = running->cwd; dev = running->cwd->dev;*/
	MINODE *mip;
	if(pathname[0]=='/') {
		mip=root;
		dev=root->dev;
	}
	else {
		mip=running->cwd;
		dev=running->cwd->dev;
	}
/*
2. Let  
     parent = dirname(pathname);   parent= "/a/b" OR "a/b"
     child  = basename(pathname);  child = "c"
*/
	strcpy(pathname_copy,pathname);
	strcpy(pathname_copy1,pathname);
    parent=dirname(pathname_copy);//dirname, basename will destroy the string passed in 
	child=basename(pathname_copy1);

/*	
3. Get the In_MEMORY minode of parent:

         pino  = getino(&dev, parent);
         pip   = iget(dev, pino); 

   Verify : (1). parent INODE is a DIR (HOW?)   AND
            (2). child does NOT exists in the parent directory (HOW?):use search(pip, childname);
*/
	pino=getino(&dev, parent);
	if (pino==0) {
		printf("invalid dir pathname\n");
		return;
	}
	MINODE *pip=iget(dev, pino);
	if(!S_ISDIR(pip->INODE.i_mode)) {
		printf("mode=%x %s is not a DIR\n",pip->INODE.i_mode, parent);
		iput(pip);
		return;
	}
	int childino=search(pip, child);
	if(childino!=0) {
		printf("creat : %s already exists in dir\n",child);
		iput(pip);
		return;
	}
	else {
		printf("%s does not yet exist\n",child);
	}

/*         
4. call my_creat(pip, child);
*/
	int success=my_creat(pip,child);//0 success, -1 fail
	if(success==0) {
	/*
	5. no need to inc parent inodes's link count by 1; 
   touch its atime and mark it DIRTY*/
		pip->INODE.i_atime=time(0L);//update the access time
		pip->dirty=1;
	/*
	6. iput(pip);
	*/
		result=search(pip, child);
		iput(pip);
	}
	else {
		printf("creat failed\n");
		iput(pip);
		return;
	}    
} 


int my_creat(MINODE *pip, char *name)
{
/* 	Same as mymkdir() except 
    INODE's file type = 0x81A4 OR 0100644
    links_count = 1
    NO data block, so size = 0
    do NOT inc parent's link count.
*/
	//1. pip points at the parent minode[] of "/a/b", name is a string "c") 

/*2. allocate an inode and a disk block for the new directory;
        ino = ialloc(dev);    
        bno = balloc(dev);
*/
	 int ino = ialloc(pip->dev);  
	 printf("ialloc: ino=%d\n",ino);  
/*
3. mip = iget(dev, ino) to load the inode into a minode[] (in order to
   wirte contents to the INODE in memory).
*/
	MINODE *mip=iget(pip->dev, ino);
	printf("creat : dev = %d  ino = %d\n",pip->dev,ino);
/*
4. Write contents to mip->INODE to make it as a DIR.
5. iput(mip); which should write the new INODE out to disk.
*/
  INODE *ip = &mip->INODE;//Use ip-> to acess the INODE fields:

  ip->i_mode = 0x81A4;		// INODE's file type = 0x81A4 OR 0100644
  ip->i_uid  = running->uid;	// Owner uid 
  //ip->i_gid  = running->gid;	// Group Id
  ip->i_size = 0;		// Size in bytes 
  ip->i_links_count = 1;	        
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
  ip->i_blocks = 0;                	// LINUX: Blocks count in 512-byte chunks ?????/2 =ON
  int index=0;
  for(index=0;index<=14;index++) {
	 ip->i_block[index]=0;
  }  
  mip->dirty = 1;               // mark minode dirty
  iput(mip);                    // write INODE to disk
/*
7. Finally, enter name ENTRY into parent's directory by 
            enter_name(pip, ino, name);
*/
  int sign=enter_name(pip,ino,name);
  return sign;
}  




