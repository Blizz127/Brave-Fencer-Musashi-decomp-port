/* SPDX-License-Identifier: MIT */
#include "mips_mounted_control.h"

#include <string.h>

static uint32_t relative_target(uint32_t origin, uint32_t word)
{
    uint32_t offset = word & UINT32_C(0xffff);
    if (offset & UINT32_C(0x8000))
        offset |= UINT32_C(0xffff0000);
    return origin + UINT32_C(4) + (offset << 2);
}

int bfm_control_decode(const BfmControlSite *site, uint32_t word,
                       BfmControlEdge *edge)
{
    if (!site || !edge || site->delay_slot || site->gpr[0] || site->load_reg >= 32 ||
        (site->pc & 3) || site->next_pc != site->pc + UINT32_C(4))
        return 0;

    unsigned opcode = word >> 26;
    unsigned rs = (word >> 21) & 31;
    unsigned rt = (word >> 16) & 31;
    unsigned rd = (word >> 11) & 31;
    BfmControlEdge transfer = {0};
    transfer.origin = site->pc;
    transfer.continuation = site->next_pc + UINT32_C(4);
    transfer.taken = 1;

    if (opcode == 0) {
        unsigned function = word & 63;
        if ((function != 8 && function != 9) || rt || ((word >> 6) & 31) ||
            (function == 8 && rd))
            return 0;
        transfer.form = BFM_J_REGISTER;
        transfer.source_reg = rs;
        transfer.target = site->gpr[rs];
        transfer.link_reg = rd;
    } else if (opcode == 2 || opcode == 3) {
        transfer.form = BFM_J_IMMEDIATE;
        transfer.target = (site->next_pc & UINT32_C(0xf0000000)) |
                          ((word & UINT32_C(0x03ffffff)) << 2);
        transfer.link_reg = opcode == 3 ? 31 : 0;
    } else {
        uint32_t value = site->gpr[rs];
        unsigned negative = value >> 31;
        transfer.form = BFM_BRANCH_RELATIVE;
        switch (opcode) {
        case 1:
            if (rt != 0 && rt != 1 && rt != 16 && rt != 17)
                return 0;
            transfer.taken = (rt & 1) ? !negative : negative;
            transfer.link_reg = (rt & 16) ? 31 : 0;
            break;
        case 4:
            transfer.taken = value == site->gpr[rt];
            break;
        case 5:
            transfer.taken = value != site->gpr[rt];
            break;
        case 6:
        case 7:
            if (rt)
                return 0;
            transfer.taken = opcode == 6 ? (negative || !value)
                                          : (!negative && value);
            break;
        default:
            return 0;
        }
        transfer.target = transfer.taken ? relative_target(site->pc, word)
                                         : transfer.continuation;
    }
    if (transfer.target & 3)
        return 0;
    *edge = transfer;
    return 1;
}

enum TicketStage { NO_TICKET, ISSUE_READY, ISSUE_CHECKED, SLOT_READY, SLOT_CHECKED, SLOT_PARKED };

static int same_edge(const BfmControlEdge *a, const BfmControlEdge *b)
{
    return a->origin == b->origin && a->target == b->target &&
           a->continuation == b->continuation && a->form == b->form &&
           a->taken == b->taken && a->link_reg == b->link_reg &&
           a->source_reg == b->source_reg;
}

static int same_site(const BfmControlSite *a, const BfmControlSite *b)
{
    return a->pc == b->pc && a->next_pc == b->next_pc &&
           a->branch_pc == b->branch_pc && a->delay_slot == b->delay_slot &&
           a->load_reg == b->load_reg && a->load_value == b->load_value &&
           memcmp(a->gpr, b->gpr, sizeof(a->gpr)) == 0;
}

static int same_owner(const BfmControlOwner *a, const BfmControlOwner *b)
{
    return a->cpu == b->cpu && a->dispatch == b->dispatch &&
           a->mapping == b->mapping && a->mapping_generation == b->mapping_generation &&
           a->clock_context == b->clock_context && a->advance == b->advance;
}

static int same_word(const BfmControlWord *a, const BfmControlWord *b)
{
    return a->image == b->image && a->generation == b->generation && a->value == b->value;
}

static int same_proof(const BfmControlProof *a, const BfmControlProof *b)
{
    return same_edge(&a->edge, &b->edge) && a->compare_lhs == b->compare_lhs &&
           a->compare_rhs == b->compare_rhs && same_word(&a->instruction, &b->instruction) &&
           same_word(&a->slot, &b->slot) && same_word(&a->destination, &b->destination) &&
           same_word(&a->resume, &b->resume) && a->grant.route == b->grant.route &&
           a->grant.recipient == b->grant.recipient;
}

