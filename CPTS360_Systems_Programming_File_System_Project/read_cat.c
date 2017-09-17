//read_cat.c
//read fd 100    pathname=fd, parameter=100
int read_file()
{
/*  Preparations: 
    ASSUME: file is opened for RD or RW;
    ask for a fd  and  nbytes to read;
    verify that fd is indeed opened for RD or RW;
    return(myread(fd, buf, nbytes));
*/
	char buf[BLKSIZE];
	if(strcmp(pathname,"0") && strcmp(pathname,"1") && strcmp(pathname,"2") && strcmp(pathname,"3"))
    {
		printf("invalid fd %s\n",pathname);
		return -1;
	}  
	int inputfd=atoi(pathname);//mode = 0|1|2|3 for R|W|RW|APPEND 
	if(inputfd!=0 && inputfd!=2) {
		printf("fd = %d is NOT FOR READ\n",inputfd);
		return -1;
	}
	int nbyte_to_read=atoi(parameter);
	if (nbyte_to_read==0 && strcmp(parameter,"0")!=0) {
		printf("bad nbytes\n");
		return -1;
	}
	return (myread(inputfd, buf, nbyte_to_read,1));
}

/*it behaves EXACTLY the same as the read() system call in Unix/Linux. 
The algorithm of myread() can be best explained in terms of the following 
diagram.
int myread(int fd, char buf[ ], nbytes) {

(1).  PROC              (2).                          | 
     =======   |--> OFT oft[ ]                        |
     | pid |   |   ============                       |
     | cwd |   |   |mode=flags|                       | 
     | . ..|   |   |minodePtr ------->  minode[ ]     |      BlockDevice
     | fd[]|   |   |refCount=1|       =============   |   ==================
 fd: | .------>|   |offset    |       |  INODE    |   |   | INODE -> blocks|
     |     |       |===|======|       |-----------|   |   ==================
     =======           |              |  dev,ino  |   |
                       |              =============   |
                       |
                       |<------- avil ------->|
    -------------------|-----------------------
    |    |    | ...  |lbk  |   |  ...| .......|
    -------------------|---|------------------|-
lbk   0    1 .....     |rem|                   |
                     start                   fsize  
                        
------------------------------------------------------------------------------
                 Data structures for reading file

(1). Assume that fd is opened for READ. 

(2). The offset in the OFT points to the current byte position in the file from
     where we wish to read nbytes. 
(3). To the kernel, a file is just a sequence of contiguous bytes, numbered from
     0 to file_size - 1. As the figure shows, the current byte position, offset
     falls in a LOGICAL block (lbk), which is 
               lbk = offset / BLKSIZE 

     the byte to start read in that logical block is 
             start = offset % BLKSIZE 

     and the number of bytes remaining in the logical block is 
             remain = BLKSIZE - start. 

     At this moment, the file has 
             avil = file_size - offset 

     bytes available for read. 

     These numbers are used in the read algorithm.


(4). myread() behaves exactly the same as the read(fd, buf, nbytes) syscall of 
     Unix/Linux. It tries to read nbytes from fd to buf[ ], and returns the 
     actual number of bytes read.

(5). ============ Algorithm and pseudo-code of myread() =======================
*/
int myread(int fd, char *buf, int nbytes,int print_info_mark)
{
// 1. int count = 0;
//    avil = fileSize - OFT's offset // number of bytes still available in file.
//    char *cq = buf;                // cq points at buf[ ]
	int count_read=0;
	char *cq=buf;
	int avil=running->fd[fd]->mptr->INODE.i_size-running->fd[fd]->offset;
    int blk=0,lbk=0,startByte=0,remain=0;
	if(print_info_mark>0)
		printf("********* read file %d  %d *********\n",fd,nbytes);
	while (nbytes && avil){
       /*Compute LOGICAL BLOCK number lbk and startByte in that block from offset;

             lbk       = oftp->offset / BLKSIZE;
             startByte = oftp->offset % BLKSIZE;*/
	   lbk=running->fd[fd]->offset/BLKSIZE;
	   startByte = running->fd[fd]->offset % BLKSIZE;
     
       // I only show how to read DIRECT BLOCKS. YOU do INDIRECT and D_INDIRECT
 
       if (lbk < 12){                     // lbk is a direct block
           blk = running->fd[fd]->mptr->INODE.i_block[lbk]; // map LOGICAL lbk to PHYSICAL blk
       }
       else if (lbk >= 12 && lbk < 256 + 12) { 
            //  read from indirect blocks 
			int indirect_buf[256];
		    get_block(running->fd[fd]->mptr->dev,running->fd[fd]->mptr->INODE.i_block[12],indirect_buf);
			blk = indirect_buf[lbk-12]; 
       }
       else{ 
            //  double indirect blocks
			int count=lbk-12-256;
			int num=count/256;
			int pos_offset=count%256;
			int double_buf1[256];
			get_block(running->fd[fd]->mptr->dev,running->fd[fd]->mptr->INODE.i_block[13],double_buf1);
			int double_buf2[256];
			get_block(running->fd[fd]->mptr->dev,double_buf1[num],double_buf2);
			blk=double_buf2[pos_offset];
       } 
       /* get the data block into readbuf[BLKSIZE] */
	   char readbuf[BLKSIZE];
       get_block(running->fd[fd]->mptr->dev, blk, readbuf);

       /* copy from startByte to buf[ ], at most remain bytes in this block */
       char *cp = readbuf + startByte;   
       remain = BLKSIZE - startByte;   // number of bytes remain in readbuf[]
       /*
		OPTMIAZATION OF THE READ CODE:

		Instead of reading one byte at a time and updating the counters on each byte,
		TRY to calculate the maximum number of bytes available in a data block and
		the number of bytes still needed to read. Take the minimum of the two, and read
		that many bytes in one operation. Then adjust the counters accordingly. This 
		would make the read loops more efficient. 
	   
       while (remain > 0){
            *cq++ = *cp++;             // copy byte from readbuf[] into buf[]
             running->fd[fd]->offset++;           // advance offset 
             count_read++;                  // inc count as number of bytes read
             avil--; nbytes--;  remain--;
             if (nbytes <= 0 || avil <= 0) 
                 break;
       }*/
/*	   while (remain > 0){
            *cq++ = *cp++;             // copy byte from readbuf[] into buf[]
             running->fd[fd]->offset++;           // advance offset 
             count_read++;                  // inc count as number of bytes read
             avil--; nbytes--;  remain--;
             if (nbytes <= 0 || avil <= 0) 
                 break;
		}
*/
		while (remain > 0){		
			int min=0;
			if(avil<=nbytes) 
				min=avil;
			else 
				min=nbytes;
			memcpy(cq,cp,min);
			running->fd[fd]->offset+=min;           // advance offset 
            count_read+=min;                  // inc count as number of bytes read
            avil-=min; //number of bytes still available in file.
			nbytes-=min; // the number of bytes the user wants to read 
			remain-=min;//number of bytes remain in readbuf[]
            if (nbytes <= 0 || avil <= 0) 
                break;
        }
 

	   //print things that have been read into buf	
		if(print_info_mark>0)	
			printf("%s",buf);
		
       // if one data block is not enough, loop back to OUTER while for more ...
   }
   if(print_info_mark>0) {
   		printf("\n***********************************\n");
   		printf("myread: read %d char from file descriptor %d\n", count_read, fd);
	}  
   return count_read;   // count is the actual number of bytes read
}

