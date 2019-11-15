// CSG_C3_C1
#include <stdio.h>
#include <string.h>
#include "filesys.h"

int main()
{
    format();
    // create "testfile.txt"
    MyFILE * file;
    file = myfopen("testfile.txt", "w");
    // write 4kB of text into testfile.txt
    // myfputc();
    // close the file
    // myfclose();
    writedisk("virtualdiskC3_C1");
    // read the testfile.txt
    // myfgetc();

    // printBlock();
    return 0;
}
