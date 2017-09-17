//write_cp_mv.c  mode = 0|1|2|3 for R|W|RW|APPEND
   
//write fd "added txt", pathname=fd, parameter="added txt", before this, open for w/wr mode
int write_file()
{
/*  1. Preprations:
     ask for a fd   and   a text string to write;
   
  2. verify fd is indeed opened for WR or RW or APPEND mode

  3. copy the text string into a buf[] and get its length as nbytes.

     return(mywrite(fd, buf, nbytes));
*/
	//1. ask for a fd   and   a text string to write;
    char buf[BLKSIZE];
	int nbytes=0;
    if(strcmp(pathname,"0") && strcmp(pathname,"1") && strcmp(pathname,"2") && strcmp(pathname,"3"))
    {
		printf("invalid fd %s\n",pathname);
		return -1;
	}  
	int inputfd=atoi(pathname);//mode = 0|1|2|3 for R|W|RW|APPEND 
	//2. verify fd is indeed opened for WR or RW or APPEND mode
	int i=0;
	if(running->fd[inputfd]->mode==0) {
		printf("fd = %d is NOT FOR WRITE\n",inputfd);
		return -1;
	}
	//3. copy the text string into a buf[] and get its length as nbytes.
	 strcpy(buf, parameter);
	 nbytes=strlen(buf);
	 return (mywrite(inputfd, buf, nbytes));
}

