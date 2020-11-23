#pragma once
#include <stdint.h>
#include <stdbool.h>

// This is a fake shim version of Hypervisor.framework on Linux KVM.
// It's used for testing a port of QEMU to macOS Apple Silicon
// without a real Apple Silicon computer.
// Intended to be somewhat source compatible.

#define HV_SUCCESS 0
typedef int hv_return_t;

// Note: not an Objective-C struct here!
typedef struct hv_vm_config * hv_vm_config_t;

//typedef uint64_t hv_vcpu_t;
// This is an opaque pointer since we need to store the vcpu exit structure somewhere
typedef struct hv_vcpu * hv_vcpu_t;
typedef enum {
    HV_INTERRUPT_TYPE_IRQ = 0,
    HV_INTERRUPT_TYPE_FIQ = 1,
} hv_interrupt_type_t;

typedef enum {
    HV_EXIT_REASON_CANCELED = 0,
    HV_EXIT_REASON_EXCEPTION = 1,
    HV_EXIT_REASON_VTIMER_ACTIVATED = 2,
    HV_EXIT_REASON_UNKNOWN = 3,
} hv_exit_reason_t;

typedef struct hv_vcpu_exit_exception {
    uint64_t syndrome;
    uint64_t virtual_address;
    uint64_t physical_address;
} hv_vcpu_exit_exception_t;

typedef struct hv_vcpu_exit {
    hv_vcpu_exit_exception_t exception;
    hv_exit_reason_t reason;
} hv_vcpu_exit_t;

// Note: not an Objective-C struct here!
typedef struct hv_vcpu_config * hv_vcpu_config_t;

typedef enum {
    HV_MEMORY_READ,
    HV_MEMORY_WRITE,
    HV_MEMORY_EXEC,
} hv_memory_flags_t;

typedef uint64_t hv_ipa_t;

// see https://www.kernel.org/doc/html/latest/virt/kvm/api.html
// "arm64 core/FP-SIMD registers have the following id bit patterns"
typedef enum {
    HV_REG_X0 = 0x6030000000100000,
    HV_REG_X1 = 0x6030000000100002,
    HV_REG_PC = 0x6030000000100040,
} hv_reg_t;

// VM functions
hv_return_t hv_vm_create(hv_vm_config_t config);
hv_return_t hv_vm_destroy(void);

// Memory functions
hv_return_t hv_vm_map(void *addr, hv_ipa_t ipa, size_t size, hv_memory_flags_t flags);

// VCPU functions

hv_vcpu_config_t hv_vcpu_config_create(void);
hv_return_t hv_vcpu_create(hv_vcpu_t *vcpu, hv_vcpu_exit_t **exit, hv_vcpu_config_t config);
hv_return_t hv_vcpu_destroy(hv_vcpu_t vcpu);
hv_return_t hv_vcpu_run(hv_vcpu_t vcpu);
hv_return_t hv_vcpu_set_pending_interrupt(hv_vcpu_t vcpu, hv_interrupt_type_t type, bool pending);
hv_return_t hv_vcpu_get_reg(hv_vcpu_t vcpu, hv_reg_t reg, uint64_t *value);
hv_return_t hv_vcpu_set_reg(hv_vcpu_t vcpu, hv_reg_t reg, uint64_t value);