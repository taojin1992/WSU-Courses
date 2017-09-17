#ifndef _TYPEH_
#define _TYPEH_
//Guard head files will prevent you too have redefinitions if you have a loop in header inclusions.


//OBJECTIVE: mount root to start the PROJECT; ls, cd, pwd to show FS contents
//*************** type.h file for FS Level-1***************
//it includes the data structure types of EXT2
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;//directory entry structure


SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define BLKSIZE  1024
#define NMINODE   100
#define NFD        16
#define NPROC       4
#define NOFT 	 32//assume two processes, each has 16 descriptors
#define FREE        0
#define READY		1

typedef struct minode{   //in-memory INODE of the file
  INODE INODE;
  int dev, ino;
  int refCount;
  int dirty;
//if mounted=0, it means it hasn't been mounted;otherwise, it has been mounted and its content is not its original content.
  int mounted;
  struct mntable *mptr;
}MINODE;

//open file table struct
typedef struct oft{
  int  mode;
  int  refCount;
  MINODE *mptr;//it points to the file's INODE in memory
  int  offset;
}OFT;

typedef struct proc{
  struct proc *next;
  int          pid;
  int          uid;
  MINODE      *cwd;
  OFT         *fd[NFD];
  int 		status;//status = FREE/READY
}PROC;

#endif

