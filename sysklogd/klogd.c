/* vi: set sw=4 ts=4: */
/*
 * Mini klogd implementation for busybox
 *
 * Copyright (C) 2001 by Gennady Feldman <gfeldman@gena01.com>.
 * Changes: Made this a standalone busybox module which uses standalone
 *					syslog() client interface.
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 *
 * Copyright (C) 2000 by Karl M. Hegbloom <karlheg@debian.org>
 *
 * "circular buffer" Copyright (C) 2000 by Gennady Feldman <gfeldman@gena01.com>
 *
 * Maintainer: Gennady Feldman <gfeldman@gena01.com> as of Mar 12, 2001
 *
 * Licensed under the GPL v2 or later, see the file LICENSE in this tarball.
 */

#include "libbb.h"
#include <syslog.h>


/* The klogctl(3) interface allows us to read past and future messages alike.
 * Alternatively, we read the messages from _PATH_KLOG. */

#if ENABLE_FEATURE_KLOGD_KLOGCTL
# include <sys/klog.h>

static inline void klogd_open(void)
{
	/* "Open the log. Currently a NOP" */
	klogctl(1, NULL, 0);
}

static inline void klogd_close(void)
{
	/* FYI: cmd 7 is equivalent to setting console_loglevel to 7
	 * via klogctl(8, NULL, 7). */
	klogctl(7, NULL, 0); /* "7 -- Enable printk's to console" */
	klogctl(0, NULL, 0); /* "0 -- Close the log. Currently a NOP" */
}

static inline int klogd_read(char *bufp, int len)
{
	return klogctl(2, bufp, len);
}

static inline void klogd_setloglevel(int lvl)
{
	/* "printk() prints a message on the console only if it has a loglevel
	 * less than console_loglevel". Here we set console_loglevel = lvl. */
	klogctl(8, NULL, lvl);
}

#else
# include <paths.h>
# ifndef _PATH_KLOG
#  ifdef __GNU__
#   define _PATH_KLOG "/dev/klog"
#  else
#   error "your system's _PATH_KLOG is unknown"
#  endif
# endif

/* FIXME: consumes global static memory */
static int klogfd = 0;

static inline void klogd_open(void)
{
	klogfd = open(_PATH_KLOG, O_RDONLY, 0);
	if (klogfd < 0) {
		syslog(LOG_ERR, "klogd: can't open "_PATH_KLOG" (error %d: %m)",
				errno);
		exit(EXIT_FAILURE);
	}
}

static inline void klogd_close(void)
{
	if (klogfd != 0)
		close(klogfd);
}

static inline int klogd_read(char *bufp, int len)
{
	return read(klogfd, bufp, len);
}

static inline void klogd_setloglevel(int lvl UNUSED_PARAM)
{
	syslog(LOG_WARNING, "klogd warning: this build does not support"
			" changing the console log level");
}

#endif


static void klogd_signal(int sig)
{
	klogd_close();
	syslog(LOG_NOTICE, "klogd: exiting");
	kill_myself_with_sig(sig);
}

#define log_buffer bb_common_bufsiz1
enum {
	KLOGD_LOGBUF_SIZE = sizeof(log_buffer),
	OPT_LEVEL      = (1 << 0),
	OPT_FOREGROUND = (1 << 1),
};

int klogd_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int klogd_main(int argc UNUSED_PARAM, char **argv)
{
	int i = 0;
	char *opt_c;
	int opt;
	int used = 0;

	opt = getopt32(argv, "c:n", &opt_c);
	if (opt & OPT_LEVEL) {
		/* Valid levels are between 1 and 8 */
		i = xatou_range(opt_c, 1, 8);
	}
	if (!(opt & OPT_FOREGROUND)) {
		bb_daemonize_or_rexec(DAEMON_CHDIR_ROOT, argv);
	}

	openlog("kernel", 0, LOG_KERN);

	bb_signals(BB_FATAL_SIGS, klogd_signal);
	signal(SIGHUP, SIG_IGN);

	klogd_open();

	if (i)
		klogd_setloglevel(i);

	syslog(LOG_NOTICE, "klogd started: %s", bb_banner);

	while (1) {
		int n;
		int priority;
		char *start;

		/* "2 -- Read from the log." */
		start = log_buffer + used;
		n = klogd_read(start, KLOGD_LOGBUF_SIZE-1 - used);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			syslog(LOG_ERR, "klogd: error %d in klogctl(2): %m",
					errno);
			break;
		}
		start[n] = '\0';

		/* klogctl buffer parsing modelled after code in dmesg.c */
		/* Process each newline-terminated line in the buffer */
		start = log_buffer;
		while (1) {
			char *newline = strchrnul(start, '\n');

			if (*newline == '\0') {
				/* This line is incomplete... */
				if (start != log_buffer) {
					/* move it to the front of the buffer */
					overlapping_strcpy(log_buffer, start);
					used = newline - start;
					/* don't log it yet */
					break;
				}
				/* ...but if buffer is full, log it anyway */
				used = 0;
				newline = NULL;
			} else {
				*newline++ = '\0';
			}

			/* Extract the priority */
			priority = LOG_INFO;
			if (*start == '<') {
				start++;
				if (*start) {
					/* kernel never generates multi-digit prios */
					priority = (*start - '0');
					start++;
				}
				if (*start == '>')
					start++;
			}
			/* Log (only non-empty lines) */
			if (*start)
				syslog(priority, "%s", start);

			if (!newline)
				break;
			start = newline;
		}
	}

	return EXIT_FAILURE;
}
