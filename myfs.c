/**************************************************************
* Class:  CSC-415-02 Summer 2023
* Names: Saripalli Hruthika, Nixxy Dewalt, Alekya Bairaboina, Banting Lin 
* Student IDs: 923066687, 922018328, 923041428, 922404012
* GitHub Name: hru952
* Group Name: Zombies
* Project: Basic File System
*
* File: mfs.c
*
* Description: 
*
**************************************************************/

#include <stdio.h>
#include <string.h>
#include "mfs.h"
#include "fsLow.h"
#include "fsFunc.h"

int fs_mkdir(const char *pathname, mode_t mode)
{
    printf("\nWELCOME TO fs_mkdir()\n");
    PP *info = parsePath(pathname);
    
    if (info == NULL)
    {
        // either invalid path or dir by that name already exists.
        printf("invalid path. fs_mkdir, mrs.c\n");
        return -1;
    }
    if (info->exists == 0)
    {
        printf("dir by that name already exists. fs_mkdir, mrs.c\n");
        return -1;
    }
    
    //find empty DE in parent dir.
    printf("\nFinding empty DE\n");
    DE *emptyDE = findEmptyDE(info->parentDirPtr);
    if (emptyDE == NULL)
    {
        printf("directory full, fs_mkdir\n");
        return -1;
    }
    printf("\nFound empty DE\n");

    //create a directory
    createDir(info->name, emptyDE, info->parentDirPtr);
    printf("\n Created New Dir\n");

    DE * updatedParentDir = malloc(totDirEnt * sizeof(DE));
    
    memcpy(updatedParentDir, info->parentDirPtr, (totDirEnt * sizeof(DE)));
 
    int retWriteDir = writeDirToVolume(updatedParentDir);
    if (retWriteDir < 0)
    {
        printf("error writing directory to volume, mfs.c, mkdir\n");
        return -1;
    }

    //if current dir contains the modified directory entry. reload it.
    if (strcmp(info->path, currPath) == 0)
    {

        //then we need to reload current directory to reflect changes.
    int retReload = reloadCurrentDir(updatedParentDir);

    free(info->path);
    info->path = NULL;
        return retReload;
    }


    free(info->path);
    info->path = NULL;

    return 0;
} //End fs_mkdir


//Function to know if the directory entry is file or dir
int CheckFileOrDir(const char * pathname){

    PP * parentDir = parsePath(pathname);

    if(parentDir == NULL){
        return -2;
    }
    else{
        
        int result;
        if(strcmp(parentDir->parentDirPtr[parentDir->index].fileType, "f")==0){
            printf("This is a file\n");
            result = 0; //File
        }
        else if(strcmp(parentDir->parentDirPtr[parentDir->index].fileType, "d")==0){
            printf(" This is a directory\n");
            result = 1; //Directory
        }
        else{
            printf("Error! Not a file or directory\n");
            result = -1;
        }
        return result;
    }

    
}




// fs_opendir and fs_closedir below

DE openDirDe[totDirEnt]; //array used in fs_opendir

fdDir * fs_opendir(const char *pathname){
    //check if the given path exists or not
    int fileOrDirVal = CheckFileOrDir(pathname);


    if(fileOrDirVal==-2){
        printf("Pathname is invalid\n");
        return NULL;
    }
    else if(fileOrDirVal==-1){
        printf("Path is neither a file nor a directory\n");
        return NULL;
    }
    else if(fileOrDirVal==0){
        printf("Path is a not a directory\n");
        return NULL;
    }
    else if(fileOrDirVal==1){
        //Opening logic starts
        PP * parentDir = parsePath(pathname);
        printf("Directory opened: %s\n",parentDir->name);

        fdDir * openedDir= malloc(sizeof(fdDir));
        openedDir->dirEntryPosition = 0;
        // calculates the number of blocks needed to store the directory entries based on the size of 
        // each directory entry and the total number of directory entries
        int numBlocks = blocksNeeded((totDirEnt * sizeof(DE)));
        int bufferSize = numBlocks * BLOCK_SIZE;
        char buffer[bufferSize];

        uint64_t numBlocksRead = LBAread(buffer, numBlocks, parentDir->parentDirPtr[parentDir->index].location);
        if (numBlocksRead != numBlocks)
        {
            return NULL;
        }

        //copy the contents of the buffer into the openDirDe array. 
        if (memcpy(openDirDe, buffer, sizeof(openDirDe)) == NULL)
        {
            printf("Error in the memcpy operation\n");
        }
        openedDir->directoryStartLocation = openDirDe[0].location; 
        openedDir->openDirPointer = openDirDe;

        openedDir->d_reclen = sizeof(fdDir);
        printf("Size of fdDir: %ld\n", sizeof(fdDir));
        return openedDir;
}   

}

int fs_closedir(fdDir *dirp)
    {
        if(dirp == NULL)
            {
                return (-1);
            }

        free(dirp);
        dirp = NULL;
        
        return 0;
    }



