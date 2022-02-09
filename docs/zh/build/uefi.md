# UEFI目标GUI Application(实验性)

## 1. 下载edk2

``` bash
git clone --recursive https://github.com/BigfootACA/simple-init.git
git clone --recursive https://github.com/tianocore/edk2.git
```

## 2. 初始化环境

```bash
export PACKAGES_PATH=$PWD/edk2:$PWD/simple-init
cd edk2
source edksetup.sh
```

## 3. 编辑EmulatorPkg.dsc

使用编辑器打开EmulatorPkg/EmulatorPkg.dsc

```diff
diff --git a/EmulatorPkg/EmulatorPkg.dsc b/EmulatorPkg/EmulatorPkg.dsc
--- a/EmulatorPkg/EmulatorPkg.dsc
+++ b/EmulatorPkg/EmulatorPkg.dsc
@@ -478,6 +479,7 @@
   EmulatorPkg/Application/RedfishPlatformConfig/RedfishPlatformConfig.inf
 !endif
 !include RedfishPkg/Redfish.dsc.inc
+!include SimpleInit.inc
 
 [BuildOptions]
   #
```

## 4. 开始构建

```bash
bash EmulatorPkg/build.sh
```

## 5. 启动UEFI模拟器

``` bash
bash EmulatorPkg/build.sh run
```

然后会打开一个gdb

```gdb
(gdb) run
```

桌面会出现一个`GOP Window`，并进入UEFI Shell

```uefi
FS0:SimpleInit.efi
```

就可进入GUI Application
