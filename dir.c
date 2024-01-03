#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

int main(int argc, char *argv[])
{
int n=0, i=0;
DIR *d;
struct dirent *dir;
d = opendir(argv[1]);

//Determine the number of files
while((dir = readdir(d)) != NULL) {
    if ( !strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") )
    {

    } else {
        n++;
    }
}
rewinddir(d);

char *filesList[n];

//Put file names into the array
while((dir = readdir(d)) != NULL) {
    if ( !strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") )
    {}
    else {
        filesList[i] = (char*) malloc (strlen(dir->d_name)+1);
        strncpy (filesList[i],dir->d_name, strlen(dir->d_name) );
        i++;
    }
}
rewinddir(d);

for(i=0; i<=n; i++)
    printf("%s\n", filesList[i]);
closedir(d);
return 0;
}
