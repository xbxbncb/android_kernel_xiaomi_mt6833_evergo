# 你好欢迎来到Redmi Note 11的内核仓库😊😊
由酷安大佬:孤独不能 提供的可编译内核源码
这里的fork是为了帮助小白可以更快的成功编译

教程:
方案1:
### debian12虚拟机💽
1.安装debian12(网上搜)
2.apt安装:

```
sudo apt install -y git ccache automake flex lzop bison gperf build-essential zip curl zlib1g-dev  g++-multilib python-networkx libxml2-utils bzip2 libbz2-dev libbz2-1.0 libghc-bzlib-dev squashfs-tools pngcrush schedtool dpkg-dev liblz4-tool make optipng maven libssl-dev pwgen libswitch-perl policycoreutils minicom libxml-sax-base-perl libxml-simple-perl bc libc6-dev-i386 lib32ncurses5-dev x11proto-core-dev libx11-dev lib32z-dev libgl1-mesa-dev xsltproc unzip bc clang bison build-essential ccache curl flex git git-lfs gnupg gperf imagemagick libelf-dev liblz4-tool libncurses5 gcc-10-aarch64-linux-gnu libncurses5-dev libsdl1.2-dev libssl-dev libxml2 libxml2-utils lzop pngcrush rsync schedtool squashfs-tools xsltproc zip zlib1g-dev lld 
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
ARCH=arm64 make CC="ccache clang"  CCACHE_DIR=".cache" CROSS_COMPILE=aarch64-linux-gnu- CLANG_TRIPLE=aarch64-linux-gnu- CROSS_COMPILE_COMPAT=aarch64-libux-gnueabi- CROSS_COMPILE_ARM32=arm-linux-gnueabi- KCFLAGS="-Wno-unused-but-set-variable -Wno-implicit-function-declaration -Wno-unused-variable -Wno-unused-function -Wno-unused-label" quiet=quiet_ LD=ld.lld LDFLAGS="-fuse-ld=lld" -k -i  evergo_defconfig

#配置配置(如果您有开启docker等功能的需要，若没有请不要执行)
#ARCH=arm64 make menuconfig CC="ccache clang"  CCACHE_DIR=".cache" CROSS_COMPILE=aarch64-linux-gnu- CLANG_TRIPLE=aarch64-linux-gnu- CROSS_COMPILE_COMPAT=aarch64-libux-gnueabi- CROSS_COMPILE_ARM32=arm-linux-gnueabi- KCFLAGS="-Wno-unused-but-set-variable -Wno-implicit-function-declaration -Wno-unused-variable -Wno-unused-function -Wno-unused-label" quiet=quiet_ LD=ld.lld LDFLAGS="-fuse-ld=lld" -k -i  
#开始编译
ARCH=arm64 make CC="ccache clang"  CCACHE_DIR=".cache" CROSS_COMPILE=aarch64-linux-gnu- CLANG_TRIPLE=aarch64-linux-gnu- CROSS_COMPILE_COMPAT=aarch64-libux-gnueabi- CROSS_COMPILE_ARM32=arm-linux-gnueabi- KCFLAGS="-Wno-unused-but-set-variable -Wno-implicit-function-declaration -Wno-unused-variable -Wno-unused-function -Wno-unused-label" quiet=quiet_ LD=ld.lld LDFLAGS="-fuse-ld=lld" -k -i  

```

6.编译完成
示例:
![image](https://github.com/user-attachments/assets/be3bf50f-bf1d-4643-ab5b-022fa075c92d)



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
