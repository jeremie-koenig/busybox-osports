#include "libbb.h"
#include "xmount.h"

#ifdef __linux__

/* xmount and xumount short-circuited to mount and umount2 in xmount.h */

#elif defined(__FreeBSD_kernel__)

static void build_iovec(struct iovec **iov, int *iovlen, const char *name,
		void *val, size_t len)
{
	int i;

	if (*iovlen < 0)
		return;
	i = *iovlen;
	*iov = realloc(*iov, sizeof **iov * (i + 2));
	if (*iov == NULL) {
		*iovlen = -1;
		return;
	}
	(*iov)[i].iov_base = strdup(name);
	(*iov)[i].iov_len = strlen(name) + 1;
	i++;
	(*iov)[i].iov_base = val;
	if (len == (size_t)-1) {
		if (val != NULL)
			len = strlen(val) + 1;
		else
			len = 0;
	}
	(*iov)[i].iov_len = (int)len;
	*iovlen = ++i;
}

int FAST_FUNC xmount(const char *source, const char *target,
		const char *filesystemtype, unsigned long mountflags,
		const void *data UNUSED_PARAM)
{
	struct iovec *iov = NULL;
	int iovlen = 0;
	char *fspath, *from;
	int ret;

	fspath = realpath(target, NULL);
	from = realpath(source, NULL);

	build_iovec(&iov, &iovlen, "fstype", (void*)filesystemtype, (size_t)-1);
	build_iovec(&iov, &iovlen, "fspath", fspath, (size_t)-1);
	build_iovec(&iov, &iovlen, "from", from, (size_t)-1);

	ret = nmount(iov, iovlen, mountflags);

	free(from);
	free(fspath);
	
	return ret;
}

int FAST_FUNC xumount(const char *target, int flags)
{
	return unmount(target, flags);
}

int FAST_FUNC xswapon(const char *path, int swapflags UNUSED_PARAM)
{
	return swapon(path);
}

int FAST_FUNC xswapoff(const char *path)
{
	return swapoff(path);
}

#endif
