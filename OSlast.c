#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

#define NUM_BLOCKS 4226
#define BLOCK_SIZE 8192
#define NUm_FILES 128
#define NUM_INODES 128
#define MAX_BLOCKS_PER_FILE 32

#define WHITESPACE " \t\n"   
#define MAX_COMMAND_SIZE 255    // The maximum command-line size
#define MAX_NUM_ARGUMENTS 10 

unsigned char data_blocks[NUM_BLOCKS][BLOCK_SIZE];
int used_blocks[NUM_BLOCKS];

struct directory_entry {
  char*name;
  int valid;
  int inode_idx;
};

struct directory_entry *directory_ptr;

struct inode {
  time_t date;
  int valid;
  int size;
  int blocks[MAX_BLOCKS_PER_FILE];
};

struct inode *inode_array_ptr[NUM_INODES];

void init()
{
	int i; 
	directory_ptr = (struct directory_entry*) &data_blocks[0]; 
	//set all directories as available
	for( i = 0; i < NUM_FILES; i++ )
	{
		directory_ptr[i].valid = 0; 
	}
	//allocate and initialize all inodes.
	int inode_inx = 0; 
	for( i = 1; i < 130; i++ ){
		inode_array_ptr[inode_idx++] = (struct inode*) &data_blocks[i]; 
	}
}


int df()
{
  int count = 0;
  int i =0;
  for (i=130;i<4226;i++)
  {
    if (used_blocks[i]==0)
      count++;
  }
  return count*BLOCK_SIZE;
}

int findFreeDirectoryEntry()
{
  int i, retval =-1;
  for ( i =0;i<128;i++)
  {
    if(directory_ptr[i].valid == 0)
    {
      retval = i;
      break;
    }
  }
  return retval;
}

int findFreeInodeBlockEntry(int inode_index)
{
  int i, retval =-1;
  for ( i =0;i<32;i++)
  {
    if(inode_array_ptr[inode_index]->blocks[i] == -1)
    {
      retval = i;
      break;
    }
  }
  return retval;
}

int findFreeInode()
{
  int i, retval =-1;
  for ( i =0;i<128;i++)
  {
    if(inode_array_ptr[i]->valid == 0)
    {
      retval = i;
      break;
    }
  }
  return retval;
}

int findFreeBlock()
{
  int i=0, retval =-1;
  for ( i =130;i<4226;i++)
  {
    if(used_blocks[i]== 0)
    {
      retval = i;
      break;
    }
  }
  return retval;
}

void list()
{
  int i, retval =-1;
  for ( i =0;i<128;i++)
  {
    if(directory_ptr[i].valid == 1)
    {
      printf("%s	%d	%s\n",directory_ptr[i].name, inode_array_ptr[directory_ptr[i].inode_idx]->size, 
					inode_array_ptr[directory_ptr[i].inode_idx]->date);
      retval = 0;
    }
  }
  if (retval == -1)
  {
    printf("No files found\n");
  }  
}

