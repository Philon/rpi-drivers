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

  char led = 0;
  switch (argv[1][0]) {
    case 'r': case 'R':
      led = 2;
      break;
    case 'g': case 'G':
      led = 3;
      break;
    case 'b': case 'B':
      led = 4;
      break;
  }
  char stat = atoi(argv[2]);
  if (ioctl(fd, led, stat) < 0) {
    perror("ioctl");
    return -1;
  }

  close(fd);
  return 0;
}
