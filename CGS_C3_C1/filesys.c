/* filesys.c
 * 
 * provides interface to virtual disk
 * 
 */
// CSG_C3_C1
#include <stdio.h>
// #include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "filesys.h"


diskblock_t  virtualDisk [MAXBLOCKS] ;           // define our in-memory virtual, with MAXBLOCKS blocks
fatentry_t   FAT         [MAXBLOCKS] ;           // define a file allocation table with MAXBLOCKS 16-bit entries
fatentry_t   rootDirIndex            = 0 ;       // rootDir will be set by format
direntry_t * currentDir              = NULL ;
fatentry_t   currentDirIndex         = 0 ;

/* writedisk : writes virtual disk out to physical disk
 * 
 * in: file name of stored virtual disk
 */

void writedisk ( const char * filename )
{
   printf ( "writedisk> virtualdisk[0] = %s\n", virtualDisk[0].data ) ;
   FILE * dest = fopen( filename, "w" ) ;
   if ( fwrite ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
   fclose(dest) ;
   
}

void readdisk ( const char * filename )
{
   FILE * dest = fopen( filename, "r" ) ;
   if ( fread ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
      fclose(dest) ;
}

/* the basic interface to the virtual disk
 * this moves memory around
 */

void writeblock ( diskblock_t * block, int block_address )
{
   //printf ( "writeblock> block %d = %s\n", block_address, block->data ) ;
   memmove ( virtualDisk[block_address].data, block->data, BLOCKSIZE ) ;
   //printf ( "writeblock> virtualdisk[%d] = %s / %d\n", block_address, virtualDisk[block_address].data, (int)virtualDisk[block_address].data ) ;
}


/* read and write FAT
 * 
 * please note: a FAT entry is a short, this is a 16-bit word, or 2 bytes
 *              our blocksize for the virtual disk is 1024, therefore
 *              we can store 512 FAT entries in one block
 * 
 *              how many disk blocks do we need to store the complete FAT:
 *              - our virtual disk has MAXBLOCKS blocks, which is currently 1024
 *                each block is 1024 bytes long
 *              - our FAT has MAXBLOCKS entries, which is currently 1024
 *                each FAT entry is a fatentry_t, which is currently 2 bytes
 *              - we need (MAXBLOCKS /(BLOCKSIZE / sizeof(fatentry_t))) blocks to store the
 *                FAT
 *              - each block can hold (BLOCKSIZE / sizeof(fatentry_t)) fat entries
 */


// ----------------------------------------------------------------------------------------------------------------
// CSG_D3_D1
// ----------------------------------------------------------------------------------------------------------------

/* implement format()
 */
void format ( )
{
   // declare a new, empty block
   diskblock_t block = getEmptyBlock();
   direntry_t  rootDir ;
   int         pos             = 0 ;
   int         fatentry        = 0 ;
   int         fatblocksneeded =  ( MAXBLOCKS / FATENTRYCOUNT ) ;

   // use strcpy() to copy some text to it for test purposes
   strcpy(block.data, "Julia Z CS3026 Operating Systems Task");

   // write block 0 to virtual disk using provided writeblock() function
   writeblock(&block, 0);

	/* prepare FAT table
	 * write FAT blocks to virtual disk
	 */
   // prapare FAT table according to definition
   FAT[0] = ENDOFCHAIN;
   FAT[1] = 2;
   FAT[2] = ENDOFCHAIN;
   FAT[3] = ENDOFCHAIN;
   for (int i = 4; i < MAXBLOCKS; i++)
   {
      FAT[i] = UNUSED;
   }

   // write FAT table to the virtual disk using a helper function

   copyFAT();

   // create an empty root directory block
   diskblock_t root_block = getEmptyBlock();

   // set all entries in entry list to unused
   for (int i = 0; i < DIRENTRYCOUNT; i ++)
   {
      root_block.dir.entrylist[i].unused = TRUE;
   }

   // indicate that the block is a directory
   root_block.dir.isdir = TRUE;

   // first element in the entry list
   root_block.dir.nextEntry = FALSE;
   
   // save block to the disk
   rootDirIndex = 3;
   writeblock(&root_block, rootDirIndex);
}

void copyFAT()
{

   for (int i = 0; i < ( MAXBLOCKS / FATENTRYCOUNT ); i++) 
   {
      // declare new block
      diskblock_t block;

      for (int entry = 0; entry < FATENTRYCOUNT; entry++)
      {
         // fill the block
         block.fat[entry] = FAT[entry + (FATENTRYCOUNT * i)];
         // save to virtual disk
      }
      writeblock(&block, i + 1);
   }
}

// ----------------------------------------------------------------------------------------------------------------
// CSG_C3_C1
// ----------------------------------------------------------------------------------------------------------------

MyFILE * myfopen( const char * filename, const char * mode )
{
   diskblock_t dir_block = virtualDisk[3]; // get the directory block

   // check if mode correct = only 'w' or 'r'
   if ( *mode != 'w' && *mode != 'r' )
   {
      printf("Incorrect file mode!\n");
      return FALSE;
   }

   // create new file descriptor
   MyFILE * newFile = malloc(sizeof(MyFILE));
   
   // assign mode to the file
   strcpy(newFile->mode, mode);
   printf("%s\n", newFile->mode);

   // find out if file already exists (if so, return its location)
   int fileLocation = lookForFile(filename, dir_block);

   if (fileLocation == -1) // FILE DOES NOT EXIST
   {
      if ( *mode == 'r' )
      {
         printf("Error! Trying to read a file that does not exist!\n");
         return FALSE;
      }
      else
      {
         // create a completely new file for writing
         printf("Open file %s for writing\n", filename);
         newFile->pos = 0; // start from the beggining

         // find an empty directory for the file
         int freeDir = emptyDirIndex(dir_block);
         if (freeDir == -1)
         {
            printf("No more free space on the disk!\n");
            return FALSE;
         }

         // find an unused FAT entry for the chain
         int freeFATindex = emptyFATindex();
         if (freeFATindex == -1)
         {
            printf("Error! No free entries in the FAT\n");
            return FALSE;
         }
         
         // create a FAT chain
         FAT[freeFATindex] = ENDOFCHAIN;

         // allocate the newly found block to the newFile
         newFile->blockno = freeFATindex;

         // set the position of the file in the directory
         dir_block.dir.entrylist[freeDir].firstblock = freeFATindex;

         // write FAT to the disk
         copyFAT();

         // copy the filename to the directory
         strcpy(dir_block.dir.entrylist[freeDir].name, filename);
         // mark this entry in the directory as USED
         dir_block.dir.entrylist[freeDir].unused = FALSE;

         // save the block in the directory (index 3)
         writeblock(&dir_block, 3);
      }
   }
   else // file exists
   {
      newFile->pos = 0; // start reading from the beginning
      newFile->blockno = dir_block.dir.entrylist[fileLocation].firstblock;
   }

   return newFile;
}

void myfclose( MyFILE * stream )
{
   // check if file is in writing mode and save unsaved data
   if ( stream->mode == "w" )
   {
      // get the next empty FAT entry
		int freeFATindex = emptyFATindex();

		// create a FAT chain
		FAT[stream->blockno] = freeFATindex;
		FAT[freeFATindex] = ENDOFCHAIN;
		
      // write FAT to the disk
		copyFAT();

		// write the buffer to the disk (save whatever is in there)
      writeblock(&stream->buffer, stream->blockno);
   }

   // free the mallocated memory for file descriptor
   free(stream);
}

void myfputc(int b, MyFILE * stream )
{
   // return if file is open only for reading 
	if (strcmp(stream->mode, "r") == FALSE)
   {
      printf("The file is in read-open mode!\n");
      return;
   }

	// if the pointer points "outside the buffer"
	if (stream->pos >= BLOCKSIZE)
	{
		// get the index of the next free FAT
		int freeFATindex = emptyFATindex();

		// extend the FAT chain
		FAT[stream->blockno] = freeFATindex;
		FAT[freeFATindex] = ENDOFCHAIN;
		
      // save FAT to the disk
		copyFAT();
		
		// restart the character pointer
		stream->pos = 0;

		// write the buffer to the disk
      writeblock(&stream->buffer, stream->blockno);
		
      // reset the buffer so it's empty
		stream->buffer = getEmptyBlock();
		// update the block number
		stream->blockno = freeFATindex;
	}

	// save the char in the file buffer
	stream->buffer.data[stream->pos] = b;
   // increment the current position
	stream->pos++;	
}

int myfgetc(MyFILE * stream )
{
   int result;
	
	if(stream->blockno == ENDOFCHAIN)
   {
      return ENDOFFILE;
   }
	
   // if at the end of a block
	if (stream->pos % BLOCKSIZE == 0)
	{
		// set the number of the block
		stream->blockno = FAT[stream->blockno];

      // get new block from the disk
      stream->buffer = virtualDisk[stream->blockno];
 
		// reset the position pointer
		stream->pos = 0;	
	}

	// return character
   result = stream->buffer.data[stream->pos];
   stream->pos++;
	return result;
}

// ----------------------------------------------------------------------------------------------------------------
// ADDITIONAL UTILITY AND HELPER FUNCTIONS
// ----------------------------------------------------------------------------------------------------------------

// create and return an empty block (filled with '\0's)
diskblock_t getEmptyBlock()
{
   diskblock_t block;
   for (int i = 0; i < BLOCKSIZE; i++)
   {
      block.data[i] = '\0';
   }
   return block;
}

// find the next unusused FAT entry and return its index
int emptyFATindex ()
{
   for (int j = 0; j < MAXBLOCKS; j++)
   {
      if (FAT[j] == UNUSED) return j;
   }

   return -1; // return -1 if not found
}

// find the next unused directory and return its index
int emptyDirIndex (diskblock_t block) 
{
   for (int i = 0; i < DIRENTRYCOUNT; i++)
   {
      if (block.dir.entrylist[i].unused) return i;
   }

   return -1;
}

// check if a file already exists in the directory; return its index
int lookForFile(const char * filename, diskblock_t block)
{

   for (int i = 0; i < DIRENTRYCOUNT; i++)
   {
      // check block name if in use
      if (block.dir.entrylist[i].unused == FALSE)
      {
         if (strcmp(block.dir.entrylist[i].name, filename) == 0)
         {
            printf("File has been foundon the %d-th position in the directory\n", i);
            return i; // file found on the i-th position in the directory, return i
         }
      }
   }
   printf("File does not exist\n");
   return -1; // file was not found, return -1
}

/* use this for testing
 */
void printBlock ( int blockIndex )
{
   printf ( "virtualdisk[%d] = %s\n", blockIndex, virtualDisk[blockIndex].data ) ;
}

