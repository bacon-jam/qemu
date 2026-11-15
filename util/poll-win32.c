/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Windows polling with a high-resolution deadline. */

#include "qemu/osdep.h"
#include "qemu/timer.h"

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

int qemu_poll_ns(GPollFD *fds, guint nfds, int64_t timeout)
{
    GPollFD pollfds[MAXIMUM_WAIT_OBJECTS];
    HANDLE timer;
    LARGE_INTEGER due;
    int ret;
    guint i;

    /* Leave room for both our deadline and GLib's message-queue handle. */
    if (timeout <= 0 || nfds >= MAXIMUM_WAIT_OBJECTS - 1) {
        return g_poll(fds, nfds, qemu_timeout_ns_to_ms(timeout));
    }

    timer = CreateWaitableTimerExW(NULL, NULL,
                                  CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                  TIMER_MODIFY_STATE | SYNCHRONIZE);
    if (!timer) {
        return g_poll(fds, nfds, qemu_timeout_ns_to_ms(timeout));
    }

    if (nfds) {
        memcpy(pollfds, fds, nfds * sizeof(*fds));
    }
    pollfds[nfds] = (GPollFD) {
        .fd = (gintptr)timer,
        .events = G_IO_IN,
    };

    /* Relative due times use negative 100 ns units, rounded away from zero. */
    due.QuadPart = -(timeout / 100 + (timeout % 100 != 0));
    if (!SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE)) {
        CloseHandle(timer);
        return g_poll(fds, nfds, qemu_timeout_ns_to_ms(timeout));
    }

    /* Keep GLib's handle/message dispatch and its old timeout as a watchdog. */
    ret = g_poll(pollfds, nfds + 1, qemu_timeout_ns_to_ms(timeout));
    for (i = 0; i < nfds; i++) {
        fds[i].revents = pollfds[i].revents;
    }
    if (ret > 0 && pollfds[nfds].revents) {
        ret--;
    }

    CancelWaitableTimer(timer);
    CloseHandle(timer);
    return ret;
}
