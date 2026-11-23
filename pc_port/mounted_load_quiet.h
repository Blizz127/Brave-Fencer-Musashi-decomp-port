/* The next checkpoint flushes every outstanding guest cycle. Include that
 * debt in the requested interval and exclude events even on its last tick.
 * Only the guest clock fixes the absolute time of every cut in advance. */
static int mounted_guest_quiet_horizon(NativeBoot *boot, uint32_t cycles, uint64_t *end_cycle) {
    const MusashiDeviceEpoch *epoch = &boot->epoch;
    const MusashiPsyCrossIrqScheduler *irq = &boot->irq.scheduler;
    const uint32_t dma_busy = 0x01000000u;
    const uint64_t span = (uint64_t)boot->guest_pending_cycles + cycles;
    if (!boot->guest_clock || host_stop_signal || boot->host_service_depth ||
        boot->bios_callback_frame.busy || boot->cd_event_busy || boot->irq.cd_frame.busy ||
        !irq->installed || !irq->host_edges_advisory || irq->pumping || irq->delivering ||
        irq->collecting || irq->faulted || irq->active_continuation ||
        irq->device != &boot->irq.device || irq->cpu_status != boot->irq.cpu_status ||
        (boot->irq.controller.mask & ~0x000du) || musashi_irq_controller_pending(&boot->irq.controller))
        return 0;
    if (!epoch->active || epoch->busy || epoch->faulted || boot->source_clock.faulted ||
        epoch->owner != epoch_owner(boot) || epoch->owner != irq->owner_thread ||
        epoch->adapter.userdata != boot || epoch->adapter.owner_thread != epoch_owner ||
        epoch->adapter.collect_source != epoch_collect || epoch->adapter.wait_until_counter != epoch_wait ||
        epoch->adapter.advance_devices != epoch_advance_devices || epoch->adapter.vblank_edge != epoch_video_edge ||
        boot->execution_clock.advance != advance_source_clock || boot->execution_clock.userdata != boot ||
        boot->callback.userdata != boot || boot->callback.checkpoint != checkpoint ||
        epoch->frequency != MUSASHI_DEVICE_CLOCK_HZ || epoch->last_cut_tick != boot->guest_tick ||
        boot->guest_tick < epoch->origin_tick || epoch->cycle != boot->guest_tick - epoch->origin_tick ||
        epoch->pending_next != epoch->pending.count || boot->guest_pending_cycles >= MUSASHI_GUEST_CLOCK_QUANTUM ||
        span > UINT32_MAX || boot->guest_tick > UINT64_MAX - span)
        return 0;
    const uint64_t cut_tick = boot->guest_tick + span;
    const uint64_t cut_cycle = cut_tick - epoch->origin_tick;
    /* A video edge also pulses the card and polls the host. Keep it outside
     * this interval even if the VBlank bit is masked. Host source edges are
     * drained as advisory metadata and cannot advance this guest clock. */
    if (boot->guest_next_vblank <= cut_tick ||
        (boot->sio.ack_pending && boot->sio.ack_due <= cut_cycle))
        return 0;
    /* The device epoch advances DMA0/1, DMA2 and DMA3. Refuse transfers,
     * including an MDEC output completion that can release its input DMA. */
    MusashiCdDma3State dma;
    if (!boot->cd_drive || !boot->cd_dma || !boot->gpu_dma ||
        boot->dma.gpu_owner != boot->gpu_dma || boot->dma.irq != &boot->irq.controller ||
        boot->mdec.dma != &boot->dma || boot->mdec.ram != &boot->memory ||
        ((boot->mdec.chcr[0] | boot->mdec.chcr[1] | boot->dma.gpu_channel_control) & dma_busy) ||
        boot->mdec.output_event_pending || boot->mdec.input_waits_for_output ||
        !musashi_cd_dma3_get_state(boot->cd_dma, &dma) || dma.fault || (dma.chcr & dma_busy))
        return 0;
    /* The drive can lag the epoch after a bounded cut. Compare each actual
     * event deadline to the final absolute cycle, including overdue events. */
    MusashiCdOwnedState cd;
    if (!musashi_cd_owned_get_state(boot->cd_drive, &cd) || cd.fault ||
        cd.cycle > cut_cycle || (cd.phase && cd.due <= cut_cycle) ||
        (cd.reading && cd.sector_due <= cut_cycle) ||
        (cd.incoming_valid && cd.sector_publish_due <= cut_cycle)) return 0;
    if (cycles && (cd.cycle > epoch->cycle || dma.cycle != epoch->cycle ||
                   dma.waiting_for_request)) return 0;
    *end_cycle = cut_cycle;
    return 1;
}

