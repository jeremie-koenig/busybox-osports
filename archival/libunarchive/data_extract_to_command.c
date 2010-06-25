/* vi: set sw=4 ts=4: */
/*
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include "libbb.h"
#include "unarchive.h"

enum {
	//TAR_FILETYPE,
	TAR_MODE,
	TAR_FILENAME,
	TAR_REALNAME,
#if ENABLE_FEATURE_TAR_UNAME_GNAME
	TAR_UNAME,
	TAR_GNAME,
#endif
	TAR_SIZE,
	TAR_UID,
	TAR_GID,
	TAR_MAX,
};

static const char *const tar_var[] = {
	// "FILETYPE",
	"MODE",
	"FILENAME",
	"REALNAME",
#if ENABLE_FEATURE_TAR_UNAME_GNAME
	"UNAME",
	"GNAME",
#endif
	"SIZE",
	"UID",
	"GID",
};

static void xputenv(char *str)
{
	if (putenv(str))
		bb_error_msg_and_die(bb_msg_memory_exhausted);
}

static void str2env(char *env[], int idx, const char *str)
{
	env[idx] = xasprintf("TAR_%s=%s", tar_var[idx], str);
	xputenv(env[idx]);
}

static void dec2env(char *env[], int idx, unsigned long long val)
{
	env[idx] = xasprintf("TAR_%s=%llu", tar_var[idx], val);
	xputenv(env[idx]);
}

static void oct2env(char *env[], int idx, unsigned long val)
{
	env[idx] = xasprintf("TAR_%s=%lo", tar_var[idx], val);
	xputenv(env[idx]);
}

void FAST_FUNC data_extract_to_command(archive_handle_t *archive_handle)
{
	file_header_t *file_header = archive_handle->file_header;

#if 0 /* do we need this? ENABLE_FEATURE_TAR_SELINUX */
	char *sctx = archive_handle->tar__next_file_sctx;
	if (!sctx)
		sctx = archive_handle->tar__global_sctx;
	if (sctx) { /* setfscreatecon is 4 syscalls, avoid if possible */
		setfscreatecon(sctx);
		free(archive_handle->tar__next_file_sctx);
		archive_handle->tar__next_file_sctx = NULL;
	}
#endif

	if ((file_header->mode & S_IFMT) == S_IFREG) {
		pid_t pid;
		int p[2], status;
		char *tar_env[TAR_MAX];

		memset(tar_env, 0, sizeof(tar_env));

		xpipe(p);
		pid = BB_MMU ? fork() : vfork();
		switch (pid) {
		case -1:
			bb_perror_msg_and_die(BB_MMU ? "fork" : "vfork");
		case 0:
			/* Child */
			/* str2env(tar_env, TAR_FILETYPE, "f"); - parent should do it once */
			oct2env(tar_env, TAR_MODE, file_header->mode);
			str2env(tar_env, TAR_FILENAME, file_header->name);
			str2env(tar_env, TAR_REALNAME, file_header->name);
#if ENABLE_FEATURE_TAR_UNAME_GNAME
			str2env(tar_env, TAR_UNAME, file_header->tar__uname);
			str2env(tar_env, TAR_GNAME, file_header->tar__gname);
#endif
			dec2env(tar_env, TAR_SIZE, file_header->size);
			dec2env(tar_env, TAR_UID, file_header->uid);
			dec2env(tar_env, TAR_GID, file_header->gid);
			close(p[1]);
			xdup2(p[0], STDIN_FILENO);
			signal(SIGPIPE, SIG_DFL);
			execl("/bin/sh", "/bin/sh" + 5, "-c", archive_handle->tar__to_command, NULL);
			bb_perror_msg_and_die("can't execute '%s'", "/bin/sh");
		}
		close(p[0]);
		/* Our caller is expected to do signal(SIGPIPE, SIG_IGN)
		 * so that we don't die if child don't read all the input: */
		bb_copyfd_exact_size(archive_handle->src_fd, p[1], file_header->size);
		close(p[1]);

		if (safe_waitpid(pid, &status, 0) == -1)
			bb_perror_msg_and_die("waitpid");
		if (WIFEXITED(status) && WEXITSTATUS(status))
			bb_error_msg_and_die("'%s' returned status %d",
				archive_handle->tar__to_command, WEXITSTATUS(status));
		if (WIFSIGNALED(status))
			bb_error_msg_and_die("'%s' terminated on signal %d",
				archive_handle->tar__to_command, WTERMSIG(status));

		if (!BB_MMU) {
			int i;
			for (i = 0; i < TAR_MAX; i++) {
				if (tar_env[i])
					bb_unsetenv_and_free(tar_env[i]);
			}
		}
	}

#if 0 /* ENABLE_FEATURE_TAR_SELINUX */
	if (sctx)
		/* reset the context after creating an entry */
		setfscreatecon(NULL);
#endif
}
