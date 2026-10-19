// The renderers' worker threads (workers.h): the 3D renderer's frame drawn in bands of lines at once
// (port/gfx/ref/ds3d.c), on the game's thread and up to 7 workers, which the program starts with --render-threads
// (port/app/main.c). A job's parts start when the caller raises the job's signal and end when the last worker's part
// raises the done signal; a waiting thread spins a little before it sleeps, since the parts of one frame end close
// together. The threads, their signals and the host's clock are port/hal/host's. The workers run renderer code only,
// on memory a job's parts share out between them, so the game stays on one thread and every frame is the same however
// many parts draw it.
#include "gfx/workers.h"
#include "hal/host/host.h"
#include <stdatomic.h>
#include <stdint.h>

// How long a thread spins before it sleeps: a worker waiting for the next job, which comes a frame later, a little;
// a part waiting for the others to finish or to reach a sync, which they do about when it does, longer
#define SPIN_FOR_JOB_NS 20000u
#define SPIN_FOR_PARTS_NS 500000u

static int sParts = 1;
static int sWorkers;
// The job signal's count when each worker was started: the jobs it is to run are those raised after
static unsigned sStartCount[PORT_WORKERS_MAX];
static PortWorkersJob sJob;
static void *sContext;
// The workers whose parts are still running, and the parts that have reached the sync
static atomic_int sPending;
static atomic_int sArrived;
// Raised for each job, when the last worker's part is done, and when the last part reaches a sync
static PortHostSignal *sJobs;
static PortHostSignal *sDone;
static PortHostSignal *sSynced;

// What a spinning thread does between two looks: tells the core it spins, so that it slows down and lets the other
// thread of its core run
static void Relax(void) {
#if defined(__aarch64__) || defined(__arm64__)
    __builtin_arm_yield();
#elif defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#endif
}

// Waits until signal's count differs from seen, spinning for up to spinNs first, and returns the count
static unsigned Await(PortHostSignal *signal, unsigned seen, uint64_t spinNs) {
    uint64_t start = port_host_now_ns();
    for (unsigned looks = 0;; looks++) {
        unsigned count = port_host_signal_count(signal);
        if (count != seen) {
            return count;
        }
        if ((looks & 63) == 63 && port_host_now_ns() - start > spinNs) {
            return port_host_signal_wait(signal, seen);
        }
        Relax();
    }
}

// A worker: its part of each job, then the next job. A worker whose part is past the job's parts (after a start with
// fewer parts than workers) sits the job out
static void Worker(void *argument) {
    int part = (int)(intptr_t)argument;
    unsigned seen = sStartCount[part];

    for (;;) {
        seen = Await(sJobs, seen, SPIN_FOR_JOB_NS);
        if (part >= sParts) {
            continue;
        }
        sJob(sContext, part, sParts);
        if (atomic_fetch_sub_explicit(&sPending, 1, memory_order_acq_rel) == 1) {
            port_host_signal_raise(sDone);
        }
    }
}

int port_workers_start(int parts) {
    if (parts <= 0) {
        parts = port_host_performance_cores();
    }
    parts = parts < 1 ? 1 : parts > PORT_WORKERS_MAX ? PORT_WORKERS_MAX : parts;
    if (parts > 1 && sJobs == NULL) {
        sJobs = port_host_signal_create();
        sDone = port_host_signal_create();
        sSynced = port_host_signal_create();
        if (sJobs == NULL || sDone == NULL || sSynced == NULL) {
            sJobs = NULL;
            parts = 1;
        }
    }
    while (sWorkers < parts - 1) {
        sStartCount[sWorkers + 1] = port_host_signal_count(sJobs);
        if (port_host_thread_start(Worker, (void *)(intptr_t)(sWorkers + 1)) != 0) {
            parts = sWorkers + 1;
            break;
        }
        sWorkers++;
    }
    sParts = parts;
    return parts;
}

int port_workers_parts(void) {
    return sParts;
}

void port_workers_run(PortWorkersJob job, void *context) {
    unsigned done;

    if (sParts <= 1) {
        job(context, 0, 1);
        return;
    }
    // The job's fields before the raise, which publishes them to the workers that see its count
    done = port_host_signal_count(sDone);
    sJob = job;
    sContext = context;
    atomic_store_explicit(&sPending, sParts - 1, memory_order_relaxed);
    atomic_store_explicit(&sArrived, 0, memory_order_relaxed);
    port_host_signal_raise(sJobs);
    job(context, 0, sParts);
    // The last worker raises the done signal once its part and every other worker's are done, so what they wrote is
    // seen here
    Await(sDone, done, SPIN_FOR_PARTS_NS);
}

void port_workers_sync(void) {
    unsigned synced;

    if (sParts <= 1) {
        return;
    }
    synced = port_host_signal_count(sSynced);
    if (atomic_fetch_add_explicit(&sArrived, 1, memory_order_acq_rel) + 1 == sParts) {
        atomic_store_explicit(&sArrived, 0, memory_order_relaxed);
        port_host_signal_raise(sSynced);
        return;
    }
    Await(sSynced, synced, SPIN_FOR_PARTS_NS);
}
