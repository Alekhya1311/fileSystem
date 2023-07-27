/**************************************************************
* Class:  CSC-415-0# Fall 2022
* Names: Justin Adriano, Anudeep Katukojwala, Kristian Goranov, Pranjal Newalkar
* Student IDs: 921719692 922404701 922003001 922892162
* GitHub Name: pranjal2410
* Group Name: Fantastic Four
* Project: Basic File System
*
* File: fsUtils.h
*
* Description: Contains helper functions structs and 
* necessary global variables shared between code in the file
* system.
*
**************************************************************/

#ifndef _FSUTILS_H
#define _FSUTILS_H

#define NUM_DE_IN_DIR 50 //default number of directory entries in a directory.
#define BLOCK_SIZE 512
#define CURRENT_PATH_MAX_SIZE 1000
#define DEFAULT_BLOCKS_ALLOC_FOR_FILE 4

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>

int blocksInVolume;
int sizeOfBlock;

typedef struct VCB {
    long int magicNumber; //volume identifier
    int totalBlocks;
    int blockSize;
    int FreeSpace;
    int root;
} VCB;

 
typedef struct DirectoryEntry {
    int location;
    int fileSize;
    int blocksSpanned;
    char fileName[256];
    char fileType[2];

    time_t created;
    time_t lastModified;
} DE;

typedef struct parsePathInfo //for parsePath
{
    DE * parentDirPtr;
    char * parentDirName;
    int index; // index in directory;
    int exists; //0 exists, -1 not exists, for last file or dir in the path
    char fileType[2]; // "f" = file, "d"= dir, is a c string.
    char name [256]; //last file or directory name.
    char * path; //parent path. must free this when your done.
    

}PPI;

//Holds bitmap.
unsigned char * bitMapPointer;

VCB * vcbPtr; //VCB in memory.

//current directory.
//Set initRoot to have a starting point.
DE * dir[NUM_DE_IN_DIR];  //initially points to root.


//absolute path of dir(the current directory)
int currPathLen;
char * currPath; //free in close.

int isTheBitFree(unsigned char input, int bitLocationInByte);
unsigned char hexValueOfClearBit(int offset);
unsigned char hexValueOfSetBit(int offset);
void freeTheBlocks(int startBlock, int numberOfBlocks);
void setTheBlocks(int startBlock, int numberOfBlocks);
int allocateFreeSpace(int size);
int blocksNeeded(int size);
void createDir(char * name, DE * dirEntry, DE * parent);
int writeDirToVolume (DE * directory);
int reloadCurrentDir(DE * directory);
int createFile(char * filename, DE * dirEntry, DE * parent);

PPI parseInfo; //for parsePath
DE parseDir[NUM_DE_IN_DIR]; //for parsePath
PPI * parsePath(const char *pathname);


//helper TEst functions
void printParsePathReturn(PPI * pathInfo);
DE * findEmptyDE (DE * parentDir);
void printDE(DE *de);
void printDir(DE * dir);
void printCurrentDir(DE * dir[]);

#endif // _FSUTILS_H
