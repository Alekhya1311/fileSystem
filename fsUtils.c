/**************************************************************
* Class:  CSC-415-0# Fall 2022
* Names: Justin Adriano, Anudeep Katukojwala, Kristian Goranov, Pranjal Newalkar
* Student IDs: 921719692 922404701 922003001 922892162
* GitHub Name: pranjal2410
* Group Name:
* Project: Basic File System
*
* File: fsUtils.h
*
* Description: Contains helper functions structs and 
* necessary global variables shared between code in the file
* system.
*
**************************************************************/

#include "fsUtils.h"
#include "fsLow.h"
#include <time.h>

//How many blocks are needed to store a data of size in bytes on to the volume.
int blocksNeeded(int size)
{  
    if(size == 0)
        {
            return 1;
        }
    
    int blocksNeeded = (size + (vcbPtr->blockSize - 1)) / (vcbPtr->blockSize);
   
    return blocksNeeded;
}

//helper routine to find if the bit is set or not
int isTheBitFree(unsigned char input, int bitLocationInByte)
{
    unsigned char hexValue;
    switch (bitLocationInByte)
    {
        case 0:
            hexValue = 0x80;
            break;
        case 1:
            hexValue = 0x40;
            break;
        case 2:
            hexValue = 0x20;
            break;
        case 3:
            hexValue = 0x10;
            break;
        case 4:
            hexValue = 0x08;
            break;
        case 5:
            hexValue = 0x04;
            break;
        case 6:
            hexValue = 0x02;
            break;
        case 7:
            hexValue = 0x01;
            break;
    }

    if ((input & hexValue) == hexValue)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

///SET BITS FOR FREE AND USED BLOCKS
unsigned char hexValueOfSetBit(int offset){
    unsigned char hexValue;
    switch(offset){
        case 0:
            hexValue = 0x80;
            break;
        case 1:
            hexValue = 0x40;
            break;
        case 2:
            hexValue = 0x20;
            break;
        case 3:
            hexValue = 0x10;
            break;
        case 4:
            hexValue = 0x08;
            break;
        case 5:
            hexValue = 0x04;
            break;
        case 6:
            hexValue = 0x02;
            break;
        case 7:
            hexValue = 0x01;
            break;

    }
    return hexValue;
}

unsigned char hexValueOfClearBit(int offset){
    unsigned char hexValue;
    switch(offset){
        case 0:
            hexValue = 0x7F;
            break;
        case 1:
            hexValue = 0xBF;
            break;
        case 2:
            hexValue = 0xDF;
            break;
        case 3:
            hexValue = 0xEF;
            break;
        case 4:
            hexValue = 0xF7;
            break;
        case 5:
            hexValue = 0xFB;
            break;
        case 6:
            hexValue = 0xFD;
            break;
        case 7:
            hexValue = 0xFE;
            break;

    }
    return hexValue;
}

void freeTheBlocks(int startBlock, int numberOfBlocks){
    int indexIntoBitMap = startBlock/8;
    int currentIndex = indexIntoBitMap;
    int offsetIntoIndex = startBlock%8;
    int currentOffset = offsetIntoIndex;
    int trackRemainingBlocks = numberOfBlocks;
    //loop to set the free blocks
    while(trackRemainingBlocks!=0){
        //int curr = bitMapPointer[currentIndex];
        bitMapPointer[currentIndex] &= hexValueOfClearBit(currentOffset);
        currentOffset++;
        trackRemainingBlocks--;
        if(currentOffset>=8){
            currentIndex++;
            currentOffset=0;
        }
    }
}

void setTheBlocks(int startBlock, int numberOfBlocks){
    int indexIntoBitMap = startBlock/8;
    int currentIndex = indexIntoBitMap;
    int offsetIntoIndex = startBlock%8;
    int currentOffset = offsetIntoIndex;
    int trackRemainingBlocks = numberOfBlocks;
    //loop to set the free blocks
    while(trackRemainingBlocks!=0){
        int curr = bitMapPointer[currentIndex];
        bitMapPointer[currentIndex] |= hexValueOfSetBit(currentOffset);
        currentOffset++;
        trackRemainingBlocks--;
        if(currentOffset>=8){
            currentIndex++;
            currentOffset=0;
        }
    }
}
//END - SET BITS FOR FREE AND USED BLOCKS

//block numbers start from 0th block.
int allocateFreeSpace(int size)
{
    //find the number of blocks of free space needed
    int numOfBlocks;

    //for now we hardcoded the value to 2442
    int lengthOfFreeSpaceMap = 2442; //change the hard coding.

    if (size % 512 == 0)
    {
        numOfBlocks = (size / 512);
    }
    else
    {
        numOfBlocks = (size / 512) + 1;
    }

    printf("\nNumber of blocks requested: %d\n", numOfBlocks);

    int trackNumOfBlocks = numOfBlocks;
    int blockNumber = 0;
    int returnBlockNumber = 0;
    for (int i = 0; i < lengthOfFreeSpaceMap; i++)
    {
        int temp = bitMapPointer[i];
        printf("From allocateFreeSpace: Current Index in bitMapPointer Array: %d\n", temp);

        for (int j = 0; j < 8; j++)
        {
            int helperVar = isTheBitFree(temp, j);
            if (helperVar != 0)
            {
                trackNumOfBlocks = numOfBlocks;
                blockNumber++;
            }
            else
            {
                if (trackNumOfBlocks == numOfBlocks)
                {
                    returnBlockNumber = blockNumber;
                }
                trackNumOfBlocks--;
                blockNumber++;
                if (trackNumOfBlocks == 0)
                {
                    return (returnBlockNumber); // made change here
                }
            }
        }
    }
    if (trackNumOfBlocks > 0)
    {
        return -1;
    }
}

//give a free directory entry from parent directory
//dont forget parent needs to be written to disk to hold changes.
//supply name of directory, free direcotyr entry, and parent DE of directory being made.
void createDir(char * name, DE * dirEntry, DE * parent) {

    DE  directory[NUM_DE_IN_DIR];
    
        
    int blocksSpanned = blocksNeeded((NUM_DE_IN_DIR * sizeof(DE)));
    int buffSize = blocksSpanned * sizeOfBlock;
    unsigned char * buffer = malloc(buffSize * sizeof(char));

    int dirSize = NUM_DE_IN_DIR * sizeof(DE);
    int location = allocateFreeSpace(dirSize);

    //set empty DE passed, that will live in parent dir.
    dirEntry->location = location;
    dirEntry->fileSize = dirSize;
    dirEntry->blocksSpanned = blocksSpanned;
    strcpy(dirEntry->fileName, name); //make sure someone doesnt pass long name.
    strcpy(dirEntry->fileType, "d");
    time(&(dirEntry->created));
    time(&(dirEntry->lastModified));
    
    //pointer to self
    directory[0].location = location;
    printf("mkdir -- location to write dir to: %d\n", directory[0].location);
    directory[0].fileSize = (dirSize); //2880
    directory[0].blocksSpanned = blocksSpanned;
    strcpy(directory[0].fileName, ".");
    
    strcpy(directory[0].fileType, "d");    //0x00; //0 denotes directory
    time(&(directory[0].created));
    time(&(directory[0].lastModified));

    
    //pointer to parent
    strcpy(directory[1].fileName, "..");
    directory[1].fileSize = parent->fileSize; //2880
    strcpy(directory[1].fileType, "d");    
    directory[1].blocksSpanned = parent->blocksSpanned;
    directory[1].created = parent->created;
    time(&(directory[1].lastModified));
    directory[1].location = parent->location;

    //initialize file name to see if empty
    for (int i = 2; i < NUM_DE_IN_DIR; i++)
    {
        directory[i].fileName[0] = '\0';
        directory[i].blocksSpanned = -1;
        directory[i].fileSize = 0;
        directory[i].location = -1;
        directory[i].fileType[0] = '\0';
        
    }

    //copy dir into buffer so fits into blocks of 512
    //copy buff to ParseDir
    unsigned char *buffLocation = buffer;
    for (int i = 0; i < NUM_DE_IN_DIR; i++)
    {
        memcpy(buffLocation, &directory[i], sizeof(DE));
        buffLocation += sizeof(DE);
        
    }


    // write directory to volume
    int blocksWritten = LBAwrite(buffer, blocksSpanned, location);
    if (blocksWritten != blocksSpanned)
    {
        printf("error: writing to volume in mfs.c, createDir()\n");
    }

    //mark freespace used to create dir.
    setTheBlocks(location, blocksSpanned);

    free(buffer);
    buffer = NULL;
    printf("\nEND createDir()------------------------------------\n");
}

//if directories become dynamic will have to ask for new memory location and free old ones.
int writeDirToVolume (DE * dirToWrite) {
    int blocksSpanned = dirToWrite[0].blocksSpanned;
    
    int buffSize = dirToWrite[0].blocksSpanned * sizeOfBlock;
    
    unsigned char * buffer = malloc(buffSize * sizeof(char));
    int dirSize = dirToWrite[0].fileSize;
    
     unsigned char *buffLocation = buffer;

    memcpy(buffer, dirToWrite, dirSize);

    // // write directory to volume
    int location = dirToWrite[0].location; //should be "." DE of directory pointing to self.
    
    int blocksWritten = LBAwrite(buffer, blocksSpanned, location);
    if (blocksWritten != blocksSpanned)
    {
        printf("error: writing to volume in mfs.c, createDir()\n");
    }

    free(buffer);
    buffer = NULL;
    
    return 0;
}

int reloadCurrentDir(DE * directory) {
    printf("\nreloadCurrentDir() ---------------------\n");
        
        int blocksSpanned = directory[0].blocksSpanned; 
        int buffSize = blocksSpanned * sizeOfBlock;
        char * buffer = malloc(buffSize * sizeof(char));

        uint64_t blocksRead = LBAread(buffer, blocksSpanned, directory[0].location);
        if (blocksRead != blocksSpanned)
        {
            printf("Error lbaREad fsInit.c, initFileSystem, load root to current dir\n");
            return -1;
        }
        
        unsigned char * buffLocation = buffer;
        for (int i = 0; i < NUM_DE_IN_DIR; i++)
        {
            if (memcpy(dir[i], buffLocation, sizeof(DE)) == NULL)
            {
                printf("Error memcpy in fsInit.c, initFileSystem copy buffer to dir\n");
                return -1;
            }
            buffLocation += sizeof(DE);
        }
        return 0;
}


//get information on a given path from PPI struct.
PPI * parsePath(const char *pathname) {

    if(pathname == NULL ||strlen(pathname) == 0) {
        printf("Invalid Path - empty or null\n");
        return NULL;
    }

    //reset previous values in struct.
    parseInfo.parentDirPtr = NULL;
    parseInfo.parentDirName = NULL;
    parseInfo.index = -1;
    parseInfo.exists = -2;
    strcpy(parseInfo.fileType, "");
    strcpy(parseInfo.name, "");
    parseInfo.path = NULL;

    
    //copy path so it wont be modified by strtok
    char path[strlen(pathname) + 1];
    strcpy(path, pathname); // error check.

    int absolutePath = pathname[0] == '/';

    //path not empty check first element if / then its starting at root
    // Absolute path and current directory is not root.
    if(absolutePath && strcmp(pathname, currPath) == 0) {
        // load root. copy into parseDir.
        int blocksSpanned = blocksNeeded((NUM_DE_IN_DIR * sizeof(DE)));
        int buffSize = blocksSpanned * BLOCK_SIZE;
        char buffer[buffSize];
        
        
        uint64_t blocksRead = LBAread(buffer, blocksSpanned, vcbPtr->root);
        if( blocksRead != blocksSpanned) {
            printf("Error lbaREad mfs.c, parsepath() 22222222\n");
            return NULL;
        }

        //copy buff to ParseDir
        if(memcpy(parseDir, buffer, sizeof(parseDir)) == NULL) {
            printf("Error memcpy in mfs.c, parsePath\n");
        }
    }
    else 
    {
      //we must maintain a current d
      // irectory. root is loaded as starting current directory.

      //   copy currentDir into parseDir to work with and change in case its
      //   a bad path we can scrap it without loding current dir.
      for (int i = 0; i < NUM_DE_IN_DIR; i++)
      {
          parseDir[i].location = dir[i]->location;
          parseDir[i].fileSize = dir[i]->fileSize;
          parseDir[i].blocksSpanned = dir[i]->blocksSpanned;
          strcpy(parseDir[i].fileName, dir[i]->fileName);
          strcpy(parseDir[i].fileType, dir[i]->fileType);
          parseDir[i].lastModified = dir[i]->lastModified;
          parseDir[i].created = dir[i]->created;
      }
    }

    //tokenize the path
    char tokenizedPath[50][256]; //max 20 items in in a path.
    int numTokens = 0;
    char * token = strtok(path, "/");

    while (token != NULL)
    {
        strcpy(tokenizedPath[numTokens], token);
        numTokens++;
        // printf("index: %d ,token : %s\n",numTokens, token);
        token = strtok(NULL, "/");

    }

    
    //search path.
    int found = 0; //0 not found, 1 found;
    int foundIndex = -1; //j index in for loop, corresponds to inex in directory we are searching
    int endPathIndex = -1;
    parseInfo.exists = -1; //if last element is not found and set to 0 last element was not found.
    
    int i;
    for (i = 0; i < numTokens; i++) 
    {
        int j;
        for (j = 0; j < NUM_DE_IN_DIR && found == 0; j++)
        {
            //filenames match
            if(strcmp(parseDir[j].fileName, tokenizedPath[i]) == 0) 
            {
                //make sure all filename tokens in path except for the last one are directories 
                if (((numTokens - i) > 1) && (strcmp(parseDir[j].fileType, "d") == 0))
                {
                    found = 1;
                    foundIndex = j;
                }
                else if ((numTokens - i) == 1) //last token so we can mark if it exists or not.
                {
                    printf("last token found : %s\n", parseDir[j].fileName);
                    parseInfo.exists = 0;
                    found = 1;
                    foundIndex = j;
                    endPathIndex = j;
                }
                else{
                    //token found but its a file when it should be a directory.
                    //except for last token in path.
                    found = 0;

                }
            }

        }//End for

        //path is valid except for last term.
        //the last token in the path name does not exist, so file or dir does not exist.
        //but still load 2nd to last directory.
        if ((numTokens - i) == 1 && found == 0)
        {
            found = 1; //not actally found, but still want to load 2nd to last directory.
        }

        //Went through directory and did not find a match for particular token.
        if(found == 0) {
            printf("invalid path token : %s\n", tokenizedPath[i]);
            return NULL;
        }

        //token found get dir
        //load directory for searching except for last token in path.
        if((numTokens - i) > 1) {
            if (foundIndex == -1)
            {
                return NULL;
            }

            // turn this into helper functio used twice.
            int blocksSpanned = blocksNeeded((NUM_DE_IN_DIR * sizeof(DE)));
            int buffSize = blocksSpanned * BLOCK_SIZE;
            char buffer[buffSize];

            uint64_t blocksRead = LBAread(buffer, blocksSpanned, parseDir[foundIndex].location);
            if (blocksRead != blocksSpanned)
            {
                printf("Error lbaREad mfs.c, parsepath()*****\n");
                return NULL;
            }

            //copy buff to ParseDir
            if (memcpy(parseDir, buffer, sizeof(parseDir)) == NULL)
            {
                printf("Error memcpy in mfs.c, parsePath 1111111\n");
            }
            printf("\n 2. PP DIR print dir in for loop path infor\n");
        }

        //reset found for next search
        found = 0;
        foundIndex = -1;

    }//END FOR.

    //final path information that that will be returned.
    //when path passed is / (root);
    if (strcmp(pathname,"/") == 0)
    {
        endPathIndex = 0; //will access 0th index of root dir "." , pointer to itself.
    }
    
    //if endpathe is -1. last element did no exist, file type will be blank, if path is "/" strtok delim does not count it as a token. 
    char* fileType = (endPathIndex > -1) ? parseDir[endPathIndex].fileType : "";

    strcpy(parseInfo.fileType, fileType);
    parseInfo.index = endPathIndex;
    parseInfo.parentDirPtr = parseDir;
    if(numTokens == 0) {
       //means only root was supplied "/".
       //delimeter is / so tokens will be 0.
       parseInfo.exists = 0;
    }
    
    //set name for last token, even if it doesnt exits. 
     char lastElementName[256] = "";
     if (numTokens > 0)
     {
         printf("token[i - 1] = %s\n",tokenizedPath[numTokens - 1] );
        strcpy(lastElementName, tokenizedPath[numTokens - 1]);
     }
     else {
         printf("last elemnt strcpy to root / \n");
         strcpy(lastElementName, "/");
     }
     
    //set name
    strcpy(parseInfo.name, lastElementName);


    int dotOrDotDot = -1; //if equals 0 . or .. is first element in path.
    //if relative path starting with .. replace .. with dir name.
    if (strcmp(pathname, "..") == 0)
    {
        dotOrDotDot = 0;

        //copy path so it wont be modified by strtok
        int lenOfCurrPath = strlen(currPath);
        char pathCopy[lenOfCurrPath + 1];
        strcpy(pathCopy, currPath); // error check.
        

        //tokenize the path
        char tokenCurrPath[50][256]; //max 20 items in in a path.
        int numCurrentTokens = 0;
        char *currToken = strtok(pathCopy, "/");
        

        while (currToken != NULL)
        {
            strcpy(tokenCurrPath[numCurrentTokens], currToken);
            numCurrentTokens++;
            currToken = strtok(NULL, "/");
        }

        // test display path.
        printf("1. numtokens: %d\n", numCurrentTokens);
        printf(" 1. display tokenized path\n");
        for (int i = 0; i < numCurrentTokens; i++)
        {
            printf("%s\n", tokenCurrPath[i]);
        }
        
    
        char * newPath;
        if(numCurrentTokens <= 1) {
            //.. is root "/"
            newPath = malloc(2 * sizeof(char));
            strcpy(newPath, "/");
            strcpy(parseInfo.name, "/");
            parseInfo.path = malloc(strlen(newPath) + 1);
            strcpy(parseInfo.path, newPath);   
        }


        if(numCurrentTokens > 1) {
            strcpy(parseInfo.name, tokenCurrPath[numCurrentTokens - 2]);
            printf("parse info name updated to : %s\n", parseInfo.name);
            
            //build new path.
            newPath = malloc(2 * sizeof(char));
            strcpy(newPath, "/");
            printf("copy newpath / : %s\n", newPath);

            for(int i = 0; i < numCurrentTokens - 2; i++){
               newPath = realloc(newPath, (strlen(newPath) + strlen(tokenCurrPath[i]) +1));
               strcat(newPath, tokenCurrPath[i]);

               if(i != numCurrentTokens - 3)
               {
                    strcat(newPath, "/");
               }
            }
            printf("copy newpath after for loop : %s\n", newPath);

            parseInfo.path = malloc(strlen(newPath) + 1);

            //update name
            strcpy(parseInfo.path, newPath);
        }

    }

    if(pathname[0] == '/') { //absolute path, if valid just use pathname.
       //copy pathname, dont modify original
       char pathnameCopy[strlen(pathname) + 1];
       strcpy(pathnameCopy, pathname);

       //create substring including / and last token to be ommited from path name.
       char substring[strlen(parseInfo.name) + 2];
       strcpy(substring, "/");
       strcat(substring, parseInfo.name);

       //removing matching substring to create path to parent dir.
       char * matchingSubString;
       int substringLen = strlen(substring) + 1;
       while (matchingSubString = strstr(pathnameCopy, substring))
       {
           *matchingSubString = '\0';
           strcat(pathnameCopy, matchingSubString);
       }
       

       //remove last token from string.

        parseInfo.path = malloc(strlen(pathnameCopy) + 1); //has a _+1 already for \0.
        strcpy(parseInfo.path, pathnameCopy);
    }
    else {
        //relative path, concat absolute path.
       if(numTokens > 1 && dotOrDotDot == -1) {
           parseInfo.path = malloc(strlen(pathname) + strlen(currPath) + 1);
           for (int i = 0; i < numTokens - 1; i++)
           {
               strcat(parseInfo.path, tokenizedPath[i]);
           }
       }
       else if (numTokens <= 1 && dotOrDotDot == -1) {
           parseInfo.path = malloc(strlen(pathname) + 1);
           strcpy(parseInfo.path, currPath);
       }
       
    }

   return &parseInfo;
}//END parsePath.



//create file
int createFile(char * filename, DE * dirEntry, DE * parent) {

    //allocate blocks for file. 5120 default. so we dont waste space
    int blocksSpanned = blocksNeeded(BLOCK_SIZE * DEFAULT_BLOCKS_ALLOC_FOR_FILE);

    int fileLocation = allocateFreeSpace(BLOCK_SIZE * DEFAULT_BLOCKS_ALLOC_FOR_FILE);
    printf("CREATE FILE: %s, loc: %d\n", filename, fileLocation);

    //modify dir entry no need to write file to disk, we have no data yet.
    //will have to write parent to disk for changes to take effect.
    dirEntry->location = fileLocation;
    dirEntry->fileSize = 0;
    dirEntry->blocksSpanned = blocksSpanned;
    strcpy(dirEntry->fileName, filename);
    strcpy(dirEntry->fileType, "f");

    //mark blocks as used in spaceAllocation
    setTheBlocks(fileLocation, blocksSpanned);   

    return 0;
}

void printParsePathReturn(PPI * pathInfo) {
    printf("\nPRINT PATHINFO *****************************\n");
	printf("exists : %d\n", pathInfo->exists);
	printf("index : %d\n", pathInfo->index);
	printf("filetype: %s\n", pathInfo->fileType);
	printf("name : %s\n", pathInfo->name);
	printf("path : %s\n", pathInfo->path);
	printf("\nparent dir*************\n");
	for(int i = 0; i < NUM_DE_IN_DIR; i++) {
		printf("\n");
		printf("Dfilename : %s\n", pathInfo->parentDirPtr[i].fileName);
		printf("Dlocation : %d\n", pathInfo->parentDirPtr[i].location);
		printf("Dfilesize: %d\n", pathInfo->parentDirPtr[i].fileSize);
		printf("Dblockspanned : %d\n", pathInfo->parentDirPtr[i].blocksSpanned);
		printf("DfileType : %s\n", pathInfo->parentDirPtr[i].fileType);
		printf("\n");
	}
	printf("\n end PRINT PATHINFO *****************************\n");
}


DE * findEmptyDE (DE * parentDir) {
    printf("in findEmptyDE\n");
    for ( int i = 0; i < NUM_DE_IN_DIR; i++)
    {
        if(strcmp(parentDir[i].fileName, "") == 0){
            return &parentDir[i];
        }
    }
    printf("NO empy DE in parent dir - return NULL, from findEmptyDE\n");
    return NULL;
}

void printDE(DE *de) {
    printf("\nDE >>>>>>>>>>>>>>>>\n");
		printf("Dfilename : %s\n", de->fileName);
		printf("Dlocation : %d\n", de->location);
		printf("Dfilesize: %d\n", de->fileSize);
		printf("Dblockspanned : %d\n", de->blocksSpanned);
		printf("DfileType : %s\n", de->fileType);

	printf("\nDE >>>>>>>>>>>>>>>>\n");
}

void printDir(DE * dir) {
    printf("\nparent dir*************\n");
	for(int i = 0; i < NUM_DE_IN_DIR; i++) {
		printf("\n");
		printf("Dfilename : %s\n", dir[i].fileName);
		printf("Dlocation : %d\n", dir[i].location);
		printf("Dfilesize: %d\n", dir[i].fileSize);
		printf("Dblockspanned : %d\n", dir[i].blocksSpanned);
		printf("DfileType : %s\n", dir[i].fileType);
		printf("\n");
	}
}

void printCurrentDir(DE * dir[]){
    printf("\ncurrent dir*************\n");
	for(int i = 0; i < NUM_DE_IN_DIR; i++) {
		printf("\n");
		printf("Dfilename : %s\n", dir[i]->fileName);
		printf("Dlocation : %d\n", dir[i]->location);
		printf("Dfilesize: %d\n", dir[i]->fileSize);
		printf("Dblockspanned : %d\n", dir[i]->blocksSpanned);
		printf("DfileType : %s\n", dir[i]->fileType);
		printf("\n");
    }
}

