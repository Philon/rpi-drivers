#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

#define PWM_LED "/dev/pwmled"

int main(int argc, char* argv[]) {
  struct termios term;
  tcgetattr(0, &term);
  term.c_lflag &= (~ICANON);
  term.c_cc[VTIME] = 0;
  term.c_cc[VMIN] = 1;
  tcsetattr(0, TCSANOW, &term);

  int fd = open(PWM_LED, O_RDWR);

  char key = 0;
  while ((key = getchar()) != 'q') {
    putchar('\b');

    switch (key) {
    case '=':
      ioctl(fd, 1);
      break;
    case '-':
      ioctl(fd, -1);
      break;
    default:
      printf("press %c\n", key);
      break;
    }
  }
  return 0;
}