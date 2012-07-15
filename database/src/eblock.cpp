#include <fcntl.h>
#include <sys/types.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>

#include "Utility/database/ebase.h"

#include "ebase_impl.h"

EbLock::EbLock (int fd)
: usedByDB ( false )
{
	lock_fd = fd;
}

bool EbLock::doLock ()
{
	struct flock ret ;
	ret.l_type = F_WRLCK ;
	ret.l_start = 0 ;
	ret.l_whence = SEEK_SET ;
	ret.l_len = 0 ;
	ret.l_pid = getpid() ;
	int r = fcntl(lock_fd, F_SETLKW, &ret);
    if (r != -1)
    {
        printf( "lock success\n" );
        return true;
    }
    else
    {
        return false;
    }
    //    FILE *file = fopen (_lockfile, "r");
    //    if (file == NULL)
    //    {
    //      EB_ASSERT (errno == ENOENT);
    //      FILE *ofile = fopen (_lockfile, "w");
    //      fprintf (ofile, "%p", this);
    //      fclose( ofile );
    //      printf( "lock success\n" );
    //      return true;
    //    }
    //    else
    //    {
    //      fclose (file);
    //      printf ( "lock failed because the lock already exists\n" );
    //      return false;
    //    }
    
}

bool EbLock::isLockingFile ()
{
	struct flock ret ;
	int r = fcntl(lock_fd, F_GETLK, &ret);

	if(ret.l_type == F_UNLCK){
		return true;
	}
	else 
		return false;
}

bool EbLock::releaseLock ()
{
    if (usedByDB) 
    {
        printf ("Cannot releaseLock () while (usedByDB == true)\n");
        return false;
    }
    struct flock ret ;
	ret.l_type = F_UNLCK ;
	ret.l_start = 0 ;
	ret.l_whence = SEEK_SET ;
	ret.l_len = 0 ;
	ret.l_pid = getpid() ;
	int r = fcntl(lock_fd, F_SETLKW, &ret);
    if (r != -1)
    {
        printf( "release success\n" );
		close(lock_fd);
        return true;
    }
    else
    {
        return false;
    }
}

#ifndef WIN32
EbSemLock::EbSemLock (sem_t *p) 
{ 
    _sem = p;  locked = false;
}

int
EbSemLock::TryWait ()
{ 
    int retval = sem_trywait (_sem); 
    if (retval == 0)
    {
        locked = true;
    }
    else
    {
        locked = false;
    }
    return retval;
}

EbSemLock::~EbSemLock ()
{ 
    if (locked)
        sem_post (_sem); 
}

#endif

