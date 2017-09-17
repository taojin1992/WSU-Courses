#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

//#include <dirent.h>  DIR conflict
struct dirent {
    ino_t          d_ino;       /* inode number */
    off_t          d_off;       /* offset to the next dirent */
    unsigned short d_reclen;    /* length of this record */
    unsigned char  d_type;      /* type of file; not supported
                                   by all file system types */
    char           d_name[256]; /* filename */
};

#include "type.h"

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;
OFT    oft[NOFT];
char names[64][128],*name[64]; // assume at most 64 components in pathnames
int fd, n;//n=the number of tokens of input dir
int nblocks, ninodes, bmap, imap, inode_start;
char pathname[256], parameter[256];
int dev;                             
char *disk = "diskimage";
char buf[BLKSIZE];              // define buf1[ ], buf2[ ], etc. as you need

#include "iget_iput_getino.c"
#include "alloc_dealloc.c"
#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir_rm.c"
#include "link_unlink.c"
#include "symlink_readlink.c"
#include "chmod_touch.c"
#include "open_close_pfd_lseek.c"
#include "read_cat.c"
#include "write_mv_cp.c"

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;//All minode's refCount=0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = 0;//how about proc[1]'s uid=1?
    p->cwd = 0;
    p->status = FREE;
    for (j=0; j<NFD; j++)
      p->fd[j] = 0;
  }
  for (i=0; i<NOFT; i++){
    oft[i].refCount = 0;
    oft[i].mode = -1;
	oft[i].mptr=NULL;
	oft[i].offset=0;
  }
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);         // Do you understand this?
}

int quit()
{
  int i=0;
  for(i=0; i<NMINODE; i++) {
	if (minode[i].refCount > 0 && minode[i].dirty) {
		iput(&minode[i]);//write its INODE back to disk;
	}
  }
  exit(0);  // terminate program
}

int menu() {
	printf("******************** Menu *******************\n");
    printf("mkdir     creat     mount(N)     umount(N)    rmdir\n");
    printf("cd        ls        pwd       stat(N)      rm\n");   
    printf("link      unlink    symlink   chmod     chown(N)  touch\n");
    printf("open      pfd       lseek     rewind(N)    close\n");
   	printf("read      write     cat       cp        mv\n");
    printf("cs(N)        fork(N)      ps(N)        kill(N)      quit\n");
    printf("=============   Usage Examples ==============\n");
    printf("mkdir  filename\n");
    //printf("mount  filesys   /mnt\n");
    printf("chmod  0644 filename\n");
    //printf("chown  filename  uid\n");
    printf("open   filename  mode (0|1|2|3 for R|W|RW|AP)\n");
    printf("write  fd  text_string\n");
    printf("read   fd  nbytes\n");
    printf("pfd    (display opened file descriptors)\n");
    //printf("cs     (switch process)\n");
    //printf("fork   (fork child process)\n");
    //printf("ps     (show process queue as Pi[uid]==>}\n");
    //printf("kill   pid   (kill a process)\n");
    printf("*********************************************\n");
}


main(int argc, char *argv[ ])   // run as a.out [diskname]
{
/*  print fd or dev to see its value!!!

(1). printf("checking EXT2 FS\n");

     Write C code to check EXT2 FS; if not EXT2 FS: exit

     get ninodes, nblocks (and print their values)


(2). Read GD block to get bmap, imap, iblock (and print their values)
*/
  int ino;
  char buf[BLKSIZE];
  char line[256], cmd[64];

  if (argc > 1)
    disk = argv[1];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);  
	exit(1);
  }
  dev = fd;
  /********** read super block at 1024 ****************/
  get_block(dev, 1, buf);

  sp = (SUPER *)buf;

  /* verify it's an ext2 file system *****************/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }  
  printf("OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  inode_start = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);


//(3). init();
  init();

//(4). mount_root();  
  mount_root();
  printf("root refCount = %d\n", root->refCount);
/*(5). printf("creating P0 as running process\n");

     WRITE C code to do these:     
       set running pointer to point at proc[0];
       set running's cwd   to point at / in memory;
*/
  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);
  while(1){       // command processing loop
    printf("input command: [menu|ls|cd|pwd|mkdir|creat|rmdir|rm|link|unlink|symlink|readlink|chmod|touch");
	printf("|open|close|lseek|pfd|read|cat|cp|mv|write]\n"); 
	fgets(line, 128, stdin);//use fgets() to get user inputs into line[128]
    line[strlen(line) - 1] = 0;//kill the \r at end 

    if (line[0]==0) {//if (line is an empty string), namely if user entered only \r
      continue;
	}
    pathname[0] = 0;
    parameter[0] = 0;
    memset(parameter, 0, 256);
 	//Extract cmd, pathname from line and save them as globals.
    sscanf(line, "%s %s %64c", cmd, pathname,parameter);//Use sscanf() to extract cmd[ ] and pathname[] from line[128]
    printf("cmd=%s pathname=%s parameter=%s\n", cmd, pathname,parameter);
    // execute the cmd
	if (strcmp(cmd, "menu")==0)
       menu();
    if (strcmp(cmd, "ls")==0)
       ls(pathname);
    if (strcmp(cmd, "cd")==0)
       chdir(pathname);
    if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
	if (strcmp(cmd, "mkdir")==0)
       make_dir();
	if (strcmp(cmd, "creat")==0)
       creat_file();
	if (strcmp(cmd, "rmdir")==0)
      rmdir();
	if (strcmp(cmd, "rm")==0)
      unlink();
    if (strcmp(cmd, "link")==0)
      link();
    if (strcmp(cmd, "unlink")==0)
      unlink();
    if (strcmp(cmd, "symlink")==0)
      symlink();
    if (strcmp(cmd, "readlink")==0){
      readlink(line);
      printf("symlink name = %s\n", line);
    }
	if (strcmp(cmd, "chmod")==0)
      my_chmod();
	if (strcmp(cmd, "touch")==0)
      my_touch();
	if (strcmp(cmd, "open")==0){
      open_file();
    }
    if (strcmp(cmd, "close")==0){
	  int fdescriptor=0;
      fdescriptor=atoi(pathname);
	  if(strcmp(pathname,"0")!=0 && fdescriptor==0) {
	      printf("invalid input\n");
	 	  continue;
	  }    
      close_file(fdescriptor);
    }
	if (strcmp(cmd, "pfd")==0){
      pfd();
    }
    if (strcmp(cmd, "lseek")==0){//int lseek(int fd, int position)--lseek fd 0
	  if(pathname[0]=='\0' || parameter[0]=='\0') 
	  {
	  	 printf("invalid input\n");
	  }
	  int fdes=0;
	  fdes=atoi(pathname);
	  if(strcmp(pathname,"0")!=0 && fdes==0) {
	      printf("invalid input\n");
	 	  continue;
	  }     
	  int curpos=atoi(parameter);     
	  if(strcmp(parameter,"0")!=0 && curpos==0) {
	      printf("invalid input\n");
		  continue;
	  }
	  int off=mylseek(fdes,curpos);
	  printf("current offset after lseek=%d\n",off);
    } 
    if (strcmp(cmd, "read")==0){
      read_file();
    }
    if (strcmp(cmd, "cat")==0){
      cat_file();
    }
	if (strcmp(cmd, "cp")==0){
      cp_file();
    }
	if (strcmp(cmd, "mv")==0){
      mv_file();
    }
    if (strcmp(cmd, "write")==0){
      write_file();
    }
    if (strcmp(cmd, "quit")==0)
       quit();
    }
}


