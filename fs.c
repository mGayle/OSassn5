#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>


#define NUM_BLOCKS 4226
#define BLOCK_SIZE 8192
#define NUM_FILES 128
#define NUM_INODES 128
#define MAX_BLOCKS_PER_FILE 32

unsigned char data_blocks[NUM_BLOCKS][BLOCK_SIZE];
int used_blocks[NUM_BLOCKS];

struct directory_entry {
	char *name; 
	int valid; 
	int inode_idx; 
}; 

struct directory_entry *directory_ptr; 
	
struct inode {
	time_t date;    
	int valid; 
	int blocks[MAX_BLOCKS_PER_FILE]; 
	int size; 
}; 

struct inode *inode_array_ptr[NUM_INODES];


//list number of bytes free
//we'll use it when user types put for making sure
//we have enough space in file system


int df()
{
	int count = 0; 
	int i = 0; 
	
	for( i = 130; i < 4226; i++ )
	{
		if( used_blocks[i] )
		{
			count ++; 
		}
	}
	return count * BLOCK_SIZE; 
}



//make this better to where the inodes are sure to be consecutive
//int findFreeInodeBlockEntry( int inode_index )
int findFreeInodeBlockEntry(void)
{
	int inode_index = 0; 
 	int i; 
	int retval = -1; 
	for( i = 0; i < 32; i ++ )
	{
		if( inode_array_ptr[inode_index]->blocks[i] == -1 )
		{
			retval = i; 
			break; 
		}
	}
	return retval; 
}



void init ()
{
	int i; 
	directory_ptr = (struct directory_entry*) &data_blocks[0]; 
//set all directories as available
	for( i = 0; i < NUM_FILES; i++ )
	{
		directory_ptr[i].valid = 0; 
	}
//allocate and initialize all inodes. 
	int inode_idx = 0; 
	for( i = 1; i < 130; i++ ){
		inode_array_ptr[inode_idx++] = (struct inode*) &data_blocks[i]; 
	}
}




//returns us a valid dir entry
int findFreeDirectoryEntry()
{
	int i; 
	int retval = -1; 
	for( i = 0; i < 128; i++ ){
		if( directory_ptr[i].valid == 0) {
			retval = i; 
			break; 
		}
	}
	return retval; 
}

//goes from inode array and finds one that's currently not used (no valid entry in it.
int findFreeInode()
{
	int i; 
	int retval = -1; 
	for( i = 0; i < 128; i++ )
	{
		if( inode_array_ptr[i]->valid == 0 )
		{
			retval = i; 
			break; 
		}
	}
	return retval; 
}


int findFreeBlock()
{
	int retval = -1; 
	int i = 0; 

	for( i = 130; i < 4226; i++ )
	{
		if( used_blocks[i] == 0 )
		{
			retval = i; 
			break; 
		}
	}
	return retval; 
}

