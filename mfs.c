
/**************************************************************
* Class:  CSC-415-0# Fall 2022
* Names: Justin Adriano, Anudeep Katukojwala, Kristian Goranov, Pranjal Newalkar
* Student IDs: 921719692 922404701 922003001 922892162
* GitHub Name: pranjal2410
* Group Name: Fantastic Four
* Project: Basic File System
*
* File: mfs.c
*
* Description: File system functions, to interact witht the shell
*
**************************************************************/
#include <stdio.h>
#include <string.h>
#include "mfs.h"
#include "fsLow.h"
#include "fsUtils.h"
#define DIRMAX_LEN 4096
#define MAX_P_SIZE 256
#define MAX_NUM_P 20

int fs_mkdir(const char *pathname, mode_t mode)
{
    
    PPI *info = parsePath(pathname);
    
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
    DE *emptyDE = findEmptyDE(info->parentDirPtr);
    if (emptyDE == NULL)
    {
        printf("directory full, fs_mkdir\n");
        return -1;
    }

    //create a directory
    createDir(info->name, emptyDE, info->parentDirPtr);

    DE * updatedParentDir = malloc(NUM_DE_IN_DIR * sizeof(DE));
    
    memcpy(updatedParentDir, info->parentDirPtr, (NUM_DE_IN_DIR * sizeof(DE)));
 
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



//helper function to know if the directory entry is file or dir
int fileOrDir(const char * pathname){

    PPI * parentDir = parsePath(pathname);

    if(parentDir == NULL){
        return -2;
    }
    else{
        
        int result;
        if(strcmp(parentDir->parentDirPtr[parentDir->index].fileType, "f")==0){
            printf("Its a file\n");
            result = 0; //indicates that it is a file
        }
        else if(strcmp(parentDir->parentDirPtr[parentDir->index].fileType, "d")==0){
            printf(" it is a dir\n");
            result = 1; //indicates that it is a directory
        }
        else{
            printf("not a file or dir.\n");
            printf("Error! Not a file or directory\n");
            result = -1;
        }
        return result;
    }

    
}

int fs_rmdir(const char *pathname) {
    PPI *parseInfo = parsePath(pathname);

    if (parseInfo == NULL)
        return -1; // Directory not found or some unexpected error

    if(strcmp(pathname, "/") == 0) {
        return -1; // Should not delete root
    }

    if (strcmp(parseInfo->parentDirPtr[parseInfo->index].fileType, "d") != 0)
        return -1; // If it is not a directory

    fdDir *dirp = fs_opendir(pathname);

    struct fs_diriteminfo *di = fs_readdir(dirp);

    while(di != NULL) {
        
        if(di->d_name[0] == '.' || strcmp(di->d_name, "..") == 0 || strcmp(di->d_name, "") == 0) // Check the '.', '..' and empty entries in a directory
            di = fs_readdir(dirp);
        else{
            return -1; // directory not empty

        }
    }
  

    freeTheBlocks(parseInfo->parentDirPtr[parseInfo->index].location, blocksNeeded(parseInfo->parentDirPtr[parseInfo->index].fileSize));

    strcpy(parseInfo->parentDirPtr[parseInfo->index].fileName, "");
    parseInfo->parentDirPtr[parseInfo->index].fileSize = 0;
    parseInfo->parentDirPtr[parseInfo->index].location = -1;
    parseInfo->parentDirPtr[parseInfo->index].blocksSpanned = 0;

    //write the directory changes to disk
    writeDirToVolume(parseInfo->parentDirPtr);

    if (strcpy(parseInfo->path, currPath) == 0) {

        reloadCurrentDir(parseInfo->parentDirPtr);
    }

    return 0;
}

// Directory iteration functions


DE openDirDE[NUM_DE_IN_DIR]; //used in fs_opendir

fdDir * fs_opendir(const char *pathname){
    //check if the given path exists or not
    int returnOfFileOrDir = fileOrDir(pathname);


    if(returnOfFileOrDir==-2){
        printf("Invalid pathname\n");
        return NULL;
    }
    else if(returnOfFileOrDir==-1){
        printf("Given path is not a file or directory\n");
        return NULL;
    }
    else if(returnOfFileOrDir==0){
        printf("Given path is a not a directory\n");
        return NULL;
    }
    else if(returnOfFileOrDir==1){

        printf("Begining or opendir\n");
        //exit(0);

        PPI * parentDir = parsePath(pathname);
        printf("Name of the directory opened: %s\n",parentDir->name);

        fdDir * openedDirectory = malloc(sizeof(fdDir));
        openedDirectory->dirEntryPosition = 0;
        //We probably have this directory we are trying to open
        //into ram. we need the pointer to that and assign that 
        //pointer to directoryStartLocation
        int blocksSpanned = blocksNeeded((NUM_DE_IN_DIR * sizeof(DE)));
        int buffSize = blocksSpanned * BLOCK_SIZE;
        char buffer[buffSize];

        uint64_t blocksRead = LBAread(buffer, blocksSpanned, parentDir->parentDirPtr[parentDir->index].location);
        if (blocksRead != blocksSpanned)
        {
            printf("Error lbaREad mfs.c, parsepath()*\n");
            return NULL;
        }

        //copy buff to ParseDir
        if (memcpy(openDirDE, buffer, sizeof(openDirDE)) == NULL)
        {
            printf("Error memcpy in mfs.c, parsePath 1111111\n");
        }

        openedDirectory->directoryStartLocation = openDirDE[0].location; //doubt check with team
        //(Should we keep track of the number of directories that we can keep open at a time.);
        //Should we add a way to say we have opened this directory

        openedDirectory->openDirPtr = openDirDE;

        openedDirectory->d_reclen = sizeof(fdDir);
        printf("Size of fdDir: %ld\n", sizeof(fdDir));
        printf("Last line of opendir\n");
        //exit(0);
        return openedDirectory;
}   

}



struct fs_diriteminfo *fs_readdir(fdDir *dirp)
    {
        // printf("we are here\n");
        // struct VCB *vcbPtr;
        // vcbPtr = malloc(sizeof(VCB));
        // int blockSize = vcbPtr->blockSize;

        // dirItem = malloc(sizeof(DE) * blockSize);

        // struct DirectoryEntry *DE;
        // DE = malloc(NUM_DE_IN_DIR * blockSize);
        // LBAread(DE, NUM_DE_IN_DIR, dirp->dirEntryPosition);

        // strcpy(dirItem->d_name, DE->fileName);
        // dirItem->d_reclen = dirp->d_reclen;
        // dirItem->fileType = DE->fileType;

        // dirp->dirEntryPosition += 1;

        // return dirItem;
        // dirp->index = 0;
        int index = dirp->dirEntryPosition;
        int validEntry = 0;
        int exit = 0;
        
        struct fs_diriteminfo *dirItem;
        dirItem = malloc(sizeof(struct fs_diriteminfo));

        while(validEntry == 0 && exit == 0)
        {
                
                //printf("%d %d \n", index, NUM_DE_IN_DIR);

                if(index == (NUM_DE_IN_DIR))
                    {
                        exit = -1;
                        return NULL;
                    }
                    
                if((strcmp(dirp->openDirPtr[index].fileName, "")) != 0)
                    {
                        strncpy(dirItem->d_name, dirp->openDirPtr[dirp->dirEntryPosition].fileName, 255);
                        //printf("file name %s\n", dirp->openDirPtr[dirp->dirEntryPosition].fileName);
                        //strcpy(dirItem->fileType, dirp->openDirPtr[dirp->dirEntryPosition].fileType );
                        dirItem->d_reclen = dirp->openDirPtr[dirp->dirEntryPosition].fileSize; 
                        // if(strcmp(dirp->openDirPtr[dirp->dirEntryPosition].fileType, "f") == 0) 
                        //     {
                        //     //    dirItem->fileType =  FT_REGFILE;        
                        //     }

                        validEntry = 1;
                        exit = -1;            
                    }
                index++;
        }
            // printf("outside while loop \n");
            dirp->dirEntryPosition = index;
           // printf("index %d\n", dirp->index);
            


        return dirItem;
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

//Misc directory functions
char cwdPath[256];

char * fs_getcwd(char *pathname, size_t size) {
    //TOD0: 
    int copySize = ((strlen(currPath) + 1) <= size) ? (strlen(currPath) + 1) : size;
    printf("getcwd size to copy : %d\n", copySize);
    strncpy(pathname, currPath,size );

   
    return pathname;

}

int fs_setcwd(char *pathname) {//linux chdir
     // set current working directory 
        
    PPI* cwd = parsePath(pathname);

    // currPath = "\0";

    // search . / ..
    // . : current | .. parent
    // if we see . replace with current directory
    // /foo/poo
    // .. -> parents dir 

    if(cwd->exists == 0)
        {
            if(strcmp(cwd->fileType, "f") == 0)
                {
                    printf("is a file\n");
                    return (-1);
                }
            else if(strcmp(cwd->fileType, "d") == 0) //update current directory to pathname directory
                {
                    
                    int blocksSpanned = blocksNeeded((NUM_DE_IN_DIR * sizeof(DE)));
                    int buffSize = blocksSpanned * BLOCK_SIZE;
                    char buf[buffSize];
                    
                    printf("lbaread location %d\n",cwd->parentDirPtr[parseInfo.index].location);
                    printf("pathinfor index %d\n", cwd->index);
                    //read dir from disk into buffer
                    //confirm these are good values. not reading  rood again location is different.
                    LBAread(buf, blocksSpanned, cwd->parentDirPtr[parseInfo.index].location);
                   
                    //copy contents of buffer into the current directory, to update current directory
                    char * buffptr = buf;
                    for (int i = 0; i < NUM_DE_IN_DIR; i++)
                    {
                      memcpy(dir[i], buffptr, sizeof(DE));
                      buffptr += sizeof(DE);
                        
                    }
                    
                    printf("\n print current dir in CD\n");
                    printCurrentDir(dir);
                    printf(" before concat parse path %s\n", parseInfo.path);
                    // strncat(cwd->path, "/", 256);
                    // // printf("parse path %s\n", parseInfo.path);
                    if (strcmp(cwd->name, ".") == 0 )
                    {
                        strcat(cwd->path, cwd->parentDirPtr[cwd->index].fileName);
                    }
                    else if (strcmp(cwd->name, "..") == 0 )
                    {
                        //get parent dir name
                        strcat(cwd->path, cwd->parentDirPtr[cwd->index].fileName);
                    }
                    
                    if((strlen(cwd->path) == 1) && (strcmp(cwd->name, "/") != 0)){
                        //parent dir is root. just concat name without adding "/"
                        strcat(cwd->path, cwd->name);
                    }
                    else if (strcmp(cwd->name, "/") != 0){ // root is its own parent, so dont add / and name to build path.
                        //add "/"
                        
                        printf("setcwd add / to path\n");
                        printf("name = %s \n", cwd->name);
                        strcat(cwd->path, "/");
                        strcat(cwd->path, cwd->name);
                    }
                    printf("after concat cwd->path %s\n", cwd->path);
                    currPath = malloc(strlen(cwdPath) + 1);
                    strcpy(currPath, cwd->path);
                    printf("crruent path after copy %s\n", currPath);

                }
        }
    else if(cwd->exists == -1)
        {
            printf("Error : Directory does not exists\n");
            return (-1);
        }

     return 0;
}  




//return 1 if file, 0 otherwise
int fs_isFile(char * filename){
    
    if(fileOrDir(filename)==-2){
        printf("From fs_isFile: Invalid pathname provided: Returning zero\n");
        return 0; //this means the pathname provided is invalid
    }
    else if(fileOrDir(filename)==-1){
        printf("From fs_isFile: Not a file or directory: Returning zero\n");
        return 0; //not a file or directory
    }
    else if(fileOrDir(filename)==0){
        return 1; //is a file
    }
    else{
        printf("From fs_isFile: Is a directory and not a file: Returning zero\n");
        return 0; //is a directory
    }
         
    
}

//return 1 if directory, 0 otherwise
int fs_isDir(char * pathname){
   //check if the pathname is a valid path that leads to a valid directory
    //call helper routine
    if(fileOrDir(pathname)==-2){
        printf("From fs_isDir: Invalid pathname provided: Returning zero\n");
        return 0; //this means the pathname provided is invalid
    }
    else if(fileOrDir(pathname)==-1){
        printf("From fs_isDir: Not a file or directory: Returning zero\n");
        return 0; //not a file or directory
    }
    else if(fileOrDir(pathname)==0){
        printf("From fs_isDir: Is a file and not a directory: Returning zero\n");
        return 0; //is a file
    }
    else{
        return 1; //is a directory
    }      

}


int fs_delete(char* filename) {
    PPI *parseInfo = parsePath(filename);

    if(parseInfo == NULL)
        return -1; // file not found

    if (strcmp(parseInfo->parentDirPtr[parseInfo->index].fileType, "f") != 0)
        return -1; // If not a file

    // Use freeTheBlocks(location, blocksNeeded(size of file))
    freeTheBlocks(parseInfo->parentDirPtr[parseInfo->index].location, blocksNeeded(parseInfo->parentDirPtr[parseInfo->index].fileSize));

    strcpy(parseInfo->parentDirPtr[parseInfo->index].fileName, "");
    parseInfo->parentDirPtr[parseInfo->index].location = -1;
    parseInfo->parentDirPtr[parseInfo->index].fileSize = 0;
    parseInfo->parentDirPtr[parseInfo->index].blocksSpanned = 0;

    return 0;
}

int fs_stat(const char *path, struct fs_stat *buf) {
    PPI *pathInfo = parsePath(path);

    if(pathInfo == NULL)
        return -1; // Does not exist or error finding the desired file or directory

    buf->st_size = pathInfo->parentDirPtr[pathInfo->index].fileSize;
    buf->st_blocks = pathInfo->parentDirPtr[pathInfo->index].blocksSpanned;
    buf->st_createtime = pathInfo->parentDirPtr[pathInfo->index].created;
    buf->st_modtime = pathInfo->parentDirPtr[pathInfo->index].lastModified;

    return 0;
}