static int mounted_load_guest_quiet(NativeBoot *boot) {
    uint64_t end;
    return mounted_guest_quiet_horizon(boot, 0, &end);
}

static int mounted_charge_quiet(void *userdata, const void *continuation, uint32_t cycles) {
    NativeBoot *boot = userdata;
    uint64_t end;
    if (!boot || !cycles || !continuation || boot->epoch.owner != epoch_owner(boot) ||
        boot->irq.scheduler.owner_thread != boot->epoch.owner || boot->epoch.busy ||
        boot->host_service_depth || boot->bios_callback_frame.busy || boot->cd_event_busy ||
        boot->irq.cd_frame.busy || boot->continuation != continuation ||
        boot->startup_devices.callback != &boot->callback ||
        boot->startup_devices.clock != &boot->execution_clock ||
        boot->callback.charge_quiet != mounted_charge_quiet ||
        boot->bios_callback_frame.faulted || boot->irq.cd_frame.faulted ||
        boot->cd_irq_production_limit || boot->cd_timing_active ||
        boot->source_clock.cycles > UINT64_MAX - cycles ||
        !musashi_psycross_irq_runtime_quiet_owner(&boot->irq, &boot->memory) ||
        !mounted_guest_quiet_horizon(boot, cycles, &end)) return 0;

    const MusashiDeviceEpoch *epoch = &boot->epoch;
    /* A positive cost pays at least one cycle per original operation. Even
     * one flush, wait, and separate zero-cost sync per operation must fit the
     * old counters; batching cannot rescue an otherwise exhausted epoch. */
    const uint64_t max_waits = (uint64_t)cycles + 1;
    uint64_t frequency, due;
    if (!guest_clock_pacing_due(boot, epoch->origin_tick + end, &frequency, &due) ||
        epoch->cuts > UINT64_MAX - 3 * max_waits || epoch->waits > UINT64_MAX - max_waits ||
        !boot->sio.pad_bound || !boot->sio.cycle_initialized || boot->sio.faulted ||
        boot->sio.pad_device.userdata != boot || boot->sio.pad_device.owner_thread != epoch_owner ||
        boot->sio.owner_token != epoch->owner || boot->sio.cycle != epoch->cycle ||
        boot->mdec.cycle != epoch->cycle || boot->timer1.last_cycles != epoch->cycle ||
        boot->timer2.last_cycles != epoch->cycle || boot->gpu.executing || boot->gpu.faulted ||
        (boot->spu_dma_chcr & 0x01000000u)) return 0;

    MusashiGpuDma2State gpu;
    MusashiDmaOtcState otc;
    MusashiSpuCdAudioSnapshot audio;
    if (!musashi_gpu_dma2_get_state(boot->gpu_dma, &gpu) || gpu.fault ||
        gpu.cycle != epoch->cycle || gpu.transfers != boot->gpu_dma_transfers_logged ||
        !musashi_dma_otc_get_state(boot->dma_otc, &otc) || otc.fault || (otc.chcr & 0x01000000u) ||
        !musashi_spu_cd_audio_snapshot(boot->spu, &audio) || audio.faulted ||
        audio.cycle != epoch->cycle ||
        audio.sample_index != end / MUSASHI_SPU_CD_AUDIO_SAMPLE_CYCLES) return 0;
    /* native_boot starts the mounted SPU at cycle zero. Excluding the next
     * sample also keeps CD queue backpressure constant at every omitted cut.
     * Timer1/2 advance only their own counters and sticky flags; validate the
     * actual timer modes on copies before committing to the real devices. */
    MusashiScanlineTimer timer1 = boot->timer1;
    MusashiTimer2 timer2 = boot->timer2;
    if (!musashi_scanline_timer_advance(&timer1, end) || !musashi_timer2_advance(&timer2, end) ||
        host_stop_signal) return 0;

    /* The ordinary charge may leave a remainder for the next checkpoint, or
     * soft-complete on a host stop. Here both ledgers must settle before the
     * caller publishes CPU/RAM. The epoch still collects at its starting and
     * ending cuts and uses its ordinary wait/device adapters. */
    if (!musashi_source_clock_advance(&boot->source_clock, cycles)) return -1;
    boot->guest_pending_cycles += cycles;
    if (!guest_clock_flush(boot)) return -1;
    if (host_stop_signal || epoch->cycle != end ||
        !musashi_psycross_irq_runtime_quiet_owner(&boot->irq, &boot->memory)) {
        boot->source_clock.faulted = 1;
        return -1;
    }
    return 1;
}