/*
The algorithm of write_file() can also be explained in terms of the following
figure.

(1).  PROC              (2).                          | 
     =======   |--> OFT oft[ ]                        |
     | pid |   |   ============                       |
     | cwd |   |   |mode=flags|                       | 
     | . ..|   |   |minodePtr ------->  minode[ ]     |      BlockDevice
     | fd[]|   |   |refCount=1|       =============   |   ==================
 fd: | .------>|   |offset    |       |  INODE    |   |   | INODE -> blocks|
     |     |       |===|======|       |-----------|   |   ==================
     =======           |              | dev,inode |   | 
                       |              |  dirty    |   |
                       |              =============   |
                       |
    -------------------|-----------------
    |    |    | ...  |lbk  | ............
    -------------------|-----------------
lbk   0    1 .....     |rem|            |
                     start           fileSize (in INODE)  
                        
------------------------------------------------------------------------------
               Data Structures for write()


mywrite behaves exactly the same as Unix's write(fd, buf, nbytes) syscall.
It writes nbytes from buf[ ] to the file descriptor fd, extending the file 
size as needed.
*/
//if not close explicitly, we can keep on writing. 
int mywrite(int fd, char *buf, int nbytes) 
{
    char *cq=buf;
	char *cp;
    int blk=0,lbk=0,startByte=0, remain=0;
	int nbytes_copy=0;
	char wbuf[BLKSIZE];
    while (nbytes > 0){		 
    /* compute LOGICAL BLOCK (lbk) and the startByte in that lbk:

          lbk       = oftp->offset / BLKSIZE;
          startByte = oftp->offset % BLKSIZE;
	*/
	 lbk=running->fd[fd]->offset/BLKSIZE;
	 startByte = running->fd[fd]->offset % BLKSIZE;
	
    // I only show how to write DIRECT data blocks, you figure out how to 
    // write indirect and double-indirect blocks.

     if (lbk < 12){                         // direct block
        if (running->fd[fd]->mptr->INODE.i_block[lbk] == 0){   // if no data block yet

           running->fd[fd]->mptr->INODE.i_block[lbk] = balloc(running->fd[fd]->mptr->dev);// MUST ALLOCATE a block

           // write a block of 0's to blk on disk: OPTIONAL for data block ?????
           //                                      but MUST for I or D blocks
		   
        }
        blk = running->fd[fd]->mptr->INODE.i_block[lbk];      // blk should be a disk block now
     }
     else if (lbk >= 12 && lbk < 256 + 12){ 
            // indirect blocks
			int indirect_buf[256];
			int k=0;
			if(running->fd[fd]->mptr->INODE.i_block[12]==0) {
				running->fd[fd]->mptr->INODE.i_block[12]=balloc(running->fd[fd]->mptr->dev);
				memset(indirect_buf,0,1024);		
				put_block(running->fd[fd]->mptr->dev,running->fd[fd]->mptr->INODE.i_block[12],indirect_buf);
			}			
			get_block(running->fd[fd]->mptr->dev,running->fd[fd]->mptr->INODE.i_block[12],indirect_buf);

			if(indirect_buf[lbk-12]==0) {
				indirect_buf[lbk-12]=balloc(running->fd[fd]->mptr->dev);
				
				put_block(running->fd[fd]->mptr->dev,running->fd[fd]->mptr->INODE.i_block[12],indirect_buf);
			}

			blk = indirect_buf[lbk-12]; 
			
     }
     else{
            // double indirect blocks */  
			int count=lbk-12-256;
			int num=count/256;//position in double_buf1,after this, all = 0
			int pos_offset=count%256;//position in double_buf2, after this, all=0
			int double_buf1[256],double_buf2[256];
			if(running->fd[fd]->mptr->INODE.i_block[13]==0) {
				running->fd[fd]->mptr->INODE.i_block[13]=balloc(running->fd[fd]->mptr->dev);
				memset(double_buf1,0,1024);
				put_block(running->fd[fd]->mptr->dev,running->fd[fd]->mptr->INODE.i_block[13],double_buf1);
			}
			get_block(running->fd[fd]->mptr->dev,running->fd[fd]->mptr->INODE.i_block[13],double_buf1);

			if(double_buf1[num]==0) {
				double_buf1[num]=balloc(running->fd[fd]->mptr->dev);
				memset(double_buf2,0,1024);
				put_block(running->fd[fd]->mptr->dev,double_buf1[num],double_buf2);
				put_block(running->fd[fd]->mptr->dev,running->fd[fd]->mptr->INODE.i_block[13],double_buf1);
			}
			get_block(running->fd[fd]->mptr->dev,double_buf1[num],double_buf2);
			if(double_buf2[pos_offset]==0) {
				double_buf2[pos_offset]=balloc(running->fd[fd]->mptr->dev);
				put_block(running->fd[fd]->mptr->dev,double_buf1[num],double_buf2);
				
			}
			blk=double_buf2[pos_offset];


     }	 
     /* all cases come to here : write to the data block */
     get_block(running->fd[fd]->mptr->dev, blk, wbuf);   // read disk block into wbuf[ ]
     cp = wbuf + startByte;      // cp points at startByte in wbuf[]
     remain = BLKSIZE - startByte;     // number of BYTEs remain in this block
/*
                OPTIMIZATION OF write Code

As in read(), the above inner while(remain > 0) loop can be optimized:
Instead of copying one byte at a time and update the control variables on each 
byte, TRY to copy only ONCE and adjust the control variables accordingly.

REQUIRED: Optimize the write() code in your project.
*/
     while (remain > 0){   
		   int min=0;            // write as much as remain allows
		   if(nbytes<=remain) 
		      min=nbytes;
		   else
		      min=remain;
		   memcpy(cp,cq, nbytes);
		   nbytes_copy+=min;
           nbytes-=min; 
		   remain-=min;         // dec counts
           running->fd[fd]->offset+=min;             // advance offset
           if (running->fd[fd]->offset > running->fd[fd]->mptr->INODE.i_size)  // especially for RW|APPEND mode
               running->fd[fd]->mptr->INODE.i_size+=min;    // inc file size (if offset > fileSize)
           if (nbytes <= 0) 
		   	   break;     // if already nbytes, break
     }
     put_block(running->fd[fd]->mptr->dev, blk, wbuf);   // write wbuf[ ] to disk
      
     // loop back to while to write more .... until nbytes are written
  }

  running->fd[fd]->mptr->dirty = 1;       // mark mip dirty for iput() 
  printf("wrote %d char into file descriptor fd=%d\n", nbytes_copy, fd);           
  return nbytes_copy;
}


