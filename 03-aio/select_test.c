#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <time.h>

int main(int argc, char* argv[])
{
  int fd = open("/dev/mykey", O_RDONLY);
  if (fd < 0) {
    perror("open");
    exit(0);
  }

  fd_set fds;
  while (1) {
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval timeout = {1, 0};
    int rc = select(fd + 1, &fds, NULL, NULL, &timeout);
    if (rc == 0) {
      printf("Timeout!!\n");
      continue;
    } else if (rc < 0) {
      perror("select");
      break;
    } else {
      if (FD_ISSET(fd, &fds)) printf("Key pressed!!");
    }
  }

  close(fd);
  return 0;
}