/* SPDX-License-Identifier: MIT */
#ifndef BFM_MIPS_MOUNTED_CONTROL_H
#define BFM_MIPS_MOUNTED_CONTROL_H

#include <stdint.h>
#include <stddef.h>

typedef struct BfmControlSite {
    uint32_t pc, next_pc, branch_pc;
    uint32_t gpr[32];
    uint32_t load_reg, load_value;
    unsigned delay_slot;
} BfmControlSite;

enum BfmControlForm {
    BFM_J_IMMEDIATE,
    BFM_J_REGISTER,
    BFM_BRANCH_RELATIVE
};

typedef struct BfmControlEdge {
    uint32_t origin, target, continuation;
    unsigned form, taken, link_reg, source_reg;
} BfmControlEdge;

/* The issuer owns link writes, prior-load retirement, timing and the delay slot.
 * target is the resolved PC after the slot, including for an untaken branch.
 * link_reg is the architectural destination, or zero when there is no write.
 * source_reg identifies the target register only for BFM_J_REGISTER. */
int bfm_control_decode(const BfmControlSite *site, uint32_t word,
                       BfmControlEdge *edge);


typedef struct BfmControlOwner {
    const void *cpu, *dispatch, *mapping;
    uint64_t mapping_generation;
    void *clock_context;
    int (*advance)(void *, uint32_t);
} BfmControlOwner;

typedef struct BfmControlWord {
    const void *image;
    uint64_t generation;
    uint32_t value;
} BfmControlWord;

enum BfmControlRoute { BFM_CONTROL_LOCAL = 1, BFM_CONTROL_CALL, BFM_CONTROL_RETURN };

typedef struct BfmControlGrant {
    enum BfmControlRoute route;
    const void *recipient;
} BfmControlGrant;

typedef struct BfmControlProof {
    BfmControlEdge edge;
    uint32_t compare_lhs, compare_rhs;
    BfmControlWord instruction, slot, destination, resume;
    BfmControlGrant grant;
} BfmControlProof;

typedef struct BfmControlFrame {
    BfmControlOwner owner;
    BfmControlProof call;
} BfmControlFrame;

/* The host owns this view, the protected frame storage and its revision.
 * Advance the revision on every stack change, including in-place replacement.
 * remaining counts instructions available to the issuer, before retirement. */
typedef struct BfmControlView {
    BfmControlOwner owner;
    BfmControlSite site;
    const void *stack_id;
    uint64_t stack_revision;
    size_t depth, capacity;
    const BfmControlFrame *top;
    uint32_t remaining;
} BfmControlView;

typedef struct BfmControlAuthority {
    void *context;
    int (*observe)(void *, BfmControlView *);
    /* Read one aligned, complete word through the active executable mapping. */
    int (*read_executable)(void *, uint32_t, BfmControlWord *);
    /* Verify source/target ownership for the named edge on this dispatch. */
    int (*authorize)(void *, const BfmControlEdge *, BfmControlGrant *);
    /* Confirm normal execution supports this non-control delay instruction. */
    int (*allows_slot)(void *, uint32_t, uint32_t);
} BfmControlAuthority;

typedef struct BfmControlTicket {
    BfmControlOwner owner;
    BfmControlSite issue;
    BfmControlProof proof;
    BfmControlFrame parent;
    const void *stack_id;
    uint64_t stack_revision;
    size_t depth, capacity;
    uint32_t remaining;
    unsigned stage;
    BfmControlAuthority authority;
} BfmControlTicket;

/* A ticket belongs to a single in-flight transfer. The trusted owner executes
 * and charges both instructions normally. These checks never call the clock,
 * write a register, or change the owner's frame stack. Call in order:
 *
 *   prepare; charge branch; after_clock; issue branch;
 *   at_slot; charge delay; after_clock; execute delay; arrive.
 *
 * remaining stays unchanged across charge and drops once at each retirement.
 * A refused clock stops execution at its ordinary boundary. Discard the
 * ticket on abort or reset. Reloaded frames must retain owner provenance.
 * After arrival the owner pushes the returned frame on CALL, pops on RETURN,
 * and leaves the stack alone on LOCAL. Single-owner, synchronous API. */
int bfm_control_prepare(const BfmControlAuthority *authority, BfmControlTicket *ticket);
int bfm_control_after_clock(const BfmControlAuthority *authority, BfmControlTicket *ticket);
int bfm_control_at_slot(const BfmControlAuthority *authority, BfmControlTicket *ticket);
/* After exhausting the last step on the branch, at_slot returns zero and keeps
 * the owner-validated pending slot. Only an explicit new grant from the
 * enclosing scheduler may resume this slot. An ordinary refusal is final. */
int bfm_control_resume_slot(const BfmControlAuthority *authority, BfmControlTicket *ticket);
int bfm_control_arrive(const BfmControlAuthority *authority, BfmControlTicket *ticket,
                       BfmControlGrant *grant, BfmControlFrame *call_frame);

#endif
