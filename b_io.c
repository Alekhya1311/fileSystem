/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Justin Adriano, Anudeep Katukojwala, Kristian Goranov, Pranjal Newalkar
* Student IDs: 921719692 922404701 922003001 922892162
* GitHub Name: pranjal2410
* Group Name: Fantastic Four
* Project: Basic File System
*
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"

//Added
#include "fsUtils.h"
#include "mfs.h"
#include "fsLow.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512


typedef struct b_fcb
{
	char * buf;		//holds the open file buffer //malloced
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
	int flags;
	int fileOffset;
	int bytesNotCopied; //left over data in our buffer, not copied to user buffer.
	int fileSize;
	int currentBlock;
	int bufferOffset;
	DE * parentDir; //malloced
	char * parentPath; //malloced
	DE * fileDE; 
} b_fcb;
	

b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}



//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}


//helper print FCB info
void printFCBinfo(b_fcb fcb) {
	printf("\nFCB ================================D\n");
    printf("index : %d\n", fcb.index);
	printf("buflen : %d\n", fcb.buflen);
	printf("flags : %d\n", fcb.flags);
	printf("offset : %d\n", fcb.fileOffset);
	printf("bytesNotCopied : %d\n", fcb.bytesNotCopied);
	printf("fileSize : %d\n", fcb.fileSize);

	if (fcb.fileDE != NULL)
	{
		printf("\nfileDE Struct -------------->\n");
		printf("DE filename : %s\n", fcb.fileDE->fileName);
		printf("DE fileSize : %d\n", fcb.fileDE->fileSize);
		printf("DE location : %d\n", fcb.fileDE->location);
		printf("DE blockspanned : %d\n", fcb.fileDE->blocksSpanned);
	}

	printf("\nparent dir print +++++++++++++++++++++++++++\n");

	for (int i = 0; i < NUM_DE_IN_DIR; i++)
	{
		printf("\n");
		printf("Dfilename : %s\n", fcb.parentDir[i].fileName);
		printf("Dlocation : %d\n", fcb.parentDir[i].location);
		printf("Dfilesize: %d\n", fcb.parentDir[i].fileSize);
		printf("Dblockspanned : %d\n", fcb.parentDir[i].blocksSpanned);
		printf("DfileType : %s\n", fcb.parentDir[i].fileType);
		printf("\n");
	}
	printf("\n end FCB ================================D\n");
}




// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR, O_CREAT, O_TRUNC
b_io_fd b_open (char * filename, int flags)
	{
	if (startup == 0) b_init();  //Initialize our system
	
	
	b_io_fd returnFd;

	//check get info on file.
    PPI * pathInfo = parsePath(filename);	

	//is it a file. can use isfile() when ready.
	//does it exist
	if (pathInfo == NULL)
	{
		printf("invalid path sent to b_open, pathIinfo = NULL\n");
		return -1;
	}

    if (strcmp(pathInfo->fileType, "d") == 0)
	{
		printf("Error: can not open, its a directory\n");
		return -1;
	}
	
	//file either exists or doesnt. will have to check flags permission to 
	//how to proceed.
	returnFd = b_getFCB(); 
    b_fcb * fcb = &fcbArray[returnFd]; //file control block

	//set FCB values that dont depend on file existing or not.
	fcb->index = 0;
	fcb->buflen = 0;
	fcb->flags = flags;
	fcb->currentBlock = 0;
	fcb->bufferOffset = 0;
	
	//buffer only used by read.
	fcb->buf = malloc(B_CHUNK_SIZE); //free in close.

	//parent path in struct for checking if we need to update current dir to reflect changes.
	fcb->parentPath = malloc(strlen(pathInfo->path) + 1);// need to free.
	strcpy(fcb->parentPath, pathInfo->path);
	

    //Handling when a file does not exist. 
	if(pathInfo->exists != 0 && (flags & O_CREAT) == O_CREAT) {
		//file does not exist
		//create file.
		printf("file does not exist, CREAT FILE!\n");

        //copy of parent dir for passing around functions
		fcb->parentDir = malloc(NUM_DE_IN_DIR * sizeof(DE));
		memcpy(fcb->parentDir, pathInfo->parentDirPtr, (NUM_DE_IN_DIR * sizeof(DE)));

		//Directory entry of new file, to be manipulated by other read, write functions
		fcb->fileDE = findEmptyDE(fcb->parentDir);
		if (fcb->fileDE == NULL)
		{
			printf("OUt of empty DE in dir, return -1, b_io.c, b_open\n");
			return -1;
		}
		
		//test
		printf("create file in b_open\n");
		createFile(pathInfo->name, fcb->fileDE, fcb->parentDir);

        //test
		printDE(fcb->fileDE);

		printf("returnfd = %d\n", returnFd);
		return returnFd;
	}
	else if (pathInfo->exists != 0)
	{
		printf("b_open: file does not exist, no O_CREAT flag, return -1\n");
		return -1;
	}

	//file exiists.
    printf("b_open open existing file\n");
	
	// copy parent dir for passing around in struct.
	fcb->parentDir = malloc(NUM_DE_IN_DIR * sizeof(DE));
	memcpy(fcb->parentDir, pathInfo->parentDirPtr, (NUM_DE_IN_DIR * sizeof(DE)));

    //DE pointer to parentdirectory copy in FCB struct for modifiying contents of DE
	//in write and seek functions
	fcb->fileDE = &fcb->parentDir[pathInfo->index];

    //filesize for modifying prior to modifying DE for final changes
	fcb->fileSize = pathInfo->parentDirPtr[pathInfo->index].fileSize;

    
    //truncate file, not freeing any block, until close
	if (flags & O_TRUNC == O_TRUNC)
	{
		printf("b_open truncating file\n");
		fcb->fileDE->fileSize = 0;
		fcb->fileOffset = 0;
		fcb->fileSize = 0;
	}
	
	// printf("b_open DE print\n");
	// printDE(fcb->fileDE);
	printf("b_open filename : %s\n", fcb->fileDE->fileName);
	printf("b_open existing file size : %d\n",fcb->fileSize);
	printf("END b_open\n");
	printf("\n");
	return returnFd;
}//End b_open



// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	
	int updatedOffset;
		
	switch (whence)
		{

		case SEEK_SET:
			{

			if(offset<0){
				updatedOffset=-1;
			}
			else{
				updatedOffset = offset;
			}
			
			break;
			}
		case SEEK_CUR:
			{
			if(fcbArray[fd].index + offset>fcbArray[fd].fileSize || fcbArray[fd].index + offset < 0){
				updatedOffset = -1;
			}
			else{
				updatedOffset = fcbArray[fd].index + offset;
			}
			
			break;
			}
		case SEEK_END:
			{
			if(fcbArray[fd].fileSize + offset>fcbArray[fd].fileSize || fcbArray[fd].fileSize + offset < 0){
				updatedOffset = -1;
			}
			else{
				updatedOffset = fcbArray[fd].fileSize + offset;
			}
			
			break;
			}
		default:
			return -1;
		}
	
	return updatedOffset; 
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	int writeFlags = fcbArray[fd].flags;

	// writing the file to disk
	// if offset + count < blockedsSpanned * bytesperblock -> write buffer to memory
	// read in 512 blocks: ex. offset = 200 read in block that contains 200 bytes
	// 
	// making the file larger
	// check file size and blocked spanned
	// if the file size >= blocked spanned * bytes in a block
	// -> update amount of memory allocated : ask for a new location with a larger size -> allocate freespace
	// -> write old stuff from old location to new location

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	
	if(!((writeFlags & O_RDWR) == 0) || ((writeFlags & O_WRONLY) == 0))
		{
			printf("Error : file does not have acess to write\n");
			return (-1);
		}
	
	int bytesRemaining = count;
    int blocksWritten = 0;
	int bytesWritten = 0;

	if(fcbArray[fd].fileDE->fileSize >= fcbArray[fd].fileDE->blocksSpanned * BLOCK_SIZE) {
		printf("*********** Enter if statement 1 ***********\n");
        // allocated freespace but never set it as used
        // never freed the old space never wrote the new space
        // never copied old data to new location
        // free old space
		// PPI *path = parsePath(fcbArray[fd].fileDE->fileName)

		//create a buffer that will hold files contents to be moved
		char *newBuf = malloc(fcbArray[fd].fileDE->blocksSpanned * BLOCK_SIZE);

		LBAread(newBuf, fcbArray[fd].fileDE->blocksSpanned, fcbArray[fd].fileDE->location);

        int newLoc = allocateFreeSpace((fcbArray[fd].fileDE->blocksSpanned * BLOCK_SIZE) * 2);
        
		LBAwrite(newBuf, fcbArray[fd].fileDE->blocksSpanned, newLoc);

		
		setTheBlocks(newLoc, fcbArray[fd].fileDE->blocksSpanned * 2);

		// free the old blocks that were used
		freeTheBlocks(fcbArray[fd].fileDE->location,fcbArray[fd].fileDE->blocksSpanned);

		// update new file location
		fcbArray[fd].fileDE->location = newLoc;
		fcbArray[fd].fileDE->blocksSpanned = fcbArray[fd].fileDE->blocksSpanned * 2;

		reloadCurrentDir(dir[0]);

		free(newBuf);
		newBuf = NULL;
		

    }

    if(bytesRemaining >= B_CHUNK_SIZE) {

		//
		int blocksToWrite = bytesRemaining/B_CHUNK_SIZE;
		

		// LBAwrite returns blocks written -> multiply by chunk_size 
		// increment seperately 
		// missing : change filesize cause we wrote to it #num of bytes we wrote
		// blocksSpanned 10 blocks dif from filesize -> num of bytes
		// need to change fileoffset
		// update filesize when writting
		
		// temporary buffer for system buffer
		char *tempBuf = malloc(blocksToWrite * B_CHUNK_SIZE);
		memcpy(tempBuf, buffer, blocksToWrite * B_CHUNK_SIZE);

        blocksWritten += LBAwrite(tempBuf, blocksToWrite, 
							fcbArray[fd].bufferOffset + fcbArray[fd].fileDE->location);
		
        bytesRemaining -= blocksToWrite * B_CHUNK_SIZE;

        int newFileSize = blocksToWrite * B_CHUNK_SIZE;
		int sizeOffset = fcbArray[fd].fileSize - fcbArray[fd].fileOffset;	

		fcbArray[fd].fileOffset += blocksToWrite * B_CHUNK_SIZE;
		fcbArray[fd].bufferOffset = blocksToWrite * B_CHUNK_SIZE;

        fcbArray[fd].currentBlock += blocksToWrite;

		free(tempBuf);
		tempBuf = NULL;
    }

	// write all the bytes remaining
	
    while(bytesRemaining < B_CHUNK_SIZE && bytesRemaining > 0) {

		// location in our buffer
		int bufLoc;

		// temporary buffer for our buffer
		char *tempBuf = malloc(B_CHUNK_SIZE);

		//
		int blockOffset = blocksNeeded(fcbArray[fd].fileOffset) - 1;

		

		// when blockoffset == 512 returns invalid block offset we need

		if(fcbArray[fd].fileOffset % B_CHUNK_SIZE == 0 && fcbArray[fd].fileOffset >= B_CHUNK_SIZE)
			{
				blockOffset += 1;
			}
		
		
		/*  **** for debug *****
		printf("block offset after if statement : %d\n", blockOffset);
		printf("actual file de location: %d\n", fcbArray[fd].fileDE->location);
		printf("blockOffset : %d\n", blockOffset);
		printf("file Offset : %d\n", fcbArray[fd].fileOffset);
		*/
		
		// Copy the initial content to temporary buffer
		// to throw everything in the block at the end
		LBAread(tempBuf, 1, blockOffset + fcbArray[fd].fileDE->location);

		if(fcbArray[fd].fileOffset < B_CHUNK_SIZE)
			{
				bufLoc = fcbArray[fd].fileOffset;
			}
		else if(fcbArray[fd].fileOffset >= B_CHUNK_SIZE)
			{
				bufLoc = fcbArray[fd].fileOffset % B_CHUNK_SIZE;
			}

		int bytesToWrite;

		if(bytesRemaining > B_CHUNK_SIZE - bufLoc)
			{
				bytesToWrite = B_CHUNK_SIZE - bufLoc;
			}
		else
			{
				bytesToWrite = bytesRemaining;
			}
		
		
		// **** works like this *******
		memcpy(tempBuf + bufLoc, buffer, bytesToWrite);
		fcbArray[fd].bufferOffset = bytesToWrite;
		// ** reconsider this **
		buffer += fcbArray[fd].bufferOffset;
		//**** buffer offset vs bytesTowrite


        int blocksWritten = LBAwrite(tempBuf, 1, 
						blockOffset + fcbArray[fd].fileDE->location);
	

		// need to check if the offset is less than the fileSize like other function

		//fcbArray[fd].fileDE->fileSize += bytesToWrite;
		//fcbArray[fd].fileSize += bytesToWrite;
		fcbArray[fd].fileOffset += bytesToWrite;

		// buffer += bytesRemaining;

		bytesWritten += bytesToWrite;
        bytesRemaining -= bytesToWrite;

        // fcbArray[fd].currentBlock += 1;

		// update file size and file offset
		free(tempBuf);

		
    }

	if(fcbArray[fd].buf == NULL) {
        fcbArray[fd].buf = malloc(B_CHUNK_SIZE);
    }
	
	fcbArray[fd].bufferOffset = 0; // Once processing is done, write will be called again with a fresh buffer
	fcbArray[fd].fileDE->fileSize += count;
	

	return (bytesWritten);
	
}//End b_write



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		printf("invalid FD \n");
		return (-1); 					//invalid file descriptor
		}

	//check if the file has permissions to read based on flags value
	int flagValue = fcbArray[fd].flags;
	if(!((flagValue & O_RDONLY) == O_RDONLY || (flagValue & O_RDWR) == O_RDWR)){
		printf("Error! File needs to be opened with O_RDONLY or O_RDWR\n");
		return -1;
	}

	// and check that the specified FCB is actually in use	
	if (fcbArray[fd].fileDE == NULL)		//File not open for this descriptor
		{
		printf("File not open for this descriptor\n");
		return -1;
		}	

	char * fileBuffer = fcbArray[fd].buf; //our buffer
	int fileSize = fcbArray[fd].fileDE->fileSize;
	// not actual location just a copy;.
	int lbaLocation = fcbArray[fd].fileDE->location; 
    int *fileOffset = &(fcbArray[fd].fileOffset);
    int *bytesNotCopied = &(fcbArray[fd].bytesNotCopied);
	int bytesCopied = 0;


    //Determine the correct number of bytes to read from the file.
	int bytesToCopy;
	if (*fileOffset == fileSize) {
		//reached end of 
		return bytesCopied; //should be 0.
	}
	else if ((count + *fileOffset) > fileSize) { 
		//too many bytes requested, trim request to end of file.
		bytesToCopy = fileSize - *fileOffset;
	}
	else{
	   //total bytes to be read will not go past file size.
	   bytesToCopy = count;
	}

	//copy any bytes left over from previous calls to read() not 
	//copied to users buffer before requesting any new blocks of data.
	if (*bytesNotCopied > 0)
	{
		//How much of the leftover data to copy into the users buffer.
		int copyBytes = (*bytesNotCopied > bytesToCopy) ? 
		                    bytesToCopy : *bytesNotCopied;

		//starting buffer postition to copy from.
		char *startBuff = fileBuffer + (B_CHUNK_SIZE - *bytesNotCopied);

		//copy data to user buffer
		if (memcpy(buffer, startBuff, copyBytes) == NULL) {
			printf("Error: memcopy failed copying to buffer\n");
			return -1;
		}
		buffer += copyBytes; //advance buffer position
		bytesCopied += copyBytes; 
		bytesToCopy -= copyBytes;
		*bytesNotCopied -= copyBytes;
		*fileOffset += copyBytes;

        //requested amount of bytes to read fullfilled.
		if(bytesToCopy == 0) {
			return bytesCopied;
		}
	}
	

	// Get the correct lba block to start from so we
	// can get blocks from where we last left off.
	if (*fileOffset > 0) { 
	    lbaLocation += (*fileOffset + (B_CHUNK_SIZE - 1))/ B_CHUNK_SIZE;	
	}


    //Bytes requested that can be divided into B_CHUNK_SIZE is directly 
	//copied into the users buffer.
	int bytesRead = 0;
	int blocksRead = 0; 
	int numBlocks = bytesToCopy/B_CHUNK_SIZE;
	if(numBlocks >= 1)
	{
		
		//write directly to parameter buffer
		blocksRead = LBAread(buffer, numBlocks, lbaLocation);
		if (blocksRead == -1) {
			printf("Error getting blockfrom LBAread\n");
			return -1;
		}
		bytesRead = blocksRead * BLOCK_SIZE;

		buffer += bytesRead; //advance buffer location.
		*fileOffset += bytesRead; 
		bytesToCopy -= bytesRead;
		bytesCopied += bytesRead;
		lbaLocation+= numBlocks;	
	}
     
    
	//number of bytes left to read are less than
	//the size of a chunk. We will have left over
	//bytes not copied to the users buffer.
	if (bytesToCopy > 0)
	{ 
		// printf("lba loc : %d\n", lbaLocation);
        // printf("\n1. ParentDir copy in read\n");
		// printDir(fcbArray[fd].parentDir);
		// printf("read loc parent dir: %d\n", fcbArray[fd].parentDir[0].location);
		// printf("read fd: %d\n", fd);
		
		blocksRead = LBAread(fileBuffer, 1, lbaLocation);
		// printf("bytes read : %d\n", bytesRead);
        // printf("\n 2. END ParentDir copy in read\n");
		// printDir(fcbArray[fd].parentDir);
		// printf("End read loc parent dir: %d\n", fcbArray[fd].parentDir[0].location);
		// printf("read fd: %d\n", fd);
		if (blocksRead == -1) {
			printf("Error getting blockfrom LBAread\n");
			return -1;
		}
		bytesRead = blocksRead * BLOCK_SIZE;
		lbaLocation++;

		//write remaining bytes to parameter buffer
		if (memcpy(buffer, fileBuffer, bytesToCopy) == NULL) {
			printf("Error: memcopy failed copying to buffer\n");
			return -1;
		}
		*fileOffset += bytesToCopy;
		bytesCopied += bytesToCopy;

		//uncopied bytes remaining in our buffer.
		*bytesNotCopied = B_CHUNK_SIZE - bytesToCopy;
	}
   	return bytesCopied;
		
	
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{
		printf("\nfileDE in close = %d : %s\n", fd, fcbArray[fd].fileDE->fileName);
		// modify file de filesize
		// fcbArray[fd].fileDE->fileSize = fcbArray[fd].fileSize;
		// printf("fileDE in close = %d\n", fd);
		printf("\nParentDir copy in close\n");
		printf("b_close loc parent dir: %d\n", fcbArray[fd].parentDir[0].location);
		//write parent dir to volume. if dir is current directory reload.
		int retWriteDir = writeDirToVolume(fcbArray[fd].parentDir);
		if (retWriteDir < 0)
		{
			printf("error writing directory to volume, mfs.c, mkdir\n");
			return -1;
		}

		//if an entry in current dir is effected reload current dir to reflect changes.
		printf("parent path: %s\n", fcbArray[fd].parentPath);
		printf("currpath: %s\n", currPath);
		
		if (strcmp(fcbArray[fd].parentPath, currPath) == 0)
		{
			printf("reload current dir\n");
			reloadCurrentDir(fcbArray[fd].parentDir);
		}

        //clean up fcb struct
		printf("*****before free bclose*******\n");
		
		// free(fcbArray[fd].buf);
		// fcbArray[fd].buf = NULL; //also frees FCB slot
		

		// free(fcbArray[fd].fileDE); //not malloced
		// fcbArray[fd].fileDE = NULL;

		free(fcbArray[fd].parentDir);
		fcbArray[fd].parentDir = NULL;

		free(fcbArray[fd].parentPath);
		fcbArray[fd].parentPath = NULL;

		printf("\nCLOSING <<<< %s\n",fcbArray[fd].fileDE->fileName);
		
		return 0;
} //end b_close;
