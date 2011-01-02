#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <dirent.h> 

#define PLUGINS_DIR "/System/Library/Internet Plug-Ins"

static void* readfile(const char* filename, size_t & fileLen ) {

        FILE *file;
        char *buffer;

        //Open file
        file = fopen(filename, "rb");
        if (!file)
        {
		return NULL;
        }

        //Get file length
        fseek(file, 0, SEEK_END);
        fileLen=ftell(file);
        fseek(file, 0, SEEK_SET);

        //Allocate memory
        buffer=(char *)malloc(fileLen+1);
        if (!buffer)
        {
                fprintf(stderr, "Memory error!");
                                fclose(file);
                exit(1);
        }

        //Read file contents into buffer
        fread(buffer, fileLen, 1, file);
        fclose(file);

        return buffer;
}

static void writefile(const char* filename, void* buffer, size_t fileLen) {
        FILE *file;
        //Open file
        file = fopen(filename, "wb");
        if (!file)
        {
                fprintf(stderr, "Unable to open file %s", filename);
                exit(1);
        }

        fwrite(buffer, fileLen, 1, file);
        fclose(file);
}

static bool patch(const char* from, const char* to, size_t len, char* mem, size_t mem_len ) {
    char* p = mem;
    bool found = false;
    for( int i = 0; i < (mem_len - len); i++ ) {
       if( memcmp( p, from, len ) == 0 ) {
           memcpy( p, to, len );
           found = true; 
       }
       p++;
    }
}

int main(int argc, char** argv) 
{
   struct dirent * dp; 
   DIR * dir = opendir (PLUGINS_DIR);
   size_t len;

    if( argc == 2 && strcmp( argv[1], "install" ) == 0 ) {
      fprintf( stderr, "Installing plugin\n" );
    } else {
      fprintf( stderr, "Uninstalling plugin\n" );
    }


   while ((dp=readdir(dir)) != NULL) { 
     char name[512], name2[512];
     if( dp->d_type == DT_DIR && dp->d_name[0] != '.' ) {
         if( strcmp( "Torrent.webplugin", dp->d_name ) == 0 ) continue;
         sprintf( name, "%s/%s/Info.plist", PLUGINS_DIR, dp->d_name ); 
         printf( "Checking: %s\n", name);
         void* mem = readfile( name, len );
         if( !mem ) continue;
         patch( "torrent", "xorrent", 7, (char*)mem, len ); 
         sprintf( name, "%s/%s/Info.plist.new", PLUGINS_DIR, dp->d_name );
         writefile( name, mem, len );
         free( mem );
     }
   }
   closedir (dir); 
   return 0;
}
