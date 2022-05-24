[Defines]
  PLATFORM_NAME                  = SimpleInit
  PLATFORM_GUID                  = EC0EB785-EFD9-4EC2-89D0-FE3757338606
  PLATFORM_VERSION               = 1.02
  DSC_SPECIFICATION              = 0x00010006
  SUPPORTED_ARCHITECTURES        = IA32|X64|ARM|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

!include SimpleInit.inc
!include MdePkg/MdeLibs.dsc.inc

[PcdsFixedAtBuild.common]
!if $(TARGET) != RELEASE
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000040
!else
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0e
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
!endif
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x06
  gSimpleInitTokenSpaceGuid.PcdGuiDefaultMouseScale|1

[LibraryClasses]
!if $(TARGET) != RELEASE
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
!else
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
!endif
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  RngLib|MdePkg/Library/DxeRngLib/DxeRngLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf

[LibraryClasses.AARCH64, LibraryClasses.ARM]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  ArmGicLib|ArmPkg/Drivers/ArmGic/ArmGicLib.inf
  ArmGicArchLib|ArmPkg/Library/ArmGicArchLib/ArmGicArchLib.inf
  ArmGenericTimerCounterLib|ArmPkg/Library/ArmGenericTimerPhyCounterLib/ArmGenericTimerPhyCounterLib.inf
  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  TimerLib|ArmPkg/Library/ArmArchTimerLib/ArmArchTimerLib.inf
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf

[LibraryClasses.X64, LibraryClasses.Ia32]
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[BuildOptions]
  GCC:*_*_*_CC_FLAGS = -nostdlib -nostdinc -nolibc
  GCC:*_*_IA32_CC_FLAGS = -DNO_TIMER -static-libgcc -lgcc
  GCC:*_*_X64_CC_FLAGS = -DNO_TIMER
  GCC:*_*_AARCH64_CC_FLAGS = -mno-outline-atomics
  GCC:*_*_ARM_CC_FLAGS = -lgcc
