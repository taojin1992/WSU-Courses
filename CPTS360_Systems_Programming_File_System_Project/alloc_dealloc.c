//my ialloc/idealloc, balloc/bdealloc 
//balloc.c to allocate a FREE disk block; return its bno(block number)

int decFreeBlocks(int dev)
{
  char buf2[BLKSIZE];
  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf2);
  sp = (SUPER *)buf2;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf2);
  get_block(dev, 2, buf2);
  gp = (GD *)buf2;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf2);
}


int balloc(int dev)//note: block #0 is not used by the file system
{
  int  i;
  char buf2[BLKSIZE],buf3[BLKSIZE];
  // read block_bitmap block
  get_block(dev, 2, buf3); 
  gp = (GD *)buf3;
  bmap = gp->bg_block_bitmap;	
  get_block(dev, bmap, buf2);
  for (i=1; i < nblocks; i++){
    if (tst_bit(buf2, i)==0){
       set_bit(buf2,i);
       decFreeBlocks(dev);
       put_block(dev, bmap, buf2);
       return i;
    }
  }
  printf("balloc(): no more free blocks\n");
  return 0;
}

int decFreeInodes(int dev)
{
  char buf2[BLKSIZE];
  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf2);
  sp = (SUPER *)buf2;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf2);
  get_block(dev, 2, buf2);
  gp = (GD *)buf2;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf2);
}

int ialloc(int dev)
{
  int  i;
  char buf2[BLKSIZE];
  
  get_block(dev, 2, buf2);
  gp = (GD *)buf2;
  imap = gp->bg_inode_bitmap;

  // read inode_bitmap block
  get_block(dev, imap, buf2);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf2, i)==0){
       set_bit(buf2,i);
       decFreeInodes(dev);

       put_block(dev, imap, buf2);

       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
}

//idealloc(int dev, int ino), which deallocates an inode number, ino
int incFreeInodes(int dev)
{
  char buf2[BLKSIZE];
  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf2);
  sp = (SUPER *)buf2;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf2);

  get_block(dev, 2, buf2);
  gp = (GD *)buf2;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf2);
}

int idealloc(int dev, int ino) {
  	char buf2[BLKSIZE];
  
  	get_block(dev, 2, buf2);
  	gp = (GD *)buf2;
  	imap = gp->bg_inode_bitmap;

  	// read inode_bitmap block
  	get_block(dev, imap, buf2);
  	 
   	if (tst_bit(buf2, ino-1)==1){
       	clr_bit(buf2,ino);
      	incFreeInodes(dev);
       	put_block(dev, imap, buf2);
		return 0;
   	}
	else {
		printf("This ino is not in use. No need to deallocate.\n");
		return -1;
	}
}

//bdealloc(int dev, int bno), which deallocates an block number, bno
int incFreeBlocks(int dev)
{
  char buf2[BLKSIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf2);
  sp = (SUPER *)buf2;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buf2);

  get_block(dev, 2, buf2);
  gp = (GD *)buf2;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf2);
}

int bdealloc(int dev, int bno) {
  char buf2[BLKSIZE],buf3[BLKSIZE];
  // read block_bitmap block
  get_block(dev, 2, buf3); 
  gp = (GD *)buf3;
  bmap = gp->bg_block_bitmap;
	
  get_block(dev, bmap, buf2);
  
  if (tst_bit(buf2, bno)==1){
     clr_bit(buf2,bno);
     incFreeBlocks(dev);
     put_block(dev, bmap, buf2);
     return 0;
  }
  else {
		printf("This block is not in use. No need to deallocate.\n");
		return -1;
	} 
}

