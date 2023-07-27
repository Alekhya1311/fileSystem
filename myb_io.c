/**************************************************************
* Class:  CSC-415-02 Summer 2023
* Names:  Saripalli Hruthika, Nixxy Dewalt, Alekya Bairaboina, Banting Lin 
* Student IDs: 923066687, 922018328, 923041428, 922404012
* GitHub Name: Alekhya1311
* Group Name: Zombies
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
#include "fsFunc.h"
#include "mfs.h"
#include "fsLow.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
	{
	/** TODO add al the information you need in the file control block **/
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
    DE * fileDE;    //Pointer to the Directory Entry
    int fileSize;
    int currentBufferOffset; //to keep track of the position within the buffer during read/write operations.
    int fileOffset; //current position within the file being read or written
    int currentBlock; //to keep track of the current block number within the file
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
		if (fcbArray[i].buff == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//
		
	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB();				// get our own file descriptor
										// check for error - all used FCB's
	
	return (returnFd);						// all set
	}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
    int fileAccessFlags = fcbArray[fd].flags;
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
    // Check if the file has the appropriate write access
	if(!((fileAccessFlags & O_RDWR) == 0) || ((fileAccessFlags & O_WRONLY) == 0)) 
		{
			return (-1); // File does not have access to write
		}
	
	int numRemainingBytes = count;
    int numBlocksWritten = 0;
	int totalBytes = 0;
    // If the file size is greater than or equal to the number of blocks spanned by the 
    // file multiplied by the block size (BLOCK_SIZE). This means file needs more space 
    // to accommodate the new data being written.
	if(fcbArray[fd].fileDE->fileSize >= fcbArray[fd].fileDE->dirBlocks * BLOCK_SIZE) {
		//allocates new space for the file, copies the old data to the new location, 
        // and updates the file descriptor accordingly.
		char *newBuffer = malloc(fcbArray[fd].fileDE->dirBlocks * BLOCK_SIZE);

		LBAread(newBuffer, fcbArray[fd].fileDE->dirBlocks, fcbArray[fd].fileDE->location);
        // Allocate new blocks for the file
        int newLocation = allocateFreeSpace(fcbArray[fd].fileDE->dirBlocks * 2);
        // Write the old data to the new blocks
		LBAwrite(newBuffer, fcbArray[fd].fileDE->dirBlocks, newLocation);

		// free the old blocks that were used
		freeTheBlocks(fcbArray[fd].fileDE->location,fcbArray[fd].fileDE->dirBlocks);

		// Update the file location and the number of blocks for the file
		fcbArray[fd].fileDE->location = newLocation;
		fcbArray[fd].fileDE->dirBlocks = fcbArray[fd].fileDE->dirBlocks * 2;

		reloadCurrentDir(dir[0]);

		free(newBuffer);
		newBuffer = NULL;
		

    }
    // if the number of remaining bytes to be written is greater than or equal to the chunk size 
    // to determine if there are enough bytes to write a whole chunk of data.

    if(numRemainingBytes >= B_CHUNK_SIZE) {

		int numBlocksToWrite = numRemainingBytes/B_CHUNK_SIZE;		
		// temporary buffer to hold the data for a whole number of chunks
		char *tempBuffer = malloc(numBlocksToWrite * B_CHUNK_SIZE);
        // Copy data from the original buffer to the temporary buffer
		memcpy(tempBuffer, buffer, numBlocksToWrite * B_CHUNK_SIZE);
        // Write the data to disk
        numBlocksWritten += LBAwrite(tempBuffer, numBlocksToWrite, 
							fcbArray[fd].currentBufferOffset + fcbArray[fd].fileDE->location);
		
        numRemainingBytes -= numBlocksToWrite * B_CHUNK_SIZE;

        int newFileSize = numBlocksToWrite * B_CHUNK_SIZE;
		int sizeOffset = fcbArray[fd].fileSize - fcbArray[fd].fileOffset;	
        // Update file offset and buffer offset
		fcbArray[fd].fileOffset += numBlocksToWrite * B_CHUNK_SIZE;
		fcbArray[fd].currentBufferOffset = numBlocksToWrite * B_CHUNK_SIZE;
        // Update current block
        fcbArray[fd].currentBlock += numBlocksToWrite;

		free(tempBuffer);
		tempBuffer = NULL;
    }

	// as long as there are bytes remaining to be written and the remaining bytes are less than the chunk size
	
    while(numRemainingBytes < B_CHUNK_SIZE && numRemainingBytes > 0) {

		// location within the buffer where the data will be written.
		int bufferLocation;

		// temporary buffer for our buffer
		char *tempBuffer = malloc(B_CHUNK_SIZE);
        // Get the offset of the block to write
		int blockOffset = blocksNeeded(fcbArray[fd].fileOffset) - 1;
		// check if the file offset is at the beginning of a new block and the offset is greater than or equal 
        // to B_CHUNK_SIZE. If so, increment blockOffset by 1 to move to the next block.

		if(fcbArray[fd].fileOffset % B_CHUNK_SIZE == 0 && fcbArray[fd].fileOffset >= B_CHUNK_SIZE)
			{
				blockOffset += 1;
			}
		// Copy the initial content to temporary buffer
		LBAread(tempBuffer, 1, blockOffset + fcbArray[fd].fileDE->location);
        // Determine the location within the buffer to write the data
		if(fcbArray[fd].fileOffset < B_CHUNK_SIZE)
			{
				bufferLocation = fcbArray[fd].fileOffset;
			}
		else if(fcbArray[fd].fileOffset >= B_CHUNK_SIZE)
			{
				bufferLocation = fcbArray[fd].fileOffset % B_CHUNK_SIZE;
			}

		int bytesToWrite;
        // Determine the number of bytes to write based on the remaining bytes and the 
        // space left in the current block
		if(numRemainingBytes > B_CHUNK_SIZE - bufferLocation)
			{
				bytesToWrite = B_CHUNK_SIZE - bufferLocation;
			}
		else
			{
				bytesToWrite = numRemainingBytes;
			}
        // Copy the data from the user buffer to the temporary buffer
		memcpy(tempBuffer + bufferLocation, buffer, bytesToWrite);
		fcbArray[fd].currentBufferOffset = bytesToWrite;
        // Move the user buffer to the next location to write
		buffer += fcbArray[fd].currentBufferOffset;
        int numBlocksWritten = LBAwrite(tempBuffer, 1, 
						blockOffset + fcbArray[fd].fileDE->location);
		fcbArray[fd].fileOffset += bytesToWrite;
        // Update the total number of bytes written
		totalBytes += bytesToWrite;
        numRemainingBytes -= bytesToWrite;
        // Free the temporary buffer used to store the data for this block
		free(tempBuffer);

		
    }
    // If the file buffer is NULL, allocate memory for it
	if(fcbArray[fd].buf == NULL) {
        fcbArray[fd].buf = malloc(B_CHUNK_SIZE);
    }
	// Reset the current buffer offset to 0 for the next write operation
	fcbArray[fd].currentBufferOffset = 0; 
	fcbArray[fd].fileDE->fileSize += count;
	

	return (totalBytes);
	}



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
		return (-1); 					//invalid file descriptor
		}
		
	return (0);	//Change this
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{

	}

