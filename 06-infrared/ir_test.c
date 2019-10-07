#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <pthread.h>

static const char* keyname[] = {
  [0x45] = "Channel-",
  [0x46] = "Channel",
  [0x47] = "Channel+",
  [0x44] = "Speed-",
  [0x40] = "Speed+",
  [0x43] = "Play/Pause",
  [0x15] = "Vol+",
  [0x07] = "Vol-",
  [0x09] = "EQ",
  [0x16] = "No.0",
  [0x19] = "100+",
  [0x0d] = "200+",
  [0x0c] = "No.1",
  [0x18] = "No.2",
  [0x5e] = "No.3",
  [0x08] = "No.4",
  [0x1c] = "No.5",
  [0x5a] = "No.6",
  [0x42] = "No.7",
  [0x52] = "No.8",
  [0x4a] = "No.9",
};


void* irrecv(void* param) {
  int ir = open("/dev/ir0", O_RDONLY);

  while (1) {
    int frame = 0;
    if (read(ir, &frame, sizeof(int)) < 0) {
      perror("read ir");
      break;
    }

    int cmd = (frame >> 16) & 0xFF;
    printf("%s\n", keyname[cmd]);
  }

  close(ir);
  return NULL;
}

int main(int argc, char* argv[]) {
  pthread_t t = 0;
  pthread_create(&t, NULL, irrecv, NULL);
  printf("Press any key to quit!\n");
  getchar();
  pthread_detach(t);
  return 0;
}