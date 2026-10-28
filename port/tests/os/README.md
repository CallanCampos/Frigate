# Actual SDK scheduler tests

[test_os.c](test_os.c) links the actual SDK thread/scheduler/message/mutex code and native
[OS core](../../hal/os/README.md), running scenarios through `port_os_run`. The existing parent
[test map](../README.md) covers the other OS scenarios. There are no scheduler substitutes or ROM data here.

| Scenario | Absolute-device-wait contract exercised |
| --- | --- |
| `os_device_deadline` | Exact/past deadlines never rewind; real timer IRQ CPU work does not extend the deadline. |
| `os_device_preempted` | SDK IRQ wakeup switches to a higher-priority thread; short work spends the wait, long work passes its deadline and the waiter resumes immediately. |
| `os_device_masked` | Timed host work progresses with I, IME and IE separately masked; IF latches, handlers wait for unmasking. |
| `os_device_irq_wait` | Actual host-stack IRQ handler waits while device work completes; callbacks cannot recursively wait, secondary IRQs remain latched, and an already-past wait drains queued work without rewinding. Existing IRQ Spin still defers host work. |
| `os_device_lifecycle` | Outside-run rejection; stop during thread and IRQ waits; IRQ unwinds; next `port_os_run` resets clock, guard and pending work. |

The same cases run with normal and fatal ASan/UBSan options; existing small-stack/fake-stack aggregate tests also
include them. Regression probes `os_busy_wait_preempted`, `os_timed_work` and `os_determinism` retain the old own-work
and pinned event-order expectations. These tests establish native API behavior, not DS hardware timing parity.