void put(char* filename)
{
  struct stat buf;
  int status = stat(filename, &buf);
  if (status ==-1)
  {
    printf("Error: No file found\n");
    return;
  }

  if (buf.st_size > df())
  { 
     printf("Error: Not enough space\n");
     return;
  }

  int dir_idx = findFreeDirectoryEntry();
  if (dir_idx == -1)
  {
    printf("Error: Not enough space\n");
    return ;
  }

  directory_ptr[dir_idx].valid = 1;
  directory_ptr[dir_idx].name = (char*) malloc (strlen(filename));
  strncpy (directory_ptr[dir_idx].name,filename, strlen(filename));
  
  int inode_idx = findFreeInode();
  if (inode_idx ==-1)  
  {
  printf("No free inodes\n");
  return;
  }

  directory_ptr[dir_idx].inode_idx = inode_idx;
  inode_array_ptr[inode_idx]->size = buf.st_size;
  inode_array_ptr[inode_idx]->date = time(NULL);
  inode_array_ptr[inode_idx]->valid = 1;

  FILE *ifp = fopen ( filename, "r" );
  int copy_size = buf.st_size;
  int offset = 0;  

  
  while(copy_size>=BLOCK_SIZE)
  {
    int block_index = findFreeBlock();
  
    if(block_index == -1)
    {
      printf("Error: Cant find free block---check the code!!\n");
      return;
    } 
    used_blocks [block_index]=1;
    int inode_block_entry = findFreeInodeBlockEntry(inode_idx);
    if(inode_block_entry == -1)
    {
      printf("Error: Cant find free inode---check the code!!\n");
      return;
    }

    inode_array_ptr[inode_idx]->blocks[inode_block_entry] = block_index;
    fseek(ifp,offset,SEEK_SET); 
    int bytes  = fread(data_blocks[block_index], BLOCK_SIZE, 1, ifp );

      // If bytes == 0 and we haven't reached the end of the file then something is 
      // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
      // It means we've reached the end of our input file.
    if( bytes == 0 && !feof( ifp ) )
    {
      printf("An error occured reading from the input file.\n");
      return;
    }

    clearerr(ifp);

    copy_size -= BLOCK_SIZE;
    offset += BLOCK_SIZE;
  }
  
  if(copy_size>0)
  {
    int block_index = findFreeBlock();

    if(block_index == -1)
    {
      printf("Error: Cant find free block---check the code!!\n");
      return;
    } 
    
    int inode_block_entry = findFreeInodeBlockEntry(inode_idx);
    if(inode_block_entry == -1)
    {
      printf("Error: Cant find free inode---check the code!!\n");
      return;
    }

    inode_array_ptr[inode_idx]->blocks[inode_block_entry] = block_index;
    used_blocks [block_index]=1;
    fseek(ifp,offset,SEEK_SET); 
    fseek(ifp,offset,SEEK_SET); 
    int bytes  = fread(data_blocks[block_index], copy_size, 1, ifp );
  }
  fclose(ifp);
  return;
}

int main( int argc, char *argv[] )
{

  init(); 
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE ); 
  while( 1 )
  {
    // Print out the msh prompt
    printf ("mfs> ");
    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    //char *working_root = working_str;
    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }
    
    if (strcmp(token[0],"put")==0)
    {
      printf("I am here0");
      put(token[1]);
    }
    else if (strcmp(token[0],"df")==0)
    {
      printf("Remaing blocks are: %d\n",df());
    }
    else if (strcmp(token[0],"list")==0)
    {
      list();
    }
    }
    return 0; 
}




/*    *********************************************************************************
    //
    // The following chunk of code demonstrates similar functionality to your get command
    //

    // Now, open the output file that we are going to write the data to.
    FILE *ofp;
    ofp = fopen(argv[2], "w");

    if( ofp == NULL )
    {
      printf("Could not open output file: %s\n", argv[2] );
      perror("Opening output file returned");
      return -1;
    }

    // Initialize our offsets and pointers just we did above when reading from the file.
    block_index = 0;
    copy_size   = buf . st_size;
    offset      = 0;

    printf("Writing %d bytes to %s\n", (int) buf . st_size, argv[2] );

    // Using copy_size as a count to determine when we've copied enough bytes to the output file.
    // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
    // our stored data to the file fp, then we will increment the offset into the file we are writing to.
    // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
    // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
    // last iteration we'd end up with gibberish at the end of our file. 
    while( copy_size > 0 )
    { 

      int num_bytes;

      // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
      // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
      // end up with garbage at the end of the file.
      if( copy_size < BLOCK_SIZE )
      {
        num_bytes = copy_size;
      }
      else 
      {
        num_bytes = BLOCK_SIZE;
      }

      // Write num_bytes number of bytes from our data array into our output file.
      fwrite(data_blocks[block_index], num_bytes, 1, ofp ); 

      // Reduce the amount of bytes remaining to copy, increase the offset into the file
      // and increment the block_index to move us to the next data block.
      copy_size -= BLOCK_SIZE;
      offset    += BLOCK_SIZE;
      block_index ++;

      // Since we've copied from the point pointed to by our current file pointer, increment
      // offset number of bytes so we will be ready to copy to the next area of our output file.
      fseek( ofp, offset, SEEK_SET );
    }

    // Close the output file, we're done. 
    fclose( ofp );
  }
  else
  {
    printf("Unable to open file: %s\n", argv[1] );
    perror("Opening the input file returned: ");
    return -1;
  }

  return 0;
  }
}
*/






