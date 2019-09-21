#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include "musicbox.h"

#define MUSIC_BOX_FILE  "/dev/musicbox"

// 《芒种》乐谱
// static const char* music = 
//   "0 0 ((3` 1`) 2`) (6 5) 6 (3` 5`) (3` 5`) (2` 3') (6 7) (1` 6) 5 - 0 "
//   "5 (3` 5`) (3` 6) 1' (2` 2`) (2` 1`) 3` 1' 6 - - - ";

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

  // 节拍以一秒为基准，2/4拍即没拍500ms
  int beat = 800 * (line[2]-'0') / (line[4]-'0');
  if (ioctl(fd, MUSICBOX_SET_BEAT, beat) < 0
   || ioctl(fd, MUSICBOX_SET_VOLUMN, 80) < 0
   || ioctl(fd, MUSICBOX_SET_KEY, line[0]) < 0) {
    perror("ioctl");
    exit(0);
  }

  while (fgets(line, sizeof(line), music)) {
    // char line[128] = {'\0'};
    // readline(music, line);
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