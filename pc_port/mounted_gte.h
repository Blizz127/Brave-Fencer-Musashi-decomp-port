/* SPDX-License-Identifier: MIT */
#ifndef BFM_MOUNTED_GTE_H
#define BFM_MOUNTED_GTE_H

#include <stdint.h>

/* Include after MusashiCpu{Context,TransferDevice} and bfm_generic_bus.
 * The CPU owner authenticates the image, PC, word, scene, CU2 permission,
 * delay state and cycle allowance on BOTH sides of its single clock advance.
 * gpr[] is the live register file visible to the issuing instruction, before
 * prior delayed GPR results retire. The provider owns all GTE register effects.
 */
enum bfm_gte_status {
    BFM_GTE_OK,
    BFM_GTE_UNHANDLED,
    BFM_GTE_DENIED,
    BFM_GTE_LOAD_ADDRESS_ERROR,
    BFM_GTE_STORE_ADDRESS_ERROR,
    BFM_GTE_STALE,
    BFM_GTE_CALLBACK_FAILED
};

enum bfm_gte_operation {
    BFM_GTE_MFC2, BFM_GTE_CFC2, BFM_GTE_MTC2, BFM_GTE_CTC2,
    BFM_GTE_COP2, BFM_GTE_LWC2, BFM_GTE_SWC2
};

struct bfm_gte_request {
    enum bfm_gte_operation operation;
    uint32_t word;
    unsigned gte_reg;
    unsigned cpu_reg;
    uint32_t operand; /* effective byte address or visible CPU source */
    const MusashiCpuContext *cpu;
    const MusashiCpuTransferDevice *device;
    MusashiCpuTransferDevice device_binding;
    const struct bfm_generic_bus *bus;
    struct bfm_generic_bus bus_binding;
};

struct bfm_gte_delayed_result {
    unsigned reg; /* zero means no write to enqueue */
    uint32_t value;
};

static inline enum bfm_gte_status
bfm_gte_prepare(uint32_t word, const uint32_t gpr[32], const MusashiCpuContext *cpu,
                const MusashiCpuTransferDevice *device, const struct bfm_generic_bus *bus,
                struct bfm_gte_request *request)
{
    unsigned primary = word >> 26;
    unsigned rs = (word >> 21) & 31u;
    unsigned rt = (word >> 16) & 31u;
    struct bfm_gte_request p = {0};
    if (primary == 0x12u) {
        if (rs & 16u) {
            p.operation = BFM_GTE_COP2;
        } else {
            /* Reach only standard transfer encodings. The CPU owner handles
             * coprocessor branches, reserved encodings and other exceptions. */
            if (word & 0x7ffu) return BFM_GTE_UNHANDLED;
            switch (rs) {
            case 0: p.operation = BFM_GTE_MFC2; break;
            case 2: p.operation = BFM_GTE_CFC2; break;
            case 4: p.operation = BFM_GTE_MTC2; break;
            case 6: p.operation = BFM_GTE_CTC2; break;
            default: return BFM_GTE_UNHANDLED;
            }
            p.gte_reg = (word >> 11) & 31u;
            p.cpu_reg = rt;
        }
    } else if (primary == 0x32u || primary == 0x3au) {
        p.operation = primary == 0x32u ? BFM_GTE_LWC2 : BFM_GTE_SWC2;
        p.gte_reg = rt;
        p.cpu_reg = rs;
    } else {
        return BFM_GTE_UNHANDLED;
    }
    if (!request || !cpu || !device || !gpr) return BFM_GTE_DENIED;
    p.word = word;
    p.cpu = cpu;
    p.device = device;
    p.device_binding = *device;
    switch (p.operation) {
    case BFM_GTE_COP2:
        if (!device->command) return BFM_GTE_DENIED;
        break;
    case BFM_GTE_MFC2:
        if (!device->read_data) return BFM_GTE_DENIED;
        break;
    case BFM_GTE_CFC2:
        if (!device->read_control) return BFM_GTE_DENIED;
        break;
    case BFM_GTE_MTC2:
    case BFM_GTE_CTC2:
        if (p.operation == BFM_GTE_MTC2 ? !device->write_data : !device->write_control)
            return BFM_GTE_DENIED;
        p.operand = rt ? gpr[rt] : 0;
        break;
    case BFM_GTE_LWC2:
    case BFM_GTE_SWC2: {
        int writing = p.operation == BFM_GTE_SWC2;
        uint32_t offset = (word & 0x7fffu) - (word & 0x8000u);
        p.operand = (rs ? gpr[rs] : 0u) + offset;
        if (p.operand & 3u) {
            *request = p; /* Supplies BadVAddr to the existing exception path. */
            return writing ? BFM_GTE_STORE_ADDRESS_ERROR : BFM_GTE_LOAD_ADDRESS_ERROR;
        }
        if (!bus || !bus->admit) return BFM_GTE_DENIED;
        if (writing ? (!device->read_data || !bus->write) : (!device->write_data || !bus->read))
            return BFM_GTE_DENIED;
        if (!bus->admit(bus->context, p.operand, 4, writing)) return BFM_GTE_DENIED;
        p.bus = bus;
        p.bus_binding = *bus;
        break;
    }
    }
    *request = p;
    return BFM_GTE_OK;
}

