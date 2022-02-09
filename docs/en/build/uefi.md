# UEFI Target GUI Applications (EXPERIMENTAL)

## 1. Download edk2

``` bash
git clone --recursive https://github.com/BigfootACA/simple-init.git
git clone --recursive https://github.com/tianocore/edk2.git
```

## 2. Setup Environments

```bash
export PACKAGES_PATH=$PWD/edk2:$PWD/simple-init
cd edk2
source edksetup.sh
```

## 3. Edit EmulatorPkg.dsc
open EmulatorPkg/EmulatorPkg.dsc
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
## 4. Run Build

```bash
bash EmulatorPkg/build.sh
```

## 5. Start UEFI Emulator

``` bash
bash EmulatorPkg/build.sh run
```
then will open a gdb
```gdb
(gdb) run
```
a `GOP window` appears with a UEFI shell
```uefi
FS0:SimpleInit.efi
```
will start GUI Applications
