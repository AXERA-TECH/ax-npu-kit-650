# 源码编译

## AX650N（交叉编译）

### 1. 编译环境
- cmake >= 3.13
- 交叉编译工具链，可按如下操作下载和配置
```shell
wget https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
tar -xvf gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
export PATH=$PATH:$PWD/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin/
```

### 2. 下载源码
```shell
git clone https://github.com/AXERA-TECH/skel.git
cd skel
```

### 3. 编译
```shell
chmod +x build_mc50.sh
./build_mc50.sh
```

编译完成后会在 build/install 下生成如下文件：
```shell
build/install
├── bin
│   └── hvcfp_demo
├── include
│   ├── ax_skel_api.h
│   ├── ax_skel_err.h
│   └── ax_skel_type.h
└── lib
    └── libax_skel.so
```