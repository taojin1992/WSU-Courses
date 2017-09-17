/*
The touch command is the easiest way to create new, empty files. It is also used to change the timestamps (i.e., dates and times of the most recent access and modification) on existing files and directories.
*/
//touch /a/f2 

int my_touch() {
	if (pathname[0]== '/') {
		dev = root->dev;
	}
    else {
		dev = running->cwd->dev;  
	}   
	int ino=getino(&dev, pathname);
	if(ino==0) {//the file doesn't exist and go create a new one
		//creat_file();
		printf("no such file\n");//don't create new file according to kc's sample program	
		return -1;
	}
	else {//modify the time fields
		MINODE *mip=iget(dev, ino);	
		mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L); 
		mip->dirty=1;
		iput(mip);
		return 0;
	}
}

//chmod    0766 filename -->100766   use the last three chars of pathname to cover the last 3 chars of previous permission bits
//drwxrw-rw-   0--7
int my_chmod() {
	int ino=getino(&dev,parameter);
	if(parameter[0]=='\0') {
		printf("invalid command\n");
		return -1;
	}
	if(ino==0) {
		printf("no such file\n");	
		return -1;
	}
	else {
		MINODE *mip=iget(dev, ino);	//*111->100111, file ; dir, -->40111
		int temp=mip->INODE.i_mode;
		temp=temp>>9;//clear the last 9 bits
		temp <<= 9;
		char *cp=pathname[3];//0 1 2 3
		temp=temp | strtoul(&pathname[0],&cp,8);
		mip->INODE.i_mode=temp;
		//printf("last:%0x\n",mip->INODE.i_mode);
		mip->dirty=1;
		iput(mip);
		return 0;
	}
}
