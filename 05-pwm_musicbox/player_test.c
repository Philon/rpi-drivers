#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#define MUSIC_BOX_SET_BEAT 1

#define MUSIC_BOX_FILE  "/dev/musicbox"

// 《芒种》乐谱
static const char* beat = "4/4";
static const char* music[] = {
  "0 0 ((3' 1') 2') (6 6)",
  "6 (3' 5') (3' 5') (2' 3')",
  "(6 7) (1' 6) 5 -",
  "5 (3' 5') (3' 6) 1'",
  "(2' 2') (2' 1') 3' 1'",
  "6 - - -",
};

int main(int argc, char* argv)
{
  int fd = open(MUSIC_BOX_FILE, O_WRONLY);
  if (fd < 0) {
    perror("open");
    exit(0);
  }

  if (ioctl(fd, MUSIC_BOX_SET_BEAT, beat) < 0) {
    perror("ioctl");
    exit(0);
  }

  char* section = music;
  while (section) {
    if (write(fd, section, strlen(section)) < 0) {
      perror("write");
      break;
    }
    section++;
  }

  close(fd);
  return 0;
}