#include "jansi-util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define JANSI_PREFIX "jansi-"
#define JANSI_LCK_SUFFIX ".so.lck"
#define JANSI_SO_SUFFIX ".so"

Jansi_File_Type jansi_file_type(const char *name) {
  if (strncmp(JANSI_PREFIX, name, strlen(JANSI_PREFIX)) != 0) return JANSI_NONE;

  if (strncmp(JANSI_LCK_SUFFIX, name + strlen(name) - strlen(JANSI_LCK_SUFFIX), strlen(JANSI_LCK_SUFFIX)) == 0)
    return JANSI_LCK;

  if (strncmp(JANSI_SO_SUFFIX, name + strlen(name) - strlen(JANSI_SO_SUFFIX), strlen(JANSI_SO_SUFFIX)) == 0)
    return JANSI_SO;

  fprintf(stderr, "Unexpected Jansi file: %s\n", name);
  return JANSI_NONE;
}

int cp(int fd_to, const char *from) {
  int fd_from;
  char buf[4096];
  ssize_t nread;
  int saved_errno;

  fd_from = open(from, O_RDONLY);
  if (fd_from < 0) return -1;

  while (nread = read(fd_from, buf, sizeof buf), nread > 0) {
    char *out_ptr = buf;
    ssize_t nwritten;

    do {
      nwritten = write(fd_to, out_ptr, nread);

      if (nwritten >= 0) {
        nread -= nwritten;
        out_ptr += nwritten;
      } else if (errno != EINTR) {
        goto out_error;
      }
    } while (nread > 0);
  }

  if (nread == 0) {
    if (close(fd_to) < 0) {
      fd_to = -1;
      goto out_error;
    }
    close(fd_from);

    /* Success! */
    return 0;
  }

out_error:
  saved_errno = errno;

  close(fd_from);
  if (fd_to >= 0) close(fd_to);

  errno = saved_errno;
  return -1;
}
