#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int main(int argc, char* argv[])
{
  if (argc < 3) {
    fprintf(stderr, "\n./rgbled_test <r|g|b> <0|1>\n\n");
    exit(0);
  }

  int fd = open("/dev/rgbled", O_RDWR);
  if (fd < 0) {
    perror("open device");
    return -1;
  }

  if (ioctl(fd, argv[1][0], atoi(argv[2])) < 0) {
    perror("ioctl");
  }

  close(fd);
  return 0;
}