/*
==========================  HOW TO cat ======================================
cat filename: pathname=filename

   char mybuf[1024], dummy = 0;  // a null char at end of mybuf[ ]
   int n;

1. int fd = open filename for READ;
2. while( n = read(fd, mybuf[1024], 1024)){
       mybuf[n] = 0;             // as a null terminated string
       // printf("%s", mybuf);   <=== THIS works but not good
       spit out chars from mybuf[ ] but handle \n properly;
   } 
3. close(fd);
*/
int cat_file() {
   char mybuf[BLKSIZE];
   int dummy = 0;  // a null char at end of mybuf[ ]
   int n=0;
//make parameter into 0
   strcpy(parameter,"0");
//1. int fd = open filename for READ;
   int fd = open_file();
   if(fd==-1) {
		printf("cat : file open error\n");
		return -1;
	}
/*2. while( n = read(fd, mybuf[1024], 1024)){
       mybuf[n] = 0;             // as a null terminated string
       // printf("%s", mybuf);   <=== THIS works but not good
       spit out chars from mybuf[ ] but handle \n properly;
   }*/
   printf("=================================\n");
   while(n = myread(fd, mybuf, 1024,0)){
       mybuf[n] = '\0';             // as a null terminated string
       // printf("%s", mybuf);   <=== THIS works but not good 
       // spit out chars from mybuf[ ] but handle \n properly;\r\n
	   int i=0;		
	   while(mybuf[i]!='\0') {
		   if(mybuf[i]=='\n') {
				putchar('\r');
			}
		   putchar(mybuf[i]);
		   i++;
	   }
   } 
   printf("\n=================================\n");
//3. close(fd);
   close_file(fd);
   return 0;
}
