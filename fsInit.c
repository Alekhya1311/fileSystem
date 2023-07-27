
/**************************************************************
* Class:  CSC-415-0# Fall 2022
* Names: Justin Adriano, Anudeep Katukojwala, Kristian Goranov, Pranjal Newalkar
* Student IDs: 921719692 922404701 922003001 922892162
* GitHub Name: pranjal2410
* Group Name:
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"
#include "fsUtils.h"

// #define NUMBER_OF_BLOCKS 10000000
#define MAGIC_NUMBER 0x46616E74466F7572 //FantFour in hex


int initFreeSpace(int startingBlk, int numOfBlks, int blkSize)
{
    struct VCB vcb;

    int bitmapSize = (5 * blkSize);
    bitMapPointer = malloc(bitmapSize); //Freed in close.

    if (bitMapPointer == NULL)
    {
        printf("Error: allocatin memory for bitmap, initFreeSpace\n");
        return (-1);
    }

    vcb.FreeSpace = startingBlk;
    //  0 : bitmap[i] = free
    //  1 : bitmap[i] = occupied
    for (int i = 0; i < bitmapSize; i++)
    {
        bitMapPointer[i] = 0x00;
    }

    // set bits for VCB first bit block 0 to not free,
    //set bits 1-5 to not free for bitMap. 2442 bytes. 5 block.
    //11111100 = 0xFC = 252 decimal
    bitMapPointer[0] = 0xFC; // VCB start 0. Freespace 1-5 block.

    printf("Free space initialized......\n");

    return 1; // location of free space bit map
}



//declared static for now, not in .h, just a helper function for creating root.
//later this function will allocate files and other directories.
//return lba location. //change to return directory
// static int initRootDirectory(char *name, char *type, DE * dirEntry, DE * parent);
static int initRootDirectory(char *name, char *type, DE * dirEntry, DE** parent)
{
                              
        int dirSize = NUM_DE_IN_DIR * sizeof(DE); //in bytes.


        for (int i = 0; i < NUM_DE_IN_DIR; i++)
        {
            dir[i] = malloc(sizeof(DE)); //need to free
        }

        //set filenames to empty to check if DE is empty.
        for (int i = 2; i < NUM_DE_IN_DIR; i++)
        {
            dir[i]->fileName[0] = '\0';
        }

        uint64_t blocksNeededForDir = blocksNeeded(dirSize);

        //starting block of dir directory.
        int location = allocateFreeSpace(dirSize);
        printf("allocate freespace 1\n");
        //printf("rootLocation = %d\n", location);

        //"." directory
        strcpy(dir[0]->fileName, ".");
        dir[0]->fileSize = (dirSize); //2880
        strcpy(dir[0]->fileType, "d");    //0x00; //0 denotes directory
        dir[0]->blocksSpanned = blocksNeededForDir;
        time(&(dir[0]->created));
        time(&(dir[0]->lastModified));
        dir[0]->location = location;

        //".." directory
        strcpy(dir[1]->fileName, "..");
        dir[1]->fileSize = (dirSize);  //2880
        strcpy(dir[1]->fileType, "d"); //0x00; //0 denotes directory
        dir[1]->blocksSpanned = blocksNeededForDir;
        time(&(dir[1]->created));
        time(&(dir[1]->lastModified));
        dir[1]->location = location;


        setTheBlocks(location, blocksNeededForDir);

        unsigned char *buffer = malloc(blocksNeededForDir * sizeOfBlock);

        //copy array of DE, into bytes, for compatability with lbaWrite.
        unsigned char *buffLocation = buffer;
        for (int i = 0; i < NUM_DE_IN_DIR; i++)
        {
            memcpy(buffLocation, dir[i], sizeof(DE));
            buffLocation += sizeof(DE);
        }


        //root directory is commited to disk.
        uint64_t blocksRead = LBAwrite(buffer, blocksNeededForDir, location); 

       
        currPath = "/"; //set current path to root for starting.
        currPathLen = strlen(currPath) +1;
        

        free(buffer);
        buffer = NULL;

        return location;
}

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize)
{

    //globals for use of blocksize and numberof blocks
    blocksInVolume = numberOfBlocks;
    sizeOfBlock = blockSize;

    //allocate space for VCB.
    vcbPtr = malloc(sizeof(VCB)); //global for file.
    if (vcbPtr == NULL)
    {
        printf("Error allocating memory for vcb in initFileSystem (-1)\n");
        return -1;
    }

    //read block 0 into buffer.
    void *buffer = malloc(MINBLOCKSIZE); //will receive block 0;
    // printf("allocating buffer\n");
    if (buffer == NULL)
    {
        printf("Error allocating buffer, fsshell.c , initFileSystem()\n");
        return -1;
    }

    //load block 0 into buffer for verification of volume intialization.
    uint64_t blocksRead = LBAread(buffer, 1, 0);
    // printf("lbaRead: %ld\n", blocksRead);
    if (blocksRead == -1)
    {
        printf("Error reading block, fsinit.c, initFileSystem (-1)\n");
        return -1;
    }

    //VCB pointing to the first block in buffer.
     memcpy(vcbPtr, buffer, sizeof(VCB));

    if (vcbPtr->magicNumber == MAGIC_NUMBER)
    {
        printf("magic number matches : volume initialized\n");

        //bringing bitmap into memory double malloc do this if it is already initialized
        int bits = 8; //8 bits in a byte
        int freeSpaceSize = (numberOfBlocks + (bits - 1)) / bits; //ceiling size of how many bytes are needed to store free space.

        int blocksNeededForFreeSpace = blocksNeeded(freeSpaceSize);

        bitMapPointer = malloc(blocksNeededForFreeSpace * blockSize);
        if (LBAread(bitMapPointer, blocksNeededForFreeSpace, vcbPtr->FreeSpace) == -1)
        {
            printf("Error: LBAread freePace, initFileSystem(), FS already initialized.\n");
            return -1;
        }

        //set dir(current directory) to root directory from volume.
        int blocksSpanned = blocksNeeded((NUM_DE_IN_DIR * sizeof(DE)));
        int buffSize = blocksSpanned * BLOCK_SIZE;
        char * rootBuffer = malloc(buffSize * sizeof(char));

        for (int i = 0; i < NUM_DE_IN_DIR; i++)
        {
            dir[i] = malloc(sizeof(DE)); //need to free in close.
        }


        uint64_t blocksRead = LBAread(rootBuffer, blocksSpanned, vcbPtr->root);
        if (blocksRead != blocksSpanned)
        {
            printf("Error lbaREad fsInit.c, initFileSystem, load root to current dir\n");
            return -1;
        }

        unsigned char *buffLocation = rootBuffer;
        for (int i = 0; i < NUM_DE_IN_DIR; i++)
        {
            if (memcpy(dir[i], buffLocation, sizeof(DE)) == NULL)
            {
                printf("Error memcpy in fsInit.c, initFileSystem copy buffer to dir\n");
            }
            buffLocation += sizeof(DE);
        }

        //set curr path string to root.
        currPath = "/";
        currPathLen = strlen(currPath) + 1;

        free(rootBuffer);
        buffer = NULL;
    }
    else
    {

        printf("magic number does not match: volume not initialized\n");
        printf("initializing volume:\n");

        //VCB values
        vcbPtr->magicNumber = MAGIC_NUMBER;
        vcbPtr->blockSize = blockSize;                    //512
        vcbPtr->totalBlocks = numberOfBlocks * blockSize; //10,000,000 //change this its number of bytes in volume.
        //need 19,532 blocks to represent bitMap.

        //Free space initialized to block 1;
        //Free space start on block 1. 1-5 reserved.
        printf("initializing FreeSpace\n");
        int freeSpaceLoc = initFreeSpace(vcbPtr->FreeSpace, vcbPtr->totalBlocks, vcbPtr->blockSize);
        if (freeSpaceLoc == -1)
        {
            printf("Error issue initializing freeSpace, initFileSystem\n");
            return -1;
        }
        vcbPtr->FreeSpace = freeSpaceLoc;
        vcbPtr->root = 6; //when ready freeSpace allocation to get this value.

        printf("initializing Root\n");
        
        //have root return location and set vcb-> root.
        initRootDirectory("root", "d", NULL, NULL); // will eventually return starting block of root.

        // write VCB to block 0
        if (LBAwrite(vcbPtr, 1, 0) == -1)
        {
            printf("lbaWrite error fsinit.c, initFilesystem\n");
        }

    } //END ELSE.

    printf("filesystem initialized.\n");
    return 0;
} //END initFileSystem

void exitFileSystem()
{

    // write freeSpace to disk.
    if (LBAwrite(bitMapPointer, 5, 1) == -1)
    {
        printf("Error: lbawrite not successful, fsinit.c, initfreespace\n");
    }

    //free resources
    free(bitMapPointer);
    bitMapPointer = NULL;

    //dir or currentDirectory
    for (int i = 0; i < NUM_DE_IN_DIR; i++) // do in close.
    {
        free(dir[i]);
        dir[i] = NULL;
    }
    printf("System exiting\n");
}
