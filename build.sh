#!/bin/sh
export PATH="$PATH:/Users/zhuowei/Library/Android/sdk/ndk/21.0.6113669/toolchains/llvm/prebuilt/darwin-x86_64/bin/"
exec aarch64-linux-android29-clang -static -osimplevm -Wno-format-extra-args fakehvf.c simplevm.c
