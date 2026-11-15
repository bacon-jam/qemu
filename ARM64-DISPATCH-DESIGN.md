# Generated ARM64 indirect dispatch experiment

Status: reviewed design, not implemented or benchmarked. Base: `a30458692016e0984a556da4976580ce43cec5fb` (the selected seven-patch prototype).

## Proposed mechanism

Generate an exact-key probe of QEMU's existing per-CPU TB jump cache for ordinary AArch64 `BR x16/x17` on native x86-64 system emulation. On a hit, use the existing `goto_ptr` TB-exit operation. On a miss or unsupported mode, execute the existing lookup helper. Cache entries, writers, hash, invalidation and reclamation stay as implemented today.

This tests whether removing a common C helper boundary is profitable after paying all runtime guards. It does not assume a speedup. Existing direct chaining and helper-mediated indirect chaining are already the baseline.

## Bounded source scope

| File | Change |
| --- | --- |
| `target/arm/tcg/translate-a64.c` | Restricted BR emission, current architectural key and fallback. |
| `accel/tcg/translator.c` | Read-only generated cache probe and generic runtime guards. |
| `include/exec/translator.h` | Narrow probe declaration and preconditions. |
| `tcg/tcg-op.c` | Expose an opcode-only wrapper for the existing goto_ptr emission. |
| `include/tcg/tcg-op-common.h` | Declare that wrapper. |

No new cache, CPU state field, epoch, runtime option, register ABI or code-region lifetime mechanism is proposed.

## Correctness requirements

- Preserve existing BTYPE handling, then compute the post-TBI target using the live TCG PC. A raw backing-field PC load can be stale before synchronization.
- Reconstruct current ARM hflags, flags2 and BTYPE. Compare every destination key field and the cache entry's virtual PC. CF_PCREL permits TB sharing across aliases, so the TB's stored PC is not a substitute for the entry PC.
- Fall back for stepping, one-instruction mode, breakpoints, logging, plugin activity, unsupported cflags and unsupported architectural state. Observe current runtime eligibility, not only the source TB's creation flags.
- Use the actual bitmap element type. Windows x64 uses 32-bit unsigned long, so the plugin bitmap cannot be assumed to contain 64-bit words.
- Keep `plugin_gen_disable_mem_helpers()` before the original lookup helper. Moving it only into a later jump wrapper changes page-walk/MMIO instrumentation semantics. The proposed wrapper emits only the existing opcode; each caller retains its existing boundary setup.
- Preserve BB_EXIT/BB_END synchronization and destination entry stop checks. A hit must not enter past the destination prologue.
- Use aligned native-width pointer/cflags reads and the existing one-owner publication/remote-clear protocol. Preserve CF_INVALID checks and existing exclusive code-reuse quiescence. No lock or extra pointer reread is claimed to eliminate overlapping invalidation.
- Keep other host backends, TCI, user emulation, authenticated branches, BLR/RET and other BR registers on their existing path initially.

Two independent Astra design reviewers checked memory/lifetime and synchronization behavior. Both required the plugin-order and Windows bitmap corrections above. Revised design review found no architectural blocker for a bounded prototype. Source and runtime validation remain required.

## First experiment and rejection gates

1. Review the five-file implementation against existing key, eligibility, publication and exit code. Inspect emitted pointer loads and fallback order.
2. Reuse generic BTI, remap/alias/SMC/MMU and SMP tests. Add warm-then-debug mode changes, fetch faults and forced code-region reuse. Test misses and excluded states explicitly.
3. Compare lookup-only loops and direct controls, neutral/adverse collision layouts, multiple code footprints and useful integer work per handler.
4. Measure a deterministic handler workload with independently checked output. Include cold translation and invalidation/context-switch costs inside separately declared intervals.
5. Reject if guards consume the helper savings, direct controls regress reproducibly, correctness gates fail or gains disappear with modest useful work. A warm branch-loop ratio alone cannot justify deployment.

Retained memory contexts, generation-based cache clears and new translation tiers remain separate research. Combining them with this first probe would obscure both correctness and measurement.
