// The renderers' worker threads: a job split into parts, part 0 run by the caller (the game's thread) and the others
// at once by the workers, the call returning when every part is done. The 3D renderer draws its frame this way, a band
// of lines a part (port/gfx/ref/ds3d.c), which makes the same picture whatever the number of parts. Nothing but
// renderer code runs on the workers: the game, the platform layer and the frame hooks stay on the game's one thread
// (port/docs/ARCHITECTURE.md, "Threads and stacks"). Plain C types only.
#ifndef PORT_GFX_WORKERS_H
#define PORT_GFX_WORKERS_H

// The most parts a job runs in: the caller and 7 workers
#define PORT_WORKERS_MAX 8

// Starts the workers so that jobs run in parts parts, the caller's included: 1 runs every job on the caller alone, 0
// as many as the host has performance cores (port/hal/host's port_host_performance_cores), at most PORT_WORKERS_MAX.
// The program calls it once, with --render-threads (port/app/main.c); before that, every job runs in one part. Returns
// the parts a job will run in, fewer than asked when the host can't start a thread
int port_workers_start(int parts);
// The parts a job runs in
int port_workers_parts(void);

// Runs job(context, part, parts) for every part, part 0 on the caller, and returns when all are done. Jobs don't nest
typedef void (*PortWorkersJob)(void *context, int part, int parts);
void port_workers_run(PortWorkersJob job, void *context);

// Inside a job: returns once every part of it has reached this call, so that what each part wrote before it is seen by
// all after it. Every part calls it the same number of times
void port_workers_sync(void);

#endif // PORT_GFX_WORKERS_H
