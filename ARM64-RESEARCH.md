# ARM64 TCG optimization experiments

This fork investigates AArch64 system emulation on an Intel x86-64 Windows host. The research branch is `codex/arm64-optimization`, based on QEMU 11.1.3, commit `005c61c0ef6f6ae615283440b4bb7be51b2c6ddd`.

## Selected source

Seven separate commits preserve the currently tested prototype bundle:

| Change | Commit | Evidence boundary |
| --- | --- | --- |
| Cached BTI helper check | 36dcce11d4be | Superseded on eligible paths by generated checks below. |
| Additional Win64 SIMD registers | 4f82b9806b0e | Specific vector loops improved; helper-call clobbers remain conservative. |
| High-resolution Windows timer polling | b5fedae5f4fe | Short waits and interrupt delivery improved; one 1 ms wait mean regressed. |
| Generated guarded BTI checks | b63a896dc7d2 | Affected indirect-branch loop improved 1.351x; fallback remains. |
| Windows socket close repair | 116ae3fe03b1 | Uses closesocket for Windows sockets. |
| Upstream SEV wakeup repair | a6068771fee2 | All 12 reproduced MTTCG failures and 24 controls pass. |
| Four-round SHA256 H/H2 unroll hints | a30458692016 | Combined with native compilation, complete-message SHA improves. |

The import retained the exact tested code after CRLF-to-LF normalization. Patch hashes are in commit messages. Import verification is source evidence; it is not a new runtime test.

## Measured results

On an i9-9900K, GCC 16.2 UCRT builds retained `-O2` and debug symbols. Native compilation added only `-march=native`; this host has no SHA-NI or AVX512. TCG already selects instructions using runtime host capabilities.

A four-way generic complete-message SHA comparison measured median times of 0.535499 s (portable control), 0.511987 s (native), 0.490627 s (portable plus hints), and 0.467850 s (native plus hints). The combined reduction is 12.63%. Each build passed 2,296 independent correctness assertions, including opcode vectors, alias cases and complete digests. Selected-build BTI regressions passed 6,560 assertions and 876 expected traps across eight configurations.

A separate Windows BCrypt comparison measured 14.44% median paired time reduction for the combined native-plus-hint build. All 42 warmup/measured jobs passed. Integer throughput was unchanged and copy measurements were noisy. These results establish SHA workload improvements; general Windows responsiveness and full startup are unresolved.

The original benchmark harnesses and raw evidence currently live outside this source fork. Focused generic tests are being integrated into the experimental branches. The numbers above should not be treated as independently reproducible from this checkout alone yet.

## Independent next experiments

- `codex/arm64-sha256su0`: expand one SHA schedule instruction into ordinary TCG arithmetic, using existing full-message SHA oracles.
- `codex/arm64-crc32c`: runtime-dispatched SSE4.2 Castagnoli arithmetic with portable fallback and independent polynomial tests.
- `codex/arm64-dispatch`: design a generated exact-key hit path using the existing jump cache and unchanged miss helper.

These branches begin at the same selected source. Their implementation, correctness and timing evidence must remain separate. No additional gain is established merely by creating a branch or removing a helper call.

The six-member Astra council also considered retained translation contexts, typed AES/PMULL operations, register-eviction policy and translation tiers. Context retention needs complete dormant invalidation and dirty-tracking semantics. Existing AES/PMULL helpers already use host acceleration. No evidence currently justifies a wholesale JIT replacement.

## Build and validation

Use QEMU's normal build instructions in `docs/devel` and existing `tests/tcg` facilities. `-march=native` produces a host-specific binary; a portable build must retain runtime dispatch. Keep compiler, configuration, guest workload, cache state and repetition counts fixed between paired measurements. Include correctness checks and direct/integer controls, plus cold execution, invalidation and context-switch costs where relevant.

This branch contains prototypes and an upstream backport, not an upstream release. Each future change needs source review, independent correctness checks and calibrated complete-work timing before selection.

## Additional measured experiments

The selected branch now includes the independent SHA256SU0 regression test. The following production experiments remain on separate branches; neither replaces the selected Windows package.

| Experiment | Reviewed result | Status |
| --- | --- | --- |
| [SHA256SU0 ordinary TCG](https://github.com/bacon-jam/qemu/blob/codex/arm64-sha256su0/ARM64-SHA256SU0-RESULTS.md) | Six paired warm full-message comparisons: 1.054286x median paired speedup, about 5.15% less time. All correctness jobs passed; caller code grew. | Held pending cold/control and remaining architectural gates. |
| [CRC32C hardware helper](https://github.com/bacon-jam/qemu/blob/codex/arm64-crc32c/ARM64-CRC32C-RESULTS.md) | 216 measured jobs passed. Single-stream 1.84-3.96x; four-stream 1.19-1.24x median paired speedup. Copy medians within 1.3%, with order-sensitive variation. Native, portable and forced-fallback functional gates passed. | Experimental; Windows workload importance unmeasured. |
| [Generated indirect dispatch](https://github.com/bacon-jam/qemu/blob/codex/arm64-dispatch/ARM64-DISPATCH-DESIGN.md) | Five-file BR x16/x17 design passed independent memory/synchronization review after plugin-order and Windows bitmap-width corrections. | Design only. |

The CRC branch includes host unit tests and generic AArch64 instruction tests. Actual unsupported x86 hardware and normal Linux-user integration were not executed on the Windows research host. Detailed scopes, frozen identities and uncovered gates are in each linked record.
