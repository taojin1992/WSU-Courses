//symlink_readlink
/*
   symlink oldNAME  newNAME    e.g. symlink /a/b/c /x/y/z

   ASSUME: oldNAME has <= 60 chars, inlcuding the ending NULL byte.
*/

void symlink() {
//(1). verify oldNAME exists (either a DIR or a REG file) and new file doesn't exist yet
	char oldName[256]; 
	char newName[256];
	strcpy(oldName,pathname);
	strcpy(newName,parameter);
	char newcopy[256],newcopy1[256];
	strcpy(newcopy,newName);
	strcpy(newcopy1,newName);
	char *newparent=dirname(newcopy);
	char *newchild=basename(newcopy1);
	
	int oldino=getino(&dev, oldName);
	if(oldino==0) {
		printf("Error: the original file/dir doesn't exist\n");
		return;
	}
	int newino=getino(&dev, newName);
	if(newino>0) {
		printf("Error: the new file/dir already exists\n");
		return;
	}
	int newpino=getino(&dev,newparent);
	MINODE *newpip=iget(dev, newpino);
	MINODE *oldmip=iget(dev, oldino);
		
//(2). creat a FILE newfile /x/y/z
	printf("CALL MYCREAT\n");
	my_creat(newpip, newchild);
	newino=getino(&dev,newName);
    MINODE *newmip=iget(dev, newino);
//(3). change /x/y/z's type to LNK (0120000)=(1010.....)=0xA...
	newmip->INODE.i_mode=0xA1FF;
/*
(4). assume the length of old file name <=60 chars.
write the string oldNAME into the i_block[ ], which has room for 60 chars.-->because generally link is short.

(INODE has 24 unused bytes after i_block[]. So, up to 84 bytes for oldNAME)
every file is represented by a unique inode structure of 128 bytes in EXT2.48 60 28 due to u32 i_pad[7]
mark new_file's minode dirty
iput(new file's minode)
*/ 
	//int j=0;      //better way  strncpy((char*) i_block, s,strlen(s))  judge strlen(s)<60
	int length=strlen(oldName);
	if(length>60) {
		printf("the name is too long. cannot symlink.\n");
		return;
	}
	newmip->INODE.i_size = length;
	strncpy((char*) newmip->INODE.i_block,oldName,length);
	newmip->dirty=1;
	/*ndev=3 ino=13
	iblock=testfile
	name=testfile type=a1ff size=8 refCount=1*/
	printf("the info about the link file:\n");
	printf("ndev=%d ino=%d\n",newmip->dev,newmip->ino);
	printf("iblock content=%s\n",(char*)newmip->INODE.i_block);
	printf("type=%0x size=%d refCount=%d\n",newmip->INODE.i_mode,newmip->INODE.i_size,newmip->refCount); 
	iput(newmip);
//(5). mark new file parent minode dirty;put new file's parent inode
	newpip->dirty=1;
	iput(newpip);
	printf("symlink %s %s OK\n",pathname,parameter);
	
 } 

//readlink pathname: return the contents of a symLink file
void readlink(char *buffer)
{
//(1). get INODE of pathname into a minode[ ].
	int ino=getino(&dev, pathname);
	if(ino==0) {
		printf("it doesn't exist\n");
		return;
	}
	MINODE *mip=iget(dev, ino);
//(2). check INODE is a symbolic Link file.
	if(!S_ISLNK(mip->INODE.i_mode)) {
		printf("The file is not a symbolic Link file.\n");
		iput(mip);
		return;
	}
//(3). return its string contents in INODE.i_block[ ].
	int i=0;
	strcpy(buffer,(char *)mip->INODE.i_block);
	iput(mip);
}
