# simple-init confd Config Guide

## runtime

This key is reserved for program runtime and does not allow save or load from local files

## boot

Used to store simple-init boot options, or boot menu items

Supported environments: `LINUX` | `UEFI`

Detailed options: [boot](boot.md)

## logger

Special settings for logger service

Supported environments: `LINUX` | `UEFI`

Detailed options: [logger](logger.md)

## uefi

Special settings under UEFI

Supported environments: `UEFI`

Detailed options: [uefi](uefi.md)
