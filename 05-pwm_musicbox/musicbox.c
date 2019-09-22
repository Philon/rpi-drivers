#include <linux/module.h>
#include <linux/pwm.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/timer.h>
#include <linux/wait.h>

#include "musicbox.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

#define MUSICBOX_MAX_VOLUME 100 // 最大音量100%
#define ONE_SECOND  1000000000  // 以纳秒为单位的一秒

static const int tones[][14] = {
  //   C    D     E     F     G     A     B
  //   1.   2.    3.    4.    5.    6.    7.
  {0, 131, 147,  165,  175,  196,  221,  248,  278,  312,  330,  371,  416,  476},
  //   1    2     3     4     5     6     7 
  {0, 262, 294,  330,  350,  393,  441,  496,  556,  624,  661,  742,  833,  935},
  //   1'   2'    3'    4'    5'    6'    7'
  {0, 525, 589,  661,  700,  786,  882,  990,  1112, 1248, 1322, 1484, 1665, 1869},
};

static struct {
  bool                playing;    // 是否正在播放
  wait_queue_head_t   wwait;      // 写等待
  struct pwm_device*  buzzer;     // 蜂鸣器
  struct timer_list   timer;      // 定时器
  char                volume;     // 音量 0-100
  char                tonality;   // 音调 A-G
  char                beat;       // 节拍
  char                key;        // 音调
} musicbox;

static void music_stop(struct timer_list* timer) {
  pwm_disable(musicbox.buzzer);
  musicbox.playing = false;
  wake_up(&musicbox.wwait);
}

static ssize_t musicbox_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
  int note = (buf[0] - '0') + musicbox.key;
  int pitch = (buf[1] == '`') ? 2 : (buf[1] == '.' ? 0 : 1);
  int tone = ONE_SECOND / tones[pitch][note];
  int volume = tone * musicbox.volume / 100;
  int delay = HZ / musicbox.beat * (len - (pitch != 1)); // 但音符后跟着'-'就延长一倍，如2--

  if (musicbox.playing) {
    if (filp->f_flags & O_NONBLOCK) {
      return -EAGAIN;
    } else {
      DECLARE_WAITQUEUE(wq, current);
      add_wait_queue(&musicbox.wwait, &wq);
      wait_event(musicbox.wwait, !musicbox.playing);
      remove_wait_queue(&musicbox.wwait, &wq);
    }
  }

  pwm_config(musicbox.buzzer, volume, tone);
  if (buf[0] > '0') {
    pwm_enable(musicbox.buzzer);
  }
  mod_timer(&musicbox.timer, jiffies + delay);
  musicbox.playing = true;

  return len;
}

static long musicbox_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  switch (cmd) {
  case MUSICBOX_SET_VOLUMN:
    if (arg >= 0 && arg <= MUSICBOX_MAX_VOLUME) {
      musicbox.volume = arg;
    } else {
      return -EINVAL;
    }
    break;
  case MUSICBOX_GET_VOLUMN:
    return musicbox.volume;
  case MUSICBOX_SET_BEAT:
    if (arg > 0 && arg <= 1000) {
      musicbox.beat = 1000 / arg;
    } else {
      return -EINVAL;
    }
    break;
  case MUSICBOX_GET_BEAT:
    return 1000 / musicbox.beat;
  case MUSICBOX_SET_KEY:
    if (arg < 'A' || arg > 'G') {
      return -EINVAL;
    }
    musicbox.key = arg >= 'C' ? (arg - 'C') : (arg - 'A' + 5);
    break;
  case MUSICBOX_GET_KEY:
    return musicbox.key;
  default:
    printk("error cmd = %d\n", cmd);
    return -EFAULT;
  }
  return 0;
}

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .write = musicbox_write,
	.unlocked_ioctl = musicbox_ioctl,
};

static struct miscdevice mdev = {
  .minor = MISC_DYNAMIC_MINOR,
  .name = "musicbox",
  .fops = &fops,
  .nodename = "musicbox",
  .mode = S_IRWXUGO,
};

int __init musicbox_init(void) {
  musicbox.buzzer = pwm_request(0, "Buzzer");
  if (IS_ERR_OR_NULL(musicbox.buzzer)) {
    printk(KERN_ERR "failed to request pwm0\n");
    return PTR_ERR(musicbox.buzzer);
  }

  musicbox.volume = 50;
  musicbox.tonality = 'A';
  musicbox.key = 0;
  musicbox.beat = 4;

  init_waitqueue_head(&musicbox.wwait);
  timer_setup(&musicbox.timer, music_stop, 0);
  add_timer(&musicbox.timer);

  misc_register(&mdev);

  return 0;
}
module_init(musicbox_init);

void __exit musicbox_exit(void) {
  misc_deregister(&mdev);
  del_timer(&musicbox.timer);
  pwm_disable(musicbox.buzzer);
  pwm_free(musicbox.buzzer);
}
module_exit(musicbox_exit);