fakehvf implements a subset of Hypervisor.framework's API on top of Linux KVM.

This was originally created to help me learn KVM and Hypervisor.framework.

It's intended to eventually be used for debugging a port of QEMU to macOS Apple Silicon without an Apple Silicon machine.

This is probably completely useless for you.

# Running

To build, install the Android NDK, adjust the path in build.sh, and run it.

You can test this using QEMU to emulate Fedora 33 Server arm64: see startqemu.sh
