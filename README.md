# 树莓派驱动开发实战

本项目是基于Raspberry Pi 3B+平台学习Linux驱动开发的记录与分享，旨于对Linux内核模块机制的熟悉、常见接口的Linux驱动实现、常用模块的原理掌握。原则上，每个驱动模块我都会编写对应的教程（因为我相信掌握知识最有效的方式是理解并能转述）。

## 环境

详细的环境搭建已在[00-hello](./00-hello/README.md)中介绍，以下仅做补充说明。

- 内核：务必按照树莓派所运行的Linux内核版本到官方仓库下载，比如[rpi-4.19.y.tar.gz](https://github.com/raspberrypi/linux/archive/rpi-4.19.y.tar.gz)，通常情况下我会将其放在工程根目录
- IDE：我个人喜欢用vscode，显然本项目的`.vscode目录`是我自己的环境配置，仅做参考
- 交叉编译：建议采用官方提供的cross-toolchain，由于我个人宿主机为macOS环境，交叉编译器为自行构建的[arm-mac-linux-gnueabihf-10.3.0](https://github.com/Philon/macos-crosstools)版本
- 接线图：文章中涉及的电路图采用[Fritzing](https://fritzing.org)绘制，即各目录下的.fzz文件

## 编译规则

`rules.mk`是一套通用的驱动模块及测试用例构建规则，在各驱动源码目录下的`Makefile`中指定模块并将其包含即可。例如：

```Makefile
# ./00-hello/Makefile
obj-m := hello.o
-include ../rules.mk
```

如此这般，便可获得如下方式对项目进行构建：

```sh
make            # 生成全部（内核模块、测试用例、dtbo等）
make clean      # 清理项目
make modules    # 仅编译驱动模块及设备树（若有）
make tests      # 仅编译测试用例
make install    # 安装相应驱动模块、测试用例等至目标开发板
```

### 文件类型

- `xxx_test.c`会被视作测试用例，会生成对应的"xxx_test"程序
- `xxx.dts`会被视作设备树，在编译模块是会同时将其编译"xxx.dtbo"

### Makefile变量

可以在Makefile中配置相关环境或参数，当然它不是必须的，如果觉得麻烦也可以直接去修改rules。不过要注意，任何配置都必须放在"-include ../rules.mk"语句之前，否则不生效。

```Makefile
# ./00-hello/Makefile
obj-m := hello.o

# 指定交叉编译工具链前缀
CROSS_COMPILE = /usr/local/bin/arm-linux-

# 指定内核源码目录
KDIR = /home/user/linux-rpi-4.19.y

# 配置测试用例编译链接参数
LDFLAGS = -lpthread -L/home/user/mylib -lmy

# 指定安装路径（将通过scp命令远程拷贝，即scp <target>:<path>）
INSTALL_PATH = 192.168.1.100:~/modules

# 额外需要安装的文件指定（*.ko *.dtbo *_test将被自动检测并安装）
INSTALL_FILES = file1 file2 file3

-include ../rules.mk
```
