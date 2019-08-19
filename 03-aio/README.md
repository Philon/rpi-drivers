# 树莓派驱动开发实战03：异步IO

本文是上一篇《GPIO驱动之按键中断》的扩展，之前的文章侧重于中断原理及Linux/IRQ基础，用驱动实现了一个按键切换LED灯色的功能。但涉及中断的场景，就会拔出萝卜带出泥要实现IO的同步/异步访问方式。归根结底，驱动要站在(用户层)接口调用的角度，除了实现自身的功能，还要为用户层提供一套良好的访问机制。本文主要涉及以下知识点：

- 机制与策略原则
- IO阻塞/非阻塞——read/write
- IO多路复用——select/epoll
- 信号异步通知——signal

## 机制与策略

Linux的核心思想之一就是“一切皆文件”，而这其中最重要的原则便是“提供机制，而非策略”。文件——内核层与用户层分水岭，通过文件，用户可以用简单而统一的方式去访问复杂而多样的设备，且不必操心设备内部的具体细节。然而哪些部分该驱动实现，哪些部分又该留给用户实现，这是需要拿捏的，我个人理解：

- 机制，提供某个功能范围的实现形式、框架、标准
- 策略，提供某种功能的具体实现方法和细节

以上一篇按键中断的驱动为例——“按一下切换一种灯色”，显然，这根本不符合驱动设计原则。首先，驱动把两种设备打包进一个程序；其次，驱动实现了“切换灯色”这个具体业务功能(策略)；最后，驱动根本没有提供用户层的访问机制。

还是回到需求——“按一下切换一种灯色”，理想情况下应该是这样的：
1. LED驱动——向用户层提供灯色切换机制
2. 按键驱动——向用户层提供“按下事件”通知/获取机制
3. 由用户层自行决定收到按键事件后，如何切换灯色

从以上分析看，原本一个驱动代码分拆分成led驱动、按键驱动、切灯app三个部分：led驱动已经在第1章《GPIO驱动之LED》里实现了，因此现在还差两件事：
- 按键驱动，砍掉原有的led功能实现，增加中断标志获取的访问机制
- 切灯app，这个就无所谓了，只要有了机制，策略想怎么写就怎么写

## IO的阻塞/非阻塞访问

阻塞/非阻塞，是访问IO的两种不同机制，即同步和异步获取。

所谓阻塞访问，就是当用户层`read/write`设备文件时，如果预期的结果还没准备好，进程就会被挂起睡眠，让出CPU资源，直到要读写的资源准备好后，重新唤醒进程执行操作。以本文实际的按键中断来说，当用户层读按键设备节点时，只要按键没有被按下，进程就应该一直阻塞在read函数，直到触发中断后才返回。

所谓非阻塞访问，就是调用`open(file, O_NONBLOCK)`或者`ioctl(fd, F_SETFL, O_NONBLOCK)`，将文件句柄设置为非阻塞模式，此时，如果要读写的资源还没准备好，`read/write`会立刻返回`-EAGAIN`错误码，进程不会被挂起。

简而言之：
- 阻塞模式：read/write时会被挂起，让出CPU，无法及时响应后续业务
- 非阻塞模式：read/write时不会挂起，占有CPU，不影响后续程序执行

显然，通过以上介绍，驱动需要在自己的`xxx_read() xxx_write()`等文件接口里实现阻塞功能，在Linux内核中主要通过“等待队列”来实现进程的阻塞与唤醒的。

### 等待队列

等待队列是基于链表实现的数据结构，用来表示某个事件上所有等待的进程。内核接口如下：

