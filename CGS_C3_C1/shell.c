// CSG_C3_C1
#include <stdio.h>
#include <string.h>
#include "filesys.h"

void taskD ()
{
    format();
    writedisk("virtualdiskD3_D1");

}

void taskC()
{
    format();

    MyFILE * fileWrite;
    fileWrite = myfopen("testfile.txt", "w");

    // write the alphabet
    char * alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int j = 0;
	for(int i = 0; i < (4 * BLOCKSIZE); i++)
    {
		if( j == 26 )
        {
            j = 0;
        }
		myfputc(alphabet[j], fileWrite);
		j++;	
	}

	//insert EOF to end of the file and close it
	myfputc(ENDOFFILE, fileWrite);
    myfclose(fileWrite);

    // open file for reading
    MyFILE *fileRead = myfopen("testfile.txt", "r");
    // create a real file
    FILE *realFile = fopen("testfileC3_C1_copy.txt", "w");

    // read the previously inputted alphabet
	for (int i = 0; i < (4 * BLOCKSIZE); i++)
	{
		char character = myfgetc(fileRead);
		// stop reading if end of file
        if (character == ENDOFFILE)
			break;
		printf("%c", character);
        // save every char to the real file
        fputc(character, realFile);
	}

    // close file after reading
	myfclose(fileRead);
    // close real file after writing into it
    fclose(realFile);

    writedisk("virtualdiskC3_C1");
}

int main () {

    // taskD();
    taskC();
    return 0;
}
