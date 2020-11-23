#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <inttypes.h>
#ifdef __APPLE__
#include <Hypervisor/Hypervisor.h>
#else
#include "fakehvf.h"
#endif

// ARM64 instructions to compute ((2 + 2) - 1), store it to memory, and reboot
const char kVMCode[] = {
    // Compute ((2 + 2) - 1)
    0x40, 0x00, 0x80, 0xD2, // mov x0, #2
    0x00, 0x08, 0x00, 0x91, // add x0, x0, #2
    0x00, 0x04, 0x00, 0xD1, // sub x0, x0, #1
    // Write it to memory pointed by x1
    0x20, 0x00, 0x00, 0xF9, // str x0, [x1]
    // Reboot the computer with PSCI/SMCCC
    // 0x84000009 is PSCI SYSTEM_RESET using SMC32 calling convention
    0x20, 0x01, 0x80, 0xd2, // mov x0, 0x0009
    0x00, 0x80, 0xb0, 0xf2, // movk x0, 0x8400, lsl #16
    0x02, 0x00, 0x00, 0xD4, // hvc #0
    // Infinite loop
    0x00, 0x00, 0x00, 0x14, // b .
};

void EnsureSuccess(hv_return_t r) {
    if (r != HV_SUCCESS) {
        fprintf(stderr, "Hypervisor call failed: %d\n", r);
        abort();
    }
}
int main(int argc, const char * argv[]) {
    // allocate 1MB of RAM, aligned to page size, for the VM
    const uint64_t kMemSize = 0x100000;
    const uint64_t kMemStart = 0x69420000;
    const uint64_t kMemResultOutOffset = 0x100;
    const uint64_t kMemGuestResultOut = kMemStart + kMemResultOutOffset;
    char* vm_ram = mmap(NULL, kMemSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (vm_ram == MAP_FAILED) {
        abort();
    }
    // copy our code into the VM's RAM
    memcpy(vm_ram, kVMCode, sizeof(kVMCode));

    // Create the VM
    EnsureSuccess(hv_vm_create(NULL));
    // Map the RAM into the VM
    EnsureSuccess(hv_vm_map(vm_ram, kMemStart, kMemSize, HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC));
    // add a virtual CPU to our VM
    hv_vcpu_t vcpu;
    hv_vcpu_exit_t *vcpu_exit;
    EnsureSuccess(hv_vcpu_create(&vcpu, &vcpu_exit, NULL));
    // Set the CPU's PC to execute code from our RAM
    EnsureSuccess(hv_vcpu_set_reg(vcpu, HV_REG_PC, kMemStart));
    // Set CPU's x1 register to point to where we want to read the result
    EnsureSuccess(hv_vcpu_set_reg(vcpu, HV_REG_X1, kMemGuestResultOut));
    // start the VM
    while (true) {
        // Run the VM until a VM exit
        EnsureSuccess(hv_vcpu_run(vcpu));
        fprintf(stderr, "Exited the VM: %d\n", vcpu_exit->reason);
        // TODO(zhuowei): if the reason isn't a PSCI poweroff, keep executing
        // right now we assume the VM shuts down immediately
        break;
    }
    uint64_t result_out = *((uint64_t*)(vm_ram + kMemResultOutOffset));
    fprintf(stderr, "The result is %d\n", (int)result_out);
    // Tear down the VM
    hv_vcpu_destroy(vcpu);
    hv_vm_destroy();
    return 0;
}