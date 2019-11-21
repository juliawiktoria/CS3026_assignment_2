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
   diskblock_t block = virtualDisk[3]; // get the directory block

   // check if mode correct
   if ( *mode != 'w' && *mode != 'r' )
   {
      printf("Incorrect file mode!\n");
      return FALSE;
   } // DONE

   // create new file descriptor
   MyFILE * newFile = malloc(sizeof(MyFILE));
   // add mode to the file
   strcpy(newFile->mode, mode);
   printf("%s\n", newFile->mode); // WORKS UP TO HERE

   int fileLocation = lookForFile(filename, block);
   printf("file location: %d\n", fileLocation);


   if (fileLocation == -1) // FILE DOES NOT EXIST
   {
      if ( *mode == 'r' )
      {
         printf("Error! Trying to read a file that does not exist!\n");
         return FALSE;
      }
      else if (*mode == 'w')
      {
         printf("Open file %s for writing\n", filename);
         newFile->pos = 0; // start from the beggining

         // find an empty directory for the file
         int freeDir = emptyDirIndex(block);
         if (freeDir == -1)
         {
            printf("No more free space on the disk!\n");
            return FALSE;
         }

         // find an unused FAT entry for the chain
         int freeFAT = emptyFATindex();
         if (freeFAT == -1)
         {
            printf("Error! No free entries in the File Allocation Table\n");
            return FALSE;
         }
         
         FAT[freeFAT] = ENDOFCHAIN;
         // createFATchain(fileLocation);

         // allocate the newly found block to the newFile
         newFile->blockno = freeFAT;

         // set the position in the directory
         block.dir.entrylist[freeDir].firstblock = freeFAT;

         copyFAT();

         strcpy(block.dir.entrylist[freeDir].name, filename);
         block.dir.entrylist[freeDir].unused = FALSE;

         writeblock(&block, 3);

      }
   }
   else // file exists
   {
      newFile->pos = 0; // start reading from the beginning
      newFile->blockno = block.dir.entrylist[fileLocation].firstblock;

      // if ( *mode == 'r' )
      // {
      //    // open file for reading
      //    printf("open existing file for reading\n");
      //    newFile->pos = 0; // start reading from the beginning
      //    newFile->blockno = block.dir.entrylist[fileLocation].firstblock;

      // }
      // else if ( *mode == 'w')
      // {
      //    // open file for appending
      //    printf("open existing file for writing\n");
      //    newFile->pos = 0;
      //    newFile->blockno = block.dir.entrylist[fileLocation].firstblock;
      // }
      // else
      // {
      //    printf("Incorrect mode!!!!!\n");
      //    return FALSE;
      // }
   }
   return newFile;
}

void myfclose( MyFILE * stream )
{
   // check if file is in writing mode and save unsaved data
   if ( stream->mode == "w" )
   {
      // get the next unused block
		int next = getUnusedBlock();
		// set fat table block as used
		FAT[stream->blockno] = next;
		// move eoc to the next fat block
		FAT[next] = ENDOFCHAIN;
		// save fat table
		copyFAT();
		// save the incomplete buffer (this is because we wouldnt always have 1024 byte filled buffer blocks
      writeblock(&stream->buffer, stream->blockno);
   }
}

void myfputc(int b, MyFILE * stream )
{
   // return if file is open only for reading 
	if (strcmp(stream->mode, "r") == FALSE)
   {
      printf("The file is in read-open mode!\n");
      return;
   }

	// if the position of the "text pointer" is equal or bigger than blocksize (1024 in this case), stops at 1024
	if (stream->pos >= BLOCKSIZE)
	{
		// get the block number of the next unused block
		int freeFATindex = emptyFATindex();
		// change the fat table so that the block is checked as used
		FAT[stream->blockno] = freeFATindex;
		// set next block as eoc
		FAT[freeFATindex] = ENDOFCHAIN;
		
      // save fat table
		copyFAT();
		
		// reset the string stream pos/"text pointer" (kind of) 
		stream->pos = 0;

		// save the buffer to the virtual disk (since the buffer is 1024bytes)
      writeblock(&stream->buffer, stream->blockno);
		
      // empty the buffer
		stream->buffer = getEmptyBlock();
		// assign the new unused block number to the buffer
		stream->blockno = freeFATindex;
	}

	// put the character in the buffer and increment the position of the text pointer
	stream->buffer.data[stream->pos] = b;
	stream->pos++;	
}

int myfgetc(MyFILE * stream )
{
   int result;
   int charCounter = 0;

   // return if file is in write-only mode
   if ( stream->mode == "w" )
   {
      return;
   }

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

   return -1; //error
}

// find the next unused directory and return its index
int emptyDirIndex (diskblock_t block) 
{
   for (int i = 0; i < DIRENTRYCOUNT; i++)
   {
      if (block.dir.entrylist[i].unused) return i;
   }

   return -1; // no free space on disk 
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

