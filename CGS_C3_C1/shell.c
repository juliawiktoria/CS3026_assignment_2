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
    // create "testfile.txt"
    MyFILE * file;
    // MyFILE * file_2;

    file = myfopen("testfile.txt", "w");

    char * alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	int x = 0;
	int y = 0; 
	//insert each character of alphabet to the file 4096 times(4KB file)
	for(x=0; x < (4*BLOCKSIZE); x++)
    {
		if( y == 26 )
        {
            y = 0;
        }
		myfputc(alphabet[y], file);
		y++;	
	}
	//insert EOF to end of the file
	myfputc(EOF, file);

    // file_2 = myfopen("testfile.txt", "w");
    // write 4kB of text into testfile.txt
    // char * alph = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    // myfputc(EOF, file);
    // close the file
    // myfclose(file);
    writedisk("virtualdiskC3_C1");
    // read the testfile.txt
    // myfgetc();

    // printBlock();
}

int main () {

    // taskD();
    taskC();
    return 0;
}
