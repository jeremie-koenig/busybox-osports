/* vi: set sw=4 ts=4: */
/*
 * System-specific definitions for mount.
 *
 * Copyright (C) 2010 by Jeremie Koenig <jk@jk.fr.eu.org>
 * Copyright (C) 2010 by Luca Favatella <slackydeb@gmail.com>
 *
 * The Linux prototypes for mount(), umount2(), swapon() and swapoff()  are
 * used as a reference for our versions of them. On non-Linux system those
 * should be implemented as compatibility wrappers (see xmount.c).
 */

/*
 * Definitions for mount flags. Non-Linux systems are free to use whatever
 * their version of xmount() will work with.
 */

#ifdef __linux__
# include <sys/mount.h>
# include <sys/swap.h>
/* Make sure we have all the new mount flags we actually try to use
 * (grab more as needed from util-linux's mount/mount_constants.h). */
# ifndef MS_DIRSYNC
#  define MS_DIRSYNC     (1 << 7) // Directory modifications are synchronous
# endif
# ifndef MS_UNION
#  define MS_UNION       (1 << 8)
# endif
# ifndef MS_BIND
#  define MS_BIND        (1 << 12)
# endif
# ifndef MS_MOVE
#  define MS_MOVE        (1 << 13)
# endif
# ifndef MS_RECURSIVE
#  define MS_RECURSIVE   (1 << 14)
# endif
# ifndef MS_SILENT
#  define MS_SILENT      (1 << 15)
# endif
/* The shared subtree stuff, which went in around 2.6.15. */
# ifndef MS_UNBINDABLE
#  define MS_UNBINDABLE  (1 << 17)
# endif
# ifndef MS_PRIVATE
#  define MS_PRIVATE     (1 << 18)
# endif
# ifndef MS_SLAVE
#  define MS_SLAVE       (1 << 19)
# endif
# ifndef MS_SHARED
#  define MS_SHARED      (1 << 20)
# endif
# ifndef MS_RELATIME
#  define MS_RELATIME    (1 << 21)
# endif

#elif defined(__FreeBSD_kernel__)
# include <sys/mount.h>
# include <sys/swap.h>
# define MS_NOSUID      MNT_NOSUID
# define MS_NODEV       0
# define MS_NOEXEC      MNT_NOEXEC
# define MS_SYNCHRONOUS MNT_SYNCHRONOUS
# define MS_DIRSYNC     0
# define MS_NOATIME     MNT_NOATIME
# define MS_NODIRATIME  0
# define MS_MANDLOCK    0
# define MS_RELATIME    0
# define MS_SILENT      0
# define MS_UNION       0
# define MS_BIND        0
# define MS_MOVE        0
# define MS_SHARED      0
# define MS_SLAVE       0
# define MS_PRIVATE     0
# define MS_UNBINDABLE  0
# define MS_RECURSIVE   0
# define MS_RDONLY      MNT_RDONLY
# define MS_REMOUNT     MNT_UPDATE

#else
# error There is no xmount() implementation for your system.
#endif

/*
 * Prototypes for the compatibility wrappers
 */

#ifdef __linux__
# define xmount mount
# define xumount umount2
# define xswapon swapon
# define xswapoff swapoff
#else
int xmount(const char *source, const char *target, const char *filesystemtype,
		unsigned long mountflags, const void *data) FAST_FUNC;
int xumount(const char *target, int flags) FAST_FUNC;
int xswapon(const char *path, int swapflags) FAST_FUNC;
int xswapoff(const char *path) FAST_FUNC;
#endif
