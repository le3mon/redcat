void kInitializeScheduler(void) {
    kInitializeTCBPool();
    kInitializeList(&(gs_stScheduler.stReadyList));

    gs_stScheduler.pstRunningTask = kAllocateTCB();
}