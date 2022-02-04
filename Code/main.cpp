/* Copyright 2013, by the California Institute of Technology.  ALL RIGHTS
   RESERVED.  United States Government Sponsorship acknowledged.  Any 
   commercial use must be negotiated with the Office of Technology Transfer
   at the California Institute of Technology.

   This software may be subject to U.S. export control laws.  By accepting
   this software, the user agrees to comply with all applicable U.S. export
   laws and regulations.  User has the responsibility to obtain export 
   licenses, or other export authority as may be required before exporting
   such information to foreign countries or providing access to foreign
   persons.

   Please do not redistribute this file separate from the NGDCS source
   distribution.  For questions regarding this software, please contact the
   author, Alan Mazer, alan.s.mazer@jpl.nasa.gov */


#include <unistd.h>
#include <semaphore.h>
#if !defined(__APPLE__)
#include <sys/file.h>
#endif
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <gtkmm.h>
#include "main.h"
#include "framebuf.h"
#include "appFrame.h"

extern const char *const mainDate = "$Date: 2015/04/24 16:48:57 $";


int main(int argc, char *argv[])
{
    int fd, headless;

	/* initialize thread support */

    if (!Glib::thread_supported())
	Glib::thread_init();
    else {
	(void)fprintf(stderr, "Internal error: No thread support.\n");
	exit(EXIT_FAILURE);
    }

	/* init differently depending on whether or not we're headless */

    if (argc == 1)
	headless = 0;
    else if (argc == 2 && strcmp(argv[1], "-h") == 0)
	headless = 1;
    else {
	(void)fprintf(stderr, "usage: ngdcs [-h]\n");
	exit(EXIT_FAILURE);
    }
    if (!headless)
	new Gtk::Main(argc, argv);
    else Glib::init();

	/* create a lock to prevent simultaneous runs */

    fd = creat(LOCK_FILE, 0777);
    if (fd == -1) {
	(void)fprintf(stderr, "Can't create lock file \"%s\".\n", LOCK_FILE);
	exit(EXIT_FAILURE);
    }
    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
	(void)fprintf(stderr, "NGDCS software seems to already be running.\n");
	exit(EXIT_FAILURE);
    }

	/* bump up our priority -- this helps us cope with other system
           activity without dropping data but only takes effect when run as
           root */

    (void)setpriority(PRIO_PROCESS, 0, -20);

        /* create the main threads depending on whether or not we're headless */

    if (headless) {
	appFrameHeadless frame;
	if (frame.failed) {
	    if (flock(fd, LOCK_UN) == -1) {
		(void)fprintf(stderr, "Internal error: Unlock failed.\n");
		exit(1);
	    }
	    (void)close(fd);
	    exit(EXIT_FAILURE);
	}
    }
    else /* we've got a GUI */ {
	appFrameWin frame;
	if (frame.failed) {
	    if (flock(fd, LOCK_UN) == -1) {
		(void)fprintf(stderr, "Internal error: Unlock failed.\n");
		exit(1);
	    }
	    (void)close(fd);
	    exit(EXIT_FAILURE);
	}
	Gtk::Main::run(frame);
    }

        /* the user has closed the app -- we're done! */

    if (flock(fd, LOCK_UN) == -1) {
	(void)fprintf(stderr, "Internal error: Unlock failed.\n");
	exit(EXIT_FAILURE);
    }
    (void)close(fd);
    (void)unlink(LOCK_FILE);
    return EXIT_SUCCESS;
}
