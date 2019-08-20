#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/fcntl.h>

void on_keypress(int sig)
{
  printf("key pressed!!\n");
}

int main(int argc, char* argv[])
{
  // 第一步：将驱动的拥有者指向本进程，否则设备信号不知道发给谁
  int oflags = 0;
  int fd = open("/dev/mykey", O_RDONLY);
  fcntl(fd, F_SETOWN, getpid());
  oflags = fcntl(fd, F_GETFL) | O_ASYNC;
  fcntl(fd, F_SETFL, oflags);

  // 第二步：捕获想要的信号，并绑定到相关处理函数
  signal(SIGIO, on_keypress);

  // 以下无关紧要，就是等到程序退出
  printf("I'm doing something ...\n");
  getchar();

  close(fd);
  return 0;
}