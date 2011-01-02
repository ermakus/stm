#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "Torrent.h"

void* tobuf(const char* filename, size_t& fileLen ) {
	
        FILE *file;
	char *buffer;

	//Open file
	file = fopen(filename, "rb");
	if (!file)
	{
		fprintf(stderr, "Unable to open file %s", filename);
		exit(1);
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
 	

class DebugListener : public ITorrentListener {
public:
  void notify(const char* msg) {
     printf("DEBUG> %s\n", msg ); 
  }
  void error(const char* msg) {
     printf("ERROR> %s\n", msg ); 
  }
};

void* run_function( void *ptr )
{
   ((ITorrent*)ptr)->run();
   return NULL;
}

int main(int argc, char **argv)
{
  if( argc != 2 ) { 
	printf("Usage: %s file.torrent\n", argv[0]); 
        exit(1); 
  }

  DebugListener listener;
  ITorrent* torrent = ITorrent::create( &listener );

  size_t buf_size;
  void* buf = tobuf( argv[1], buf_size );

  torrent->init( buf, buf_size );
  
  pthread_t thread; 

  pthread_create (&thread, NULL, run_function, (void*) torrent);
  int ch = getchar();
  torrent->stop();
  ch = getchar();
  pthread_create (&thread, NULL, run_function, (void*) torrent);
  ch = getchar();
  torrent->stop();
  ITorrent::release( torrent );
  pthread_join(thread, NULL);
  exit(0);
}

