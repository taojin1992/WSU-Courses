#include <time.h>

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

int ls_file(char *fname,MINODE *mip)
{
	int r, i;
    char ftime[64];
	if ((mip->INODE.i_mode & 0xF000) == 0x8000)
        printf("%c",'-');
    if ((mip->INODE.i_mode & 0xF000) == 0x4000)
        printf("%c",'d');
    if ((mip->INODE.i_mode & 0xF000) == 0xA000)
        printf("%c",'l');
    
    for (i=8; i >= 0; i--){
        if (mip->INODE.i_mode & (1 << i))
            printf("%c", t1[i]);
        else
            printf("%c", t2[i]);
    }
    
    printf("%4d ",mip->INODE.i_links_count);
    printf("%4d ",mip->INODE.i_gid);
    printf("%4d ",mip->INODE.i_uid);
    printf("%8d ",mip->INODE.i_size);
    // print time
    strcpy(ftime, ctime(&mip->INODE.i_ctime));
    ftime[strlen(ftime)-1] = 0;
    printf("%s  ",ftime);

    // print name
    printf("%s", basename(fname));//get the basename function included??
   	
    // print -> linkname if it's a symbolic file
	//modify ls using void readlink(char *buffer)
    if ((mip->INODE.i_mode & 0xF000)== 0xA000){ // YOU FINISH THIS PART
        // use readlink() SYSCALL to read the linkname
        // printf(" -> %s", linkname);
		
		//(3). return its string contents in INODE.i_block[ ].
		int i=0;
        char buffer[256];
		strcpy(buffer,(char *)mip->INODE.i_block);
        printf(" ->");
		printf("%s",buffer);       
    }
	printf("\n");

}

int ls_dir(char *dname,MINODE *mip)//print info of data in buf1
{
    
	char *cp;
	char buf1[BLKSIZE];
	int num=0;
	char namebuf[256];
	/*	step through DIR entries://the quiz question
    for each name, ls_file(pathname/name);//pathname/name append
	*/
	for(num=0;num<12 &&mip->INODE.i_block[num]!=0;num++) {
			printf("i_block[%d]=%d\n",num,mip->INODE.i_block[num]);
    		int blkno=mip->INODE.i_block[num];
			get_block(mip->dev, blkno, buf1);//everything we need is in buf1 now
			dp=(DIR*)buf1;
    		cp=buf1;
		/*	step through DIR entries://the quiz question
    	for each name, ls_file(pathname/name);//pathname/name append
		*/
    	while(cp<buf1+BLKSIZE) {
			strncpy(namebuf,dp->name,dp->name_len);	
			
			namebuf[dp->name_len]=0;
			MINODE* miptr=iget(mip->dev,dp->inode);//MINODE* miptr=iget(mip->dev,dp->inode)
			
			ls_file(namebuf,miptr);
			
			iput(miptr);
			cp+=dp->rec_len;
			dp=(DIR*)cp;
		}
	}
    return 0;
}

int ls(char *pathname)  // dig out YOUR OLD lab work for ls() code  if no pathname, list current dir
{
	int ino=0,num=0;
	char *cp;
	//  determine initial dev: 
    if (pathname[0]== '/') {
		dev = root->dev;
		//   convert pathname to (dev, ino);    int ino=getino(dev,pathname) one-line
    	ino=getino(&dev,pathname);  
	}
	else if (pathname[0]=='\0') {
		dev = running->cwd->dev;		
		ino=running->cwd->ino;
	}
    else {
		dev = running->cwd->dev;
		//    convert pathname to (dev, ino);    int ino=getino(dev,pathname) one-line
    	ino=getino(&dev,pathname);    
	}             
	//    get a MINODE *mip pointing at a minode[ ] with (dev, ino);
    MINODE *mip=iget(dev,ino); //return a pointer to inode. check iget. 
/*    
    if mip->INODE is a file: ls_file(pathname);
    if mip->INODE is a DIR{
       step through DIR entries://the quiz question
       for each name, ls_file(pathname/name);//pathname/name append
    }
*/	
	if(S_ISREG(mip->INODE.i_mode) || S_ISLNK(mip->INODE.i_mode)) {
		ls_file(pathname,mip);
	}
	else if(S_ISDIR(mip->INODE.i_mode)) {
		ls_dir(pathname,mip);
	}
	printf("\n");
	iput(mip);
}	


int chdir(char *pathname)
{
	//determine initial dev as in ls()
	int ino=0,num=0;
    if (pathname[0]== '/') {
		dev = root->dev;
	}
    else {
		dev = running->cwd->dev;  
	}             
//    convert pathname to (dev, ino); 
    ino=getino(&dev,pathname); 

//  get a MINODE *mip pointing at (dev, ino) in a minode[ ];
	MINODE *mip=iget(dev,ino);

    if (!S_ISDIR(mip->INODE.i_mode))
	{
		//mode=81a4   file1 is not a directory         ??????what is mode? no need to treat .,.. specically in this way right?
		printf("%s is not a directory\n",pathname);
	}

    if (S_ISDIR(mip->INODE.i_mode)) {
       //dispose of OLD cwd;//iput(running->cwd)
		iput(running->cwd);
       //set cwd to mip;
		running->cwd=mip;
    }
}

 

void getChildName(MINODE *parent,int childIno,char *resultName)
{
	int i=0;
	char *cp;
	char databuffer[BLKSIZE],namebuf[256];
	for(i=0;i<12 && parent->INODE.i_block[i]!=0;i++) {
		get_block(parent->dev,parent->INODE.i_block[i],databuffer);
		cp=databuffer;
		dp=(DIR*)databuffer;
		while(cp<databuffer+BLKSIZE) {
			if(childIno==dp->inode) {
				strncpy(namebuf,dp->name,dp->name_len);
				namebuf[dp->name_len]=0;
			}
			cp+=dp->rec_len;
			dp=(DIR*)cp;
		}		
	}
	strcpy(resultName,namebuf);

}


void pwd(MINODE *cur)//.. parent find the parent inonum. mailman aly to convert it back to an inode,append the string
{
    char result[256][256];
	int inodeNum=cur->ino;
	char buffer[BLKSIZE];
	int parent_ino=0;
	int index=0;
	if(inodeNum==root->ino && cur->dev==root->dev)
	{
		printf("/\n");
		return 0;
	}
	//get parent inode ptr,get i_block[0], using iget(dev, ino of ..), call pwd(parent's mip)
	//then search parent's children to get the name for that inodeNum
	while(cur->ino!=root->ino) {
		char resultName[1024];
		get_block(cur->dev, cur->INODE.i_block[0],buffer);
		parent_ino=search(cur, "..");//find the parent
		MINODE *parent_mip=iget(dev, parent_ino);
		getChildName(parent_mip,cur->ino,resultName);
		strcpy(result[index],"/");
		strcat(result[index],resultName);		
		cur=parent_mip;
		index++;
	}
	int last=index-1;
	for(index=last;index>=0;index--) {
		printf("%s",result[index]);
	}	
	printf("\n");
}