int fs_rmdir(const char *pathname)
{
    PP *parseInfo = parsePath(pathname);

    if (parseInfo == NULL)
        return -1; // Directory not found or some unexpected error

    if (strcmp(pathname, "/") == 0)
    {
        return -1; // Should not delete root
    }

    if (strcmp(parseInfo->parentDirPtr[parseInfo->index].fileType, "d") != 0)
        return -1; // If it is not a directory

    fdDir *dirp = fs_opendir(pathname);

    struct fs_diriteminfo *di = fs_readdir(dirp);

    while (di != NULL)
    {

        if (di->d_name[0] == '.' || strcmp(di->d_name, "..") == 0 || strcmp(di->d_name, "") == 0) // Check the '.', '..' and empty entries in a directory
            di = fs_readdir(dirp);
        else
        {
            return -1; // directory not empty
        }
    }

    freeBlocks(parseInfo->parentDirPtr[parseInfo->index].location, blocksNeeded(parseInfo->parentDirPtr[parseInfo->index].fileSize));

    strcpy(parseInfo->parentDirPtr[parseInfo->index].fileName, "");
    parseInfo->parentDirPtr[parseInfo->index].fileSize = 0;
    parseInfo->parentDirPtr[parseInfo->index].location = -1;
    parseInfo->parentDirPtr[parseInfo->index].dirBlocks = 0;

    // write the directory changes to disk
    writeDirToVolume(parseInfo->parentDirPtr);

    if (strcpy(parseInfo->path, currPath) == 0)
    {

        reloadCurrentDir(parseInfo->parentDirPtr);
    }

    return 0;
}

int fs_delete(char *filename)
{
    PP *parseInfo = parsePath(filename);

    if (parseInfo == NULL)
        return -1; // file not found

    if (strcmp(parseInfo->parentDirPtr[parseInfo->index].fileType, "f") != 0)
        return -1; // If not a file

    strcpy(parseInfo->parentDirPtr[parseInfo->index].fileName, "");
    parseInfo->parentDirPtr[parseInfo->index].location = -1;
    parseInfo->parentDirPtr[parseInfo->index].fileSize = 0;
    parseInfo->parentDirPtr[parseInfo->index].dirBlocks = 0;

    // Use freeTheBlocks(location, blocksNeeded(size of file))
    freeBlocks(parseInfo->parentDirPtr[parseInfo->index].location, blocksNeeded(parseInfo->parentDirPtr[parseInfo->index].fileSize));

    return 0;
}

// return 1 if file, 0 otherwise
int fs_isFile(char *filename)
{

    if (CheckFileOrDir(filename) == -2)
    {
        printf("From fs_isFile: Invalid pathname provided: Returning zero\n");
        return 0; // this means the pathname provided is invalid
    }
    else if (CheckFileOrDir(filename) == -1)
    {
        printf("From fs_isFile: Not a file or directory: Returning zero\n");
        return 0; // not a file or directory
    }
    else if (CheckFileOrDir(filename) == 0)
    {
        return 1; // is a file
    }
    else
    {
        printf("From fs_isFile: Is a directory and not a file: Returning zero\n");
        return 0; // is a directory
    }
}

// return 1 if directory, 0 otherwise
int fs_isDir(char *pathname)
{
    // check if the pathname is a valid path that leads to a valid directory
    // call helper routine
    if (CheckFileOrDir(pathname) == -2)
    {
        printf("From fs_isDir: Invalid pathname provided: Returning zero\n");
        return 0; // this means the pathname provided is invalid
    }
    else if (CheckFileOrDir(pathname) == -1)
    {
        printf("From fs_isDir: Not a file or directory: Returning zero\n");
        return 0; // not a file or directory
    }
    else if (CheckFileOrDir(pathname) == 0)
    {
        printf("From fs_isDir: Is a file and not a directory: Returning zero\n");
        return 0; // is a file
    }
    else
    {
        return 1; // is a directory
    }
}