/* Compare individual callback pointers; C struct padding is unspecified. */
static inline int bfm_gte_same_binding(const struct bfm_gte_request *p,
                                       const struct bfm_gte_request *q)
{
    const MusashiCpuTransferDevice *a = &p->device_binding;
    const MusashiCpuTransferDevice *b = &q->device_binding;
    if (p->cpu != q->cpu || p->word != q->word || p->operand != q->operand ||
        p->device != q->device || a->userdata != b->userdata || p->bus != q->bus)
        return 0;
    if (p->bus && (p->bus_binding.context != q->bus_binding.context ||
                   p->bus_binding.admit != q->bus_binding.admit)) return 0;
    switch (p->operation) {
    case BFM_GTE_COP2: return a->command == b->command;
    case BFM_GTE_MFC2: return a->read_data == b->read_data;
    case BFM_GTE_CFC2: return a->read_control == b->read_control;
    case BFM_GTE_MTC2: return a->write_data == b->write_data;
    case BFM_GTE_CTC2: return a->write_control == b->write_control;
    case BFM_GTE_LWC2:
        return a->write_data == b->write_data && p->bus_binding.read == q->bus_binding.read;
    case BFM_GTE_SWC2:
        return a->read_data == b->read_data && p->bus_binding.write == q->bus_binding.write;
    }
    return 0;
}

/* Call once per successful prepare after the outer CPU owner has revalidated
 * the live dispatch. A failure after issuance is terminal for this attempt:
 * the callback ABI does not promise rollback, including side-effecting reads.
 * On success, the caller retires old delayed loads and enqueues result->reg
 * on the existing GPR delay pipeline. No PC, clock or GPR is written here. */
static inline enum bfm_gte_status
bfm_gte_issue(const struct bfm_gte_request *expected, uint32_t word,
              const uint32_t gpr[32], const MusashiCpuContext *cpu,
              const MusashiCpuTransferDevice *device, const struct bfm_generic_bus *bus,
              struct bfm_gte_delayed_result *result)
{
    struct bfm_gte_request p;
    struct bfm_gte_delayed_result pending = {0};
    uint32_t value = 0;
    int ok = 0;
    if (!expected || !result) return BFM_GTE_DENIED;
    if (bfm_gte_prepare(word, gpr, cpu, device, bus, &p) != BFM_GTE_OK ||
        !bfm_gte_same_binding(expected, &p)) return BFM_GTE_STALE;
    switch (p.operation) {
    case BFM_GTE_COP2:
        ok = device->command(device->userdata, cpu, word);
        break;
    case BFM_GTE_MFC2:
    case BFM_GTE_CFC2:
        ok = p.operation == BFM_GTE_MFC2
            ? device->read_data(device->userdata, cpu, p.gte_reg, &value)
            : device->read_control(device->userdata, cpu, p.gte_reg, &value);
        if (p.cpu_reg) {
            pending.reg = p.cpu_reg;
            pending.value = value;
        }
        break;
    case BFM_GTE_MTC2:
        ok = device->write_data(device->userdata, cpu, p.gte_reg, p.operand);
        break;
    case BFM_GTE_CTC2:
        ok = device->write_control(device->userdata, cpu, p.gte_reg, p.operand);
        break;
    case BFM_GTE_LWC2:
        ok = bus->read(bus->context, p.operand, 4, &value) &&
             device->write_data(device->userdata, cpu, p.gte_reg, value);
        break;
    case BFM_GTE_SWC2:
        ok = device->read_data(device->userdata, cpu, p.gte_reg, &value) &&
             bus->write(bus->context, p.operand, 4, value, 15u);
        break;
    }
    if (!ok) return BFM_GTE_CALLBACK_FAILED;
    *result = pending;
    return BFM_GTE_OK;
}

#endif