static int mounted_word(const BfmControlAuthority *a, uint32_t address, BfmControlWord *word)
{
    return !(address & 3) && a->read_executable(a->context, address, word) && word->image;
}

static int control_slot(uint32_t word)
{
    unsigned opcode = word >> 26;
    unsigned function = word & 63;
    if (!opcode)
        return function == 8 || function == 9;
    return opcode <= 7 || (opcode >= 16 && opcode <= 19 && ((word >> 21) & 31) == 8) ||
           (opcode >= 20 && opcode <= 23);
}

static int inspect_edge(const BfmControlAuthority *a, const BfmControlEdge *edge,
                        uint32_t compare_lhs, uint32_t compare_rhs, BfmControlProof *proof)
{
    BfmControlProof p = {0};
    p.edge = *edge;
    if (!mounted_word(a, edge->origin, &p.instruction) ||
        !mounted_word(a, edge->origin + UINT32_C(4), &p.slot) ||
        !mounted_word(a, edge->target, &p.destination) ||
        p.slot.image != p.instruction.image || p.slot.generation != p.instruction.generation ||
        control_slot(p.slot.value) ||
        !a->allows_slot(a->context, edge->origin + UINT32_C(4), p.slot.value) ||
        !a->authorize(a->context, edge, &p.grant) || !p.grant.recipient)
        return 0;

    /* Reconstruct the original read operands without consulting live registers
     * after the transfer. The slot or callee may have overwritten them. */
    BfmControlSite source = {.pc = edge->origin, .next_pc = edge->origin + UINT32_C(4)};
    uint32_t word = p.instruction.value;
    if (edge->form != BFM_J_IMMEDIATE)
        source.gpr[(word >> 21) & 31] = compare_lhs;
    unsigned opcode = word >> 26;
    if (opcode == 4 || opcode == 5)
        source.gpr[(word >> 16) & 31] = compare_rhs;
    BfmControlEdge decoded;
    if (!bfm_control_decode(&source, word, &decoded) || !same_edge(edge, &decoded))
        return 0;
    p.compare_lhs = compare_lhs;
    p.compare_rhs = compare_rhs;

    if (edge->taken && edge->link_reg) {
        if (p.grant.route != BFM_CONTROL_CALL ||
            !mounted_word(a, edge->continuation, &p.resume) ||
            p.resume.image != p.instruction.image ||
            p.resume.generation != p.instruction.generation)
            return 0;
    } else if (p.grant.route == BFM_CONTROL_RETURN) {
        if (edge->form != BFM_J_REGISTER || edge->link_reg)
            return 0;
    } else if (p.grant.route != BFM_CONTROL_LOCAL ||
               (edge->form == BFM_J_REGISTER && edge->source_reg == 31)) {
        return 0;
    }
    *proof = p;
    return 1;
}

static int get_view(const BfmControlAuthority *a, BfmControlView *v)
{
    *v = (BfmControlView){0};
    return a && a->observe && a->read_executable && a->authorize && a->allows_slot &&
           a->observe(a->context, v) && v->owner.cpu && v->owner.dispatch &&
           v->owner.mapping && v->owner.advance && v->stack_id &&
           v->depth <= v->capacity && (!!v->top == !!v->depth) &&
           !v->site.gpr[0] && v->site.load_reg < 32;
}

static int current_route(const BfmControlAuthority *a, const BfmControlView *v,
                         const BfmControlProof *p)
{
    if (v->depth && !same_owner(&v->owner, &v->top->owner))
        return 0;
    if (p->grant.route == BFM_CONTROL_CALL)
        return v->depth < v->capacity;

    const BfmControlEdge *edge = &p->edge;
    if (p->grant.route == BFM_CONTROL_LOCAL) {
        /* A matched return through a non-ABI link register also needs its frame. */
        return !v->depth || edge->form != BFM_J_REGISTER ||
               edge->source_reg != v->top->call.edge.link_reg ||
               edge->target != v->top->call.edge.continuation;
    }
    if (!v->depth)
        return 0;

    const BfmControlProof *call = &v->top->call;
    BfmControlProof live_call;
    return call->grant.route == BFM_CONTROL_CALL && call->edge.taken &&
           call->edge.link_reg && edge->source_reg == call->edge.link_reg &&
           edge->target == call->edge.continuation &&
           same_word(&p->destination, &call->resume) &&
           inspect_edge(a, &call->edge, call->compare_lhs, call->compare_rhs, &live_call) &&
           same_proof(call, &live_call);
}