```c
#include <linux/wait.h>

// 等待队列链表数据结构
struct wait_queue_head {
	spinlock_t		    lock;
	struct list_head	head;
};
typedef struct wait_queue_head wait_queue_head_t;

// 初始化一个等待队列
#define init_waitqueue_head(wq_head)

// 声明一个等到节点(进程)
#define DECLARE_WAITQUEUE(name, tsk)

// 添加/删除节点到等待队列中
void add_wait_queue(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry);
void remove_wait_queue(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry);

///////////////////////////////////////////
// 往下看之前先记住两个函数
// set_current_state(value) 👈设置当前进程的状态，比如阻塞
// schedule()               👈调度其他进程
///////////////////////////////////////////

// 【接收】开始等待事件到来
// 以下宏，均是由上边的两个函数封装而言，不过是设置进程状态不同罢了
#define wait_event(wq_head, condition)                  // 不可被中断
#define wait_event_interruptible(wq_head, condition)    // 可被信号中断
#define wait_event_timeout(wq_head, condition, timeout) // 会超时
#define wait_event_interruptible_timeout(wq_head, condition, timeout) // 可被信号中断和超时

// 【发送】唤醒队列中的所有等待队列
#define wake_up(x)
#define wake_up_interruptible(x)
```

上边的API比较难理解的就是`wait_event_xxx`和`wake_up_xxx`，其实很简单，一般在驱动的读写接口里调用`wait_xxx`让进程切换到阻塞状态等待唤醒，然后在中断或其他地方调用`wake_up_xxx`即可唤醒队列。再有，通常情况下建议使用`interruptible`模式，否则进程将无法被系统信号中断。

### 最简单的按键阻塞实现

下面来实现按键的阻塞访问，当用`cat /dev/key`命令去读按键是否按下时，应该会被阻塞，直到按键真的被按下。为此，需要实现：
1. 在`init`函数中初始化gpio按键相关，以及一个“等待队列”
2. 在`read`函数中创建一个“等待”，然后进入阻塞，直到被唤醒
3. 在中断响应函数中，唤醒整个“等待队列”

PS：以下驱动也顺便实现了“非阻塞”访问模式，这个其实很简单，无非就是判断以下文件标识是否为`O_NONBLOCK`即可。

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>

#define KEY_GPIO  17

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

static wait_queue_head_t r_wait;

// 读取阻塞
static ssize_t gpiokey_read(struct file *filp, char __user *buf, size_t len, loff_t * off)
{
  int data = 1;
  // 新建一个”等待“并加入队列
  DECLARE_WAITQUEUE(wait, current);
  add_wait_queue(&r_wait, &wait);

  if ((filp->f_flags & O_NONBLOCK) && !gpio_get_value(KEY_GPIO)) {
    // 如果是非阻塞访问，且按键没有按下时，直接返回错误
    return -EAGAIN;
  }

  // 进程进入阻塞状态，等待事件唤醒(可被信号中断)
  wait_event_interruptible(r_wait, gpio_get_value(KEY_GPIO));
  remove_wait_queue(&r_wait, &wait);

  // 被唤醒后，进行业务处理，返回一个“按下”标志给用户进程
  len = sizeof(data);
  if ((data = copy_to_user(buf, &data, len)) < 0) {
    return -EFAULT;
  }
  *off += len;

  return 0;
}

static int press_irq = 0;
static struct timer_list delay;

// 按键中断顶半部响应及防抖延时判断
static irqreturn_t on_key_pressed(int irq, void* dev)
{
  mod_timer(&delay, jiffies + (HZ/20));
  return IRQ_HANDLED;
}
static void on_delay50(struct timer_list* timer)
{
  if (gpio_get_value(KEY_GPIO)) {
    // 按下按键后，唤醒阻塞队列
    wake_up_interruptible(&r_wait);
  }
}

struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = gpiokey_read,
};

struct miscdevice gpiokey = {
  .minor = 1,
  .name = "gpiokey",
  .fops = &fops,
  .nodename = "mykey",
  .mode = 0700,
};