/*=============================================================================

                      HOW TO cp ONE file:

cp src dest: pathname=src, parameter=dest

1. fd = open src for READ;

2. gd = open dst for WR|CREAT; 

   NOTE:In the project, you may have to creat the dst file first, then open it 
        for WR, OR  if open fails due to no file yet, creat it and then open it
        for WR.

3. while( n=read(fd, buf[ ], BLKSIZE) ){
       write(gd, buf, n);  // notice the n in write()
   }

mode = 0|1|2|3 for R|W|RW|APPEND
*/
//open f1 0
int cp_file() {
	int n=0;
	char buf[BLKSIZE];
	char dest_parameter_copy[256];
	char src_pathname_copy[256];
	strcpy(src_pathname_copy, pathname);
	strcpy(dest_parameter_copy,parameter);
//1. fd = open src for READ; open src 0
	strcpy(parameter, "0");
	int src_fd=open_file();
	if(src_fd==-1) {
		printf("bad SRC file %s\n",pathname);
	}
//2. gd = open dst for WR(2)|CREAT;    creat_file(): creat file1
/*   NOTE:In the project, you may have to creat the dst file first, then open it 
        for WR, OR  if open fails due to no file yet, creat it and then open it
        for WR.
*/
	int dest_dev=0;
	if (dest_parameter_copy[0]=='/') 
		dest_dev = root->dev;          // root INODE's dev
    else                  
		dest_dev = running->cwd->dev; 
	int dest_ino=getino(&dest_dev,dest_parameter_copy);
	//if no such file, go creat it
	if(dest_ino==0) {
		strcpy(pathname,dest_parameter_copy);
		creat_file();
	}
	//open dest 2
	strcpy(pathname, dest_parameter_copy);
	strcpy(parameter, "2");
	int des_gd=open_file();		
/*
3. while( n=read(fd, buf[ ], BLKSIZE) ){
       write(gd, buf, n);  // notice the n in write()
   }
*/ 
	while( n=myread(src_fd, buf, BLKSIZE,0) ){
       mywrite(des_gd, buf, n);  // notice the n in write()
    }
    close_file(src_fd);
	close_file(des_gd);
	return 0;
}
/*==============================================================================

                    HOW TO mv (rename)
mv src dest: pathname=src, parameter=dest

1. verify src exists; get its INODE in ==> you already know its dev

2. check whether dest is on the same dev as src

              CASE 1: same dev:
3. Hard link dst with src (i.e. same INODE number)
4. unlink src (i.e. rm src name from its parent directory and reduce INODE's
               link count by 1).
                
              CASE 2: not the same dev:
3. cp src to dst
4. unlink src*/
int mv_file() {
	int src_dev=0;
//1. verify src exists; get its INODE in ==> you already know its dev
	if (pathname[0]=='/') 
		src_dev = root->dev;          // root INODE's dev
    else                  
		src_dev = running->cwd->dev;  
	int src_ino=getino(&src_dev, pathname);
	if(src_ino==0) {
		printf("the source file doesn't exist.\n");
		return -1;
	}
	MINODE *src_mip=iget(src_dev,src_ino);
	
//2. check whether dest is on the same dev as src
	int dest_dev=0;
	if (parameter[0]=='/') 
		dest_dev = root->dev;          // root INODE's dev
    else                  
		dest_dev = running->cwd->dev;  
/*
CASE 1: same dev:
3. Hard link dst with src (i.e. same INODE number)
*/
	if(dest_dev==src_dev) {
		link();
	}
/*                
CASE 2: not the same dev:
3. cp src to dst */
	else {
		cp_file();
	}	
//4. unlink src
	strcpy(parameter,"");
	unlink();
	return 0;
}
        

