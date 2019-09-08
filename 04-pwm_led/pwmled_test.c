#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "pwmled.h"

#define PWM_LED "/dev/pwmled"

void init_stdin() {
  struct termios term;
  tcgetattr(0, &term);
  term.c_lflag &= (~ICANON);
  term.c_cc[VTIME] = 0;
  term.c_cc[VMIN] = 1;
  tcsetattr(0, TCSANOW, &term);
}

int main(int argc, char* argv[]) {
  int fd = open(PWM_LED, O_RDWR);
  int brightness = 0;
  char key = 0;

  init_stdin();
  while ((key = getchar()) != 'q') {
    switch (key) {
    case '=':
      brightness += brightness < PWMLED_MAX_BRIGHTNESS ? 10 : 0;
      break;
    case '-':
      brightness -= brightness > 0 ? 10 : 0;
      break;
    case '9':
      brightness = PWMLED_MAX_BRIGHTNESS;
      break;
    case '0':
      brightness = 0;
      break;
    default:
      // printf("unknown opt: '%c'\n", key);
      break;
    }

    if (ioctl(fd, PWMLED_CMD_SET_BRIGHTNESS, brightness) < 0) {
      perror("ioctl");
      break;
    }
  } while (key != 'q');

  close(fd);
  return 0;
}