static int __init gpiokey_init(void)
{
  // 初始化定时器，用于防抖延时
  timer_setup(&delay, on_delay50, 0);
  add_timer(&delay);

  // 初始化“读阻塞”等待队列
  init_waitqueue_head(&r_wait);

  // 向内核申请GPIO和IRQ并绑定中断处理函数
  gpio_request_one(KEY_GPIO, GPIOF_IN, "key");
  press_irq = gpio_to_irq(KEY_GPIO);
  if (request_irq(press_irq, on_key_pressed, IRQF_TRIGGER_RISING, "onKeyPress", NULL) < 0) {
    printk(KERN_ERR "Failed to request irq for gpio%d\n", KEY_GPIO);
  }

  // 注册驱动模块并创建设备节点
  misc_register(&gpiokey);
  return 0;
}
module_init(gpiokey_init);

static void __exit gpiokey_exit(void)
{
  misc_deregister(&gpiokey);
  free_irq(0, NULL);
  gpio_free(KEY_GPIO);
  del_timer(&delay);
}
module_exit(gpiokey_exit);
```

![按键中断阻塞访问效果](https://i.loli.net/2019/08/18/iX8BPOrq1pw74aH.gif)

## 多路复用IO模型-poll

如果程序只监听一个设备，那用阻塞或非阻塞足够了，但如果设备数量繁多呢？比如我们的键盘，有一百多个键，难道每次都要全部扫描一遍有没有被按下？(当然，键盘事件有另外的机制，这里只是举个例子)

这个时候轮询操作就非常有用了，据我所知有很多小型的网络服务正是用此机制实现的高性能并发访问，简单来说，就是把成千上万个socket句柄放到一种名叫`FD_SET`的集合里，然后通过`select()/epoll()`同时监听集合里的句柄状态，其中任何一个socket可读写时就唤醒进程并及时响应。

综上，设备驱动要做的，便是实现`select/epoll`的底层接口。而有关select的应用层开发这里就不介绍了，网上一大堆。

驱动模块的多路复用实现其实非常简单，和读写接口一样，你只需实现`file_operations`里的poll接口：
```c
#include <linux/poll.h>

// file_operations->poll
// 由驱动自行实现多路复用功能
__poll_t (*poll) (struct file *, struct poll_table_struct *);

// 在具体实现poll接口是需要调用
// 它负责加入驱动的等待队列，让进程阻塞，直到队列被唤醒
void poll_wait(struct file * filp, wait_queue_head_t * wait_address, poll_table *p);

// 当等待队列被唤醒后，或者异常情况下
// 驱动还需要返回掩码，告诉用户层，设备当前的可操作状态
#define POLLIN		0x0001  // 可读
#define POLLPRI		0x0002  // 紧急数据可读
#define POLLOUT		0x0004  // 可写
#define POLLERR		0x0008  // 错误
#define POLLHUP		0x0010  // 被挂起
#define POLLNVAL	0x0020  // 非法

#define POLLRDNORM	0x0040  // 普通数据可读
#define POLLRDBAND	0x0080  // 优先数据可读
#define POLLWRNORM	0x0100  // 普通数据可写
#define POLLWRBAND	0x0200  // 优先数据可写
#define POLLMSG     0x0400  // 有消息
#define POLLREMOVE	0x1000  // 被移除
#define POLLRDHUP   0x2000  // 读被挂起
```

结合第二小结的等待队列，实现gpio按键的多路复用IO就非常简单了：

```c
// 实现内核的poll接口
static __poll_t gpiokey_poll(struct file *filp, struct poll_table_struct *wait)
{
  __poll_t mask = 0;;
  
  // 加入等待队列
  poll_wait(filp, &r_wait, wait);
  if (gpio_get_value(KEY_GPIO)) {
    // 按键设备不存在写，所以总是返回可读
    mask = POLLIN | POLLRDNORM;
  }

  return mask;
}

// 注意接口要加入到文件操作描述里
struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = gpiokey_read,
  .poll = gpiokey_poll,
};
```