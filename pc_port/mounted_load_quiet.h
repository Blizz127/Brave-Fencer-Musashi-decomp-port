/* Called on the foreground owner immediately after charging an ordinary RAM
 * load. Its next instruction starts with epoch_sync_flushed(), which pays
 * ALL outstanding guest cycles and advances devices again at the same cut.
 * A host-paced checkpoint can observe a later wall clock. */
static int mounted_load_guest_quiet(NativeBoot *boot) {
    const MusashiDeviceEpoch *epoch = &boot->epoch;
    const MusashiPsyCrossIrqScheduler *irq = &boot->irq.scheduler;
    const uint32_t dma_busy = 0x01000000u;
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
        boot->guest_tick > UINT64_MAX - boot->guest_pending_cycles)
        return 0;
    const uint64_t cut_tick = boot->guest_tick + boot->guest_pending_cycles;
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
    return musashi_cd_owned_get_state(boot->cd_drive, &cd) && !cd.fault &&
        cd.cycle <= cut_cycle && (!cd.phase || cd.due > cut_cycle) &&
        (!cd.reading || cd.sector_due > cut_cycle) &&
        (!cd.incoming_valid || cd.sector_publish_due > cut_cycle);
}
