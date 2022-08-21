# rv6

xv6移植到qemu的sifive_u以及fu740的板子上
本代码基于xv6-k210改编而来

# 文档索引


## 使用方法

```
git clone https://github.com/HUST-OS/xv6-sifive_u.git
cd xv6-sifive_u
```

然后您可以在qemu上进行运行:

```
make qemu
```

如果要生成二进制文件，执行一下命令生成os.bin

```
make all
```


## 调试选项

### 平台选项

在qemu上调试
```
make all platform=qemu
```
在fu740上调试
```
make all platform=sifive_u
```
默认为qemu平台

### 用户态选项

命令行形式（使用user文件夹中的可执行文件）
```
make all init=cmd-user
```
命令行形式（使用sd文件夹中的可执行文件）
```
make all init=cmd-sd
```
执行sd下所有的测试文件
```
make all init=runall
```
helloworld测试用户态
```
make qemu init=hello
```
默认为cmd-user

### 文件系统选项

基于SD卡
```
make all fat=SD
```
基于内存
```
make all fat=RAM
```

## 我们的工作

- 调试操作系统启动的引导程序,使得操作系统多核启动能够顺利执行.
- 调试SBI输入输出函数,使得操作系统能够正常进行键盘输入.
- 调试底层文件系统接口,使用RAM来模拟磁盘.
- 利用spi协议读写sd卡为文件系统提供支持.
- 调试FAT32文件系统,为用户程序提供稳定的文件操作接口.
- 写好相关文档，相关文档放在了doc文件夹中


## 可用分支

`main`分支

`redis`分支