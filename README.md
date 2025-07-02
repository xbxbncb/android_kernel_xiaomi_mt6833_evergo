# 你好欢迎来到Redmi Note 11的内核仓库😊😊
由酷安大佬:孤独不能 提供的可编译内核源码
这里的fork是为了帮助小白可以更快的成功编译

## 教程:

### 方案1:debian12虚拟机💽
1.安装debian12(网上搜)
2.apt安装:

```
sudo apt update && sudo apt install -y bc bison build-essential ccache curl flex git git-lfs \
gnupg gperf imagemagick libelf-dev liblz4-tool libncurses5 libncurses5-dev \
libsdl1.2-dev libssl-dev libxml2 libxml2-utils lzop pngcrush rsync schedtool \
squashfs-tools xsltproc zip zlib1g-dev lld gcc-aarch64-linux-gnu g++-aarch64-linux-gnu cpio clang

```

3.克隆仓库

```
git clone https://github.com/cuicanmx/android_kernel_xiaomi_mt6833_evergo.git
```
本fork保证可以编译成功

4.配置ccache(加速编译)
```
export CCACHE_DIR=".cache"
ccache -M 20G
```

5.开始编译

```
#使用默认配置
make ARCH=arm64 \
CC="ccache clang" \
CROSS_COMPILE=aarch64-linux-gnu- \
CROSS_COMPILE_ARM32=arm-linux-gnueabi- \
CCACHE_DIR="/home/c/.ccache" \
CLANG_TRIPLE=aarch64-linux-gnu- \
LD=ld.lld \
KCFLAGS="-Wno-unused-but-set-variable -Wno-implicit-function-declaration -Wno-unused-variable -Wno-unused-function -Wno-unused-label" quiet=quiet_  \
-j$(nproc) -k O=out evergo_defconfig
```
```
#配置配置(如果您有开启docker等功能的需要，若没有请不要执行)
#make ARCH=arm64 \
#CROSS_COMPILE=aarch64-linux-gnu- \
#CROSS_COMPILE_ARM32=arm-linux-gnueabi- \
#CC="ccache clang" \
#CCACHE_DIR="/home/c/.ccache" \
#CLANG_TRIPLE=aarch64-linux-gnu- \
#LD=ld.lld \
#KCFLAGS="-Wno-error=unused-but-set-variable -Wno-implicit-function-declaration" \
# menuconfig -j$(nproc) O=out
```
```
#开始编译
make ARCH=arm64 \
CC="ccache clang" \
CROSS_COMPILE=aarch64-linux-gnu- \
CROSS_COMPILE_ARM32=arm-linux-gnueabi- \
CCACHE_DIR="/home/c/.ccache" \
CLANG_TRIPLE=aarch64-linux-gnu- \
LD=ld.lld \
KCFLAGS="-Wno-error=unused-but-set-variable -Wno-implicit-function-declaration" \
 -j$(nproc) -k O=out

```

6.编译完成
示例:
![image](https://github.com/user-attachments/assets/be3bf50f-bf1d-4643-ab5b-022fa075c92d)


重新编译：
```
make clean
make mrproper
```

### 方案2:Github Action(不可用目前)
1.fork本项目
2.点击Action
![image](https://github.com/user-attachments/assets/f5985f53-9d4f-4b0e-a03d-6782214a0041)
3.点击Kernel Build and Release 然后     Run workflow ！  
![image](https://github.com/user-attachments/assets/a62eed5e-1766-444c-af30-90b2570e13c3)
4.首次编译大约20-30min，第二次编译会使用缓存(保存7天)

Linux kernel
============

This file was moved to Documentation/admin-guide/README.rst

Please notice that there are several guides for kernel developers and users.
These guides can be rendered in a number of formats, like HTML and PDF.

In order to build the documentation, use ``make htmldocs`` or
``make pdfdocs``.

There are various text files in the Documentation/ subdirectory,
several of them using the Restructured Text markup notation.
See Documentation/00-INDEX for a list of what is contained in each file.

Please read the Documentation/process/changes.rst file, as it contains the
requirements for building and running the kernel, and information about
the problems which may result by upgrading your kernel.
