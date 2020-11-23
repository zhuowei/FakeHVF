#include "fakehvf.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kvm.h>
#include <sys/mman.h>
#include <stdio.h>

// see https://www.kernel.org/doc/html/latest/virt/kvm/api.html

static int gKvmFd;
static int gRunSize;

struct hv_vcpu {
    struct kvm_run *run;
    int fd;
    hv_vcpu_exit_t exit;
};

struct hv_vcpu_config {
};

// VM functions
hv_return_t hv_vm_create(hv_vm_config_t config) {
    if (config) {
        return -1;
    }
    int dev_kvm_fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (dev_kvm_fd == -1) {
        return -1;
    }
    gRunSize = ioctl(dev_kvm_fd, KVM_GET_VCPU_MMAP_SIZE, NULL);
    int fd = ioctl(dev_kvm_fd, KVM_CREATE_VM, KVM_VM_TYPE_ARM_IPA_SIZE(0));
    if (fd == -1) {
        return -1;
    }
    close(dev_kvm_fd);
    gKvmFd = fd;
    return HV_SUCCESS;
}

hv_return_t hv_vm_destroy(void) {
    close(gKvmFd);
    gKvmFd = 0;
    return HV_SUCCESS;
}

hv_return_t hv_vm_map(void *addr, hv_ipa_t ipa, size_t size, hv_memory_flags_t flags) {
    // TODO(zhuowei) HVF has no concept of slots
    static int s_NextSlot = 0;
    struct kvm_userspace_memory_region region = {
        .slot = s_NextSlot++,
        .flags = (flags & HV_MEMORY_WRITE)? 0: KVM_MEM_READONLY,
        .guest_phys_addr = ipa,
        .memory_size = size,
        .userspace_addr = (uint64_t)addr,
    };
    if (ioctl(gKvmFd, KVM_SET_USER_MEMORY_REGION, &region) == -1) {
        return -1;
    }
    return HV_SUCCESS;
}

// CPU functions

hv_vcpu_config_t hv_vcpu_config_create(void) {
    return calloc(1, sizeof(struct hv_vcpu_config));
}

hv_return_t hv_vcpu_create(hv_vcpu_t *vcpu, hv_vcpu_exit_t **exit, hv_vcpu_config_t config) {
    // TODO(zhuowei) support creating more than one VCPU
    int fd = ioctl(gKvmFd, KVM_CREATE_VCPU, 0);
    if (fd == -1) {
        return -1;
    }
    struct kvm_vcpu_init vcpu_init;
    if (ioctl(gKvmFd, KVM_ARM_PREFERRED_TARGET, &vcpu_init) == -1) {
        return -1;
    }
    // ask for psci 0.2
    vcpu_init.features[0] |= 1ul << KVM_ARM_VCPU_PSCI_0_2;
    if (ioctl(fd, KVM_ARM_VCPU_INIT, &vcpu_init) == -1) {
        return -1;
    }
    struct kvm_run *run = mmap(NULL, gRunSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (run == MAP_FAILED) {
        return -1;
    }
    hv_vcpu_t newcpu = calloc(1, sizeof(struct hv_vcpu));
    newcpu->run = run;
    newcpu->fd = fd;
    *vcpu = newcpu;
    *exit = &newcpu->exit;
    return HV_SUCCESS;
}

hv_return_t hv_vcpu_destroy(hv_vcpu_t vcpu) {
    munmap(vcpu->run, gRunSize);
    close(vcpu->fd);
    free(vcpu);
    return HV_SUCCESS;
}

hv_return_t hv_vcpu_run(hv_vcpu_t vcpu) {
    if (ioctl(vcpu->fd, KVM_RUN, NULL) == -1) {
        return -1;
    }
    fprintf(stderr, "run!\n");
    // TODO(zhuowei) update the VMExit
    return HV_SUCCESS;
}


hv_return_t hv_vcpu_set_pending_interrupt(hv_vcpu_t vcpu, hv_interrupt_type_t type, bool pending) {
    struct kvm_irq_level req = {
        .irq = type,
        .level = pending,
    };
    return ioctl(vcpu->fd, KVM_IRQ_LINE, &req);
}

hv_return_t hv_vcpu_get_reg(hv_vcpu_t vcpu, hv_reg_t reg, uint64_t *value) {
    struct kvm_one_reg reg_req = {
        .id = reg,
        .addr = (uint64_t)value,
    };
    if (ioctl(vcpu->fd, KVM_GET_ONE_REG, &reg_req) == -1) {
        return -1;
    }
    return HV_SUCCESS;
}

hv_return_t hv_vcpu_set_reg(hv_vcpu_t vcpu, hv_reg_t reg, uint64_t value) {
    struct kvm_one_reg reg_req = {
        .id = reg,
        .addr = (uint64_t)&value,
    };
    if (ioctl(vcpu->fd, KVM_SET_ONE_REG, &reg_req) == -1) {
        return -1;
    }
    return HV_SUCCESS;
}