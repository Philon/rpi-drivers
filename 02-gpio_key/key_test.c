#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>

int main(int argc, char* argv[])
{
  int fd = open("/dev/key", O_RDONLY);
  if (fd < 0) {
    perror("open");
    exit(0);
  }

  fd_set readfds;
  struct timeval timeout = {2, 0}
  while (1) {
    FD_ZERO(&readfds);
    FD_SET(fd, readfds);
    int rc = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
    if (rc < 0) {
      perror("select");
      break;
    } else if (rc == 0) {
      printf("timeout!\n");
      continue;
    }

    // todo: ...
  }

  close(fd);

  return 0;
}