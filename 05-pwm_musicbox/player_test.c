#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include "musicbox.h"

#define MUSIC_BOX_FILE  "/dev/musicbox"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Usage: ./player <musicfile>");
    return -1;
  }

  int fd = open(MUSIC_BOX_FILE, O_RDWR);
  if (fd < 0) {
    perror("open musicbox");
    exit(0);
  }

  FILE* music = fopen(argv[1], "r");
  if (music == NULL) {
    perror("open music");
    exit(0);
  }

  char line[128] = {'\0'};
  if (fgets(line, sizeof(line), music) == NULL) {
    perror("read music");
    exit(0);
  }

  // 4秒为一节，计算每拍的长度，如2/4拍时，每拍长度为500ms
  int beat = 4000 / (line[4]-'0') / (line[2]-'0');
  if (ioctl(fd, MUSICBOX_SET_BEAT, beat) < 0
   || ioctl(fd, MUSICBOX_SET_VOLUMN, 90) < 0
   || ioctl(fd, MUSICBOX_SET_KEY, line[0]) < 0) {
    perror("ioctl");
    exit(0);
  }

  while (fgets(line, sizeof(line), music)) {
    printf("%s", line);
    if (line[0] == '#' || line[0] == '\0') {
      continue;
    }

    char* p = line;
    while (*p) {
      if (*p == '(') {
        ioctl(fd, MUSICBOX_SET_BEAT, ioctl(fd, MUSICBOX_GET_BEAT) / 2);
      } else if (*p == ')') {
        ioctl(fd, MUSICBOX_SET_BEAT, ioctl(fd, MUSICBOX_GET_BEAT) * 2);
      } else if (*p >= '0' && *p <= '7') {
        char* q = p+1;
        while (*q == '`') q++;
        while (*q == '.') q++;
        while (*q == '-') q++;
        write(fd, p, q-p);
      }
      p++;
    }
  }

  close(fd);
  fclose(music);
  return 0;
}