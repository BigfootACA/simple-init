# UEFI Target GUI Applications (EXPERIMENTAL)

## 1. Download edk2 and edk2-libc

``` bash
git clone --recursive https://github.com/BigfootACA/simple-init.git
git clone --recursive https://github.com/tianocore/edk2.git
git clone https://github.com/tianocore/edk2-libc
```

## 2. Setup Environments

```bash
export PACKAGES_PATH=$PWD/edk2:$PWD/edk2-libc:$PWD/simple-init
cd edk2
source edksetup.sh
```

## 3. Edit EmulatorPkg.dsc
open EmulatorPkg/EmulatorPkg.dsc
```diff
diff --git a/EmulatorPkg/EmulatorPkg.dsc b/EmulatorPkg/EmulatorPkg.dsc
--- a/EmulatorPkg/EmulatorPkg.dsc
+++ b/EmulatorPkg/EmulatorPkg.dsc
@@ -319,6 +319,7 @@
   MdeModulePkg/Universal/ReportStatusCodeRouter/Pei/ReportStatusCodeRouterPei.inf
   MdeModulePkg/Universal/StatusCodeHandler/Pei/StatusCodeHandlerPei.inf
 
+  SimpleInit.inf
   EmulatorPkg/BootModePei/BootModePei.inf
   MdeModulePkg/Universal/FaultTolerantWritePei/FaultTolerantWritePei.inf
   MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
@@ -478,6 +479,7 @@
   EmulatorPkg/Application/RedfishPlatformConfig/RedfishPlatformConfig.inf
 !endif
 !include RedfishPkg/Redfish.dsc.inc
+!include StdLib/StdLib.inc
 
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