void put ( char *filename )
	{
	struct stat buf; 
	//use this to get size of the file and whether file really exists
	//if doesn't exist, stat returns -1
	int status = stat( filename, &buf ); 

	if( status == -1 ){
		printf( "Error: File not found \n"); 
		return; 
	}


	if( buf.st_size > df() )
	{
		printf( "Error: Not enough room in file system\n"); 
		return; 
	}

	int dir_idx = findFreeDirectoryEntry(); 
	if( dir_idx == -1 )
	{
		printf( "Error: Not enough room in file system \n"); 
		return; 
	}
	//mark as used so don't overwrite
	directory_ptr[dir_idx].valid = 1;
	directory_ptr[dir_idx].name = (char*)mallox( stsrlen( filename ) ); 
	//safe one to use bc can specify size so no buff overflow
	strncpy( directory_ptr[dir_idx].name, filename, strlen( filename ) );    	
	
	int inode_index = findFreeInode(); //inode for our file
	if( inode_index == -1 )
	{
		printf("Error: No free inodes\n"); 
		return; 	
	}
//beginning to piece together this trail of the file. WE've got our directory entry which has
//our filename and says we're using this entry with a valid file and it points to an inode index
//and with this we can start storing off time and filesize and start dealing with datablocks. 
	directory_ptr[dir_idx].inode_idx = inode_index; 

	inode_array_ptr[inode_index]->size = buf.st_size; 
	inode_array_ptr[inode_index]->date = time(NULL); 
	inode_array_ptr[inode_index]->valid = 1; 


//size of file/ block size = # times we'll go through loop to copy the file
//take file ptr, move by offset and read 8K out of there (1 block size out of file) into our
//used block array. first find free block. read into that block. update offset. decrement 
//number of bytes left to copy until have read all data. 

   	 // Open the input file read-only 
    	FILE *ifp = fopen ( filename, "r" ); 

	//how big the file is so we know how many bytes to copy
	int copy_size = buf.st_size; 
	int offset = 0; 
	
	
    	int block_index =  findFreeBlock();
	if( block_index == -1 )
	{
		printf("Error: Can't find free block\n"); 
		//cleanup a bunch of dirrectory and inode stuff
		//shouldn't have an error here really. There's a bug somewhere.
		return; 
	}

    // copy_size is initialized to the size of the input file so each loop iteration we
    // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
    // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
    // we have copied all the data from the input file.
    	while( copy_size >= BLOCK_SIZE )
    	{
		int block_index = findFreeBlock(); 
		if( block_index == -1 )
		{
			printf("Error: Can't find free block\n"); 
			return; 	
		}	
		used_blocks[ block_index ] = 1; 
		int inode_block_entry = findfreeInodeBlockEntry(); 
		if( inode_block_entry == -1 )
		{
			printf("Error: Cant find free node block\n"); 
			//Cleanup a bunch of directory and inode stuff
			return; 
		}
		inode_array_ptr[inode_index]->blocks[findFreeInodeBlockEntry] = block_index; 


      // Index into the input file by offset number of bytes.  Initially offset is set to
      // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We 
      // then increase the offset by BLOCK_SIZE and continue the process.  This will
      // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
		fseek( ifp, offset, SEEK_SET );
 
      // Read BLOCK_SIZE number of bytes from the input file and store them in our
      // data array. 
		int bytes  = fread( data_blocks[block_index], BLOCK_SIZE, 1, ifp );
		
      // If bytes == 0 and we haven't reached the end of the file then something is 
      // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
      // It means we've reached the end of our input file.
		if( bytes == 0 && !feof( ifp ) )
		{
			printf("An error occured reading from the input file.\n");
			return;
		};
		clearerr( ifp ); 
			
		copy_size -= BLOCK_SIZE; 
		offset	  += BLOCK_SIZE; 	



		}
		if( copy_size > 0 )
		{	

			//handle remainder	
			int block_index = findFreeBlock(); 
			if( block_index == -1 )
			{
				printf("Error: Can't find free block\n"); 
				return; 	
			}	

			int inode_block_entry = findFreeInodeBlockEntry(); 
			if( inode_block_entry == -1 )
			{
				printf("Error: Can't find free node block\n"); 
				//Cleanup a bunch of directory and inode stuff
				return; 
			}
			inode_array_ptr[inode_index]->blocks[inode_block_entry] = block_index; 
			
			used_blocks[block_index] = 1; 
			fseek( ifp, offset, SEEK_SET );
			int bytes  = fread( data_blocks[block_index], copy_size, 1, ifp );
		}
	fclose( ifp ); 	
	return; 
}


int main ()
{
	init(); 

	char *filename1 = "firstfile.txt"; 
	char *filename2 = "second_file.txt"; 

	//Lets find a spot for the first file
	int dir_idx = findFreeDirectoryEntry(); 
	printf("Found a free directory entry at %d for file %s\n", dir_idx, filename1 ); 
	
	//Now set this directory entry as used so we don't use it twice
	directory_ptr[dir_idx].valid = 1; 

	//Allocate room for the filename
	directory_ptr[dir_idx].name = (char*)malloc( stsrlen( filename1 ) ); 

	//Copy the filename
	memcpy( directory_ptr[dir_idx].name, filename1, strlen( filename1 ) );

	//Lets find a spot for the second filee
	dir_idx = findFreeDirectoryEntry(); 

	//Now set this directory entry as used so we don't use it twice
	directory_ptr[dir_idx].valid = 1; 

	//Allocate room for the filename
	directory_ptr[dir_idx].name = (char*)malloc( strlen( filename2 )); 

	//copy the filename
	memcpy( directory_ptr[dir_idx].name, filename2, strlen( filename2 ) ); 

	printf("Found a free directory entry at %d for file %s\n", dir_idx, filename2 ); 

	printf("Disk free: %d bytes\n", df() ); 

	//Now iterate over the firectory structure to make sure it's good. 
	//This is similar to the list() function you need to write
	int idx = 0; 
	for( idx = 0; idx < 128; idx++ )
	{
		if( directory_ptr[idx].valid == 1 )
		{
			printf("File: %s\n", directory_ptr[idx].name ); 
		}
	}
	return 0; 
} 


/*

HOW TO DELETE:  

easy way is to go through dir entries and set valid flag as 0. 
but need to go to inode index from dir entry and set that to 0 so we can reallocate.
set inode valid = 0
every block in block array, set as 0 in the used array. 
in inode struct, for blocks in blocks array, reset to -1, unless you want to do the undelete. 

*/ 
