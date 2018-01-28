#include <string.h>
#include <stdio.h>


#ifdef _WIN32
#include <windows.h>
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/statvfs.h>
#endif

// Following 2 routines taken from: http://stackoverflow.com/questions/1557400/hex-to-char-array-in-c
int xdigit( char digit ){
  int val;
       if( '0' <= digit && digit <= '9' ) val = digit -'0';
  else if( 'a' <= digit && digit <= 'f' ) val = digit -'a'+10;
  else if( 'A' <= digit && digit <= 'F' ) val = digit -'A'+10;
  else                                    val = -1;
  return val;
}

int xstr2strr(char *buf, unsigned bufsize, const char *in) {
  if( !in ) return -1; // missing input string

  unsigned inlen=strlen(in);
  if( inlen%2 != 0 ) inlen--; // hex string must even sized

  unsigned int i,j;
  for(i=0; i<inlen; i++ )
    if( xdigit(in[i])<0 ) return -3; // bad character in hex string

  if( !buf || bufsize<inlen/2+1 ) return -4; // no buffer or too small

  for(i=0,j=0; i<inlen; i+=2,j++ )
    buf[j] = xdigit(in[i])*16 + xdigit(in[i+1]);

  buf[inlen/2] = '\0';
  return inlen/2+1;
}
// END (taken from)

// Detect number of CPUs (by Dirk-Jan Kroon)
 
int getNumberOfCores() {
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#elif MACOS
    int nm[2];
    size_t len = 4;
    uint32_t count;
 
    nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);
 
    if(count < 1) {
    nm[1] = HW_NCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);
    if(count < 1) { count = 1; }
    }
    return count;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

// From http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
int hostname_to_ip(char * hostname , char* ip) {
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
    
    if ( (he = gethostbyname( hostname ) ) == NULL) {
        herror("gethostbyname");
        return 1;
    }
    
    addr_list = (struct in_addr **) he->h_addr_list;
    
    for(i = 0; addr_list[i] != NULL; i++) {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
    
    return 1;
}

unsigned long long freespace(char *path) {
	struct statvfs fData;

	if(statvfs(path,&fData) != 0) {
		printf("Failed to get free disk space on %s\n", path);
		return 0;
	}
	return (unsigned long long)fData.f_bsize * fData.f_bavail;
}

// Free memory. Taken from http://stackoverflow.com/questions/2513505/how-to-get-available-memory-c-g
unsigned long long freemem() {
#ifdef _WIN32
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status.ullTotalPhys;
#else
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	return (unsigned long long)pages * page_size;
#endif
}

