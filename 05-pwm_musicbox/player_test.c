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

// 保卫黄河
// static const char* music = 
//   "(5` (4`) 3` 2` 1` 7 1` 0) (3 (2) 3 5 (6 5 6 1`) 5 0) "
//   "(6` (5`) 4` 3` 2` 1` 7 6 5 6 1` 2` 5 6 2` 3` 5 6 3` 4` 5 6 4` 5`) "
//   "1` (1` 3) 5- 1` (1` 3) 5-  (3) 3 (5) 1` 1` (6) 6 (4) 2` 2` "
//   "(5 (6) 5 4) (3 2 3 0) (5 (6) 5 4) (3 2 3 1)"
//   "5 6 1` 3 (5 (3`) 2` 1`) 5 6 3- "
//   "5 6 1` 3 (5 (3`) 2` 1`) 5 6 1`- "
//   "(5 (3 5) 6 5 1` 1`) 0 (5 (3 5) 6 5 2` 2`) 0 "
//   "(5 6 1` 1`) 0 (5 6 2` 2`) 0 (5 (6) 3` 3`) (5 (6) 3` 2` 1`----)";

int readline(FILE* fp, char* line) {
  
  while (!feof(fp) || line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
    fscanf(fp, "%[^\n]%*c", line);
    printf("get line: %s\n", line);
  }
  return strlen(line);
}

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

  while (!feof(music)) {
    char line[128] = {'\0'};
    readline(music, line);
    printf("%s\n", line);
  }
  exit(0);

  // if (ioctl(fd, MUSICBOX_SET_BEAT, 500) < 0
  //    || ioctl(fd, MUSICBOX_SET_VOLUMN, 20) < 0
  //    || ioctl(fd, MUSICBOX_SET_KEY, 'C') < 0) {
  //   perror("ioctl");
  //   exit(0);
  // }

  // char tone[32] = {'\0'};
  // const char* p = music;
  // do {
  //   switch (*p) {
  //   case ' ':
  //     break;
  //   case '(':
  //     ioctl(fd, MUSICBOX_SET_BEAT, ioctl(fd, MUSICBOX_GET_BEAT) / 2);
  //     break;
  //   case ')':
  //     ioctl(fd, MUSICBOX_SET_BEAT, ioctl(fd, MUSICBOX_GET_BEAT) * 2);
  //     break;
  //   }

  //   if (*p >= '0' && *p <= '7') {
  //     int len = (p[1] == '`' || p[1] == '.') ? 2 : 1;
  //     while (p[len] == '-') len++;
  //     memset(tone, '\0', sizeof(tone));
  //     strncpy(tone, p, len);
  //     write(fd, tone, len);
  //     printf("wirte %s\n", tone);
  //   }
  // } while (*++p);

  close(fd);
  fclose(music);
  return 0;
}