int bfm_control_prepare(const BfmControlAuthority *a, BfmControlTicket *ticket)
{
    BfmControlView v;
    BfmControlWord word;
    BfmControlEdge edge;
    BfmControlProof proof;
    if (!ticket)
        return 0;
    ticket->stage = NO_TICKET;
    if (!get_view(a, &v) || !v.remaining || !mounted_word(a, v.site.pc, &word) ||
        !bfm_control_decode(&v.site, word.value, &edge))
        return 0;
    uint32_t lhs = edge.form == BFM_J_IMMEDIATE ? 0 : v.site.gpr[(word.value >> 21) & 31];
    unsigned opcode = word.value >> 26;
    uint32_t rhs = opcode == 4 || opcode == 5 ? v.site.gpr[(word.value >> 16) & 31] : 0;
    if (!inspect_edge(a, &edge, lhs, rhs, &proof) ||
        !same_word(&word, &proof.instruction) || !current_route(a, &v, &proof))
        return 0;
    *ticket = (BfmControlTicket){
        .owner = v.owner, .issue = v.site, .proof = proof, .stack_id = v.stack_id,
        .stack_revision = v.stack_revision, .depth = v.depth, .capacity = v.capacity,
        .remaining = v.remaining, .stage = ISSUE_READY, .authority = *a
    };
    if (v.depth)
        ticket->parent = *v.top;
    return 1;
}

static int read_current(const BfmControlAuthority *a, const BfmControlTicket *t,
                        BfmControlView *v)
{
    BfmControlProof p;
    if (!a || a->context != t->authority.context || a->observe != t->authority.observe ||
        a->read_executable != t->authority.read_executable || a->authorize != t->authority.authorize ||
        a->allows_slot != t->authority.allows_slot || !get_view(a, v) ||
        !same_owner(&t->owner, &v->owner) || t->stack_id != v->stack_id ||
        t->stack_revision != v->stack_revision || t->depth != v->depth || t->capacity != v->capacity)
        return 0;
    if (v->depth && (!same_owner(&v->top->owner, &t->parent.owner) ||
                     !same_proof(&v->top->call, &t->parent.call)))
        return 0;
    return inspect_edge(a, &t->proof.edge, t->proof.compare_lhs, t->proof.compare_rhs, &p) &&
           same_proof(&t->proof, &p) &&
           current_route(a, v, &p);
}

int bfm_control_after_clock(const BfmControlAuthority *a, BfmControlTicket *t)
{
    if (!t)
        return 0;
    unsigned stage = t->stage;
    t->stage = NO_TICKET;
    BfmControlView v;
    if ((stage != ISSUE_READY && stage != SLOT_READY) || !read_current(a, t, &v) ||
        !same_site(&v.site, &t->issue) || v.remaining != t->remaining)
        return 0;
    t->stage = stage == ISSUE_READY ? ISSUE_CHECKED : SLOT_CHECKED;
    return 1;
}

int bfm_control_at_slot(const BfmControlAuthority *a, BfmControlTicket *t)
{
    if (!t)
        return 0;
    unsigned stage = t->stage;
    t->stage = NO_TICKET;
    BfmControlView v;
    const BfmControlEdge *edge = &t->proof.edge;
    if (stage != ISSUE_CHECKED || !read_current(a, t, &v) ||
        v.remaining != t->remaining - 1 || v.site.delay_slot != 1 ||
        v.site.branch_pc != edge->origin || v.site.pc != edge->origin + UINT32_C(4) ||
        v.site.next_pc != edge->target ||
        (edge->link_reg && v.site.gpr[edge->link_reg] != edge->continuation))
        return 0;
    t->issue = v.site;
    t->remaining = v.remaining;
    t->stage = v.remaining ? SLOT_READY : SLOT_PARKED;
    return v.remaining != 0;
}

int bfm_control_resume_slot(const BfmControlAuthority *a, BfmControlTicket *t)
{
    if (!t || t->stage != SLOT_PARKED)
        return 0;
    BfmControlView v;
    if (!read_current(a, t, &v) || !same_site(&t->issue, &v.site)) {
        t->stage = NO_TICKET;
        return 0;
    }
    if (!v.remaining)
        return 0;
    t->remaining = v.remaining;
    t->stage = SLOT_READY;
    return 1;
}

int bfm_control_arrive(const BfmControlAuthority *a, BfmControlTicket *t,
                       BfmControlGrant *grant, BfmControlFrame *call_frame)
{
    if (!t)
        return 0;
    unsigned stage = t->stage;
    t->stage = NO_TICKET;
    BfmControlView v;
    if (stage != SLOT_CHECKED || !grant || !read_current(a, t, &v) ||
        v.remaining != t->remaining - 1 || v.site.delay_slot ||
        v.site.pc != t->proof.edge.target || v.site.next_pc != v.site.pc + UINT32_C(4))
        return 0;
    if (t->proof.grant.route == BFM_CONTROL_CALL) {
        if (!call_frame)
            return 0;
        *call_frame = (BfmControlFrame){ .owner = t->owner, .call = t->proof };
    }
    *grant = t->proof.grant;
    return 1;
}
