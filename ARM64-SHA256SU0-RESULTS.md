# SHA256SU0 warm feasibility result

Status: experimental, not selected for the Windows VM. Source change: `6e48d2a`; generic QEMU test: `826d866`. Both start from the selected seven-patch source at `a304586`.

The median paired speed ratio across six pairs is **1.054286**, equivalent to about **5.15% less time** for this warm complete-message SHA workload. All pairs favor the candidate. The separate ratio of elapsed-time medians gives **3.83556% less time**; it is a different statistic.

## Work and raw durations

Intel i9-9900K, Windows x64, GCC16.2 UCRT, unchanged selected native build settings. One AArch64 vCPU, Cortex-A710, MTTCG. Both binaries use the same frozen guest ELF and 618496 input blocks (37.75MiB), plus final SHA padding. The guest checks the complete digest. It runs SHA known-answer tests before the timed region, so this is warm throughput even though each job starts a fresh process.

Two pilot jobs and all 14 final jobs passed. The final matrix contains two warmups and 12 measured jobs, paired in AB, BA, BA, AB, AB, BA order. No compiler or other emulator workload ran during timing. The shortest measured region was 0.436593808s.

| Pair | Baseline seconds | SU0 seconds |
| --- | ---: | ---: |
| 1 | 0.472093200 | 0.464042688 |
| 2 | 0.462621392 | 0.459370496 |
| 3 | 0.498020304 | 0.445281504 |
| 4 | 0.485559792 | 0.459722704 |
| 5 | 0.471976400 | 0.448488704 |
| 6 | 0.470332592 | 0.436593808 |

Median elapsed times: baseline 0.4720348s; SU0 0.4539296s. Every job passed 2296 independent SHA assertions and the expected payload digest `b829dbcc7a0efa83203f2e743c3e0848c44f9604ca054423253df045b4b067fc`.

## Correctness and code generation

The new `tests/tcg/aarch64/test-sha256su0.c` checks 64 lane results, including fixed independent alias answers. It passed on both binaries through a freestanding bridge using the existing EL1 startup. The normal Linux-user integration was not executed on this Windows host. Its standard build/run commands, with an appropriate configured build and cross compiler, are:

```sh
make build-tcg-tests-aarch64-linux-user
make -C tests/tcg/aarch64-linux-user run-test-sha256su0 V=1
```

Separate filtered diagnostics showed two SU0 helper calls in the baseline and none in the candidate. The two caller TBs grew from 656/480 bytes to 792/616 bytes, respectively. Each grew 136 bytes; these figures exclude outlined helper code. Unchanged direct stack references do not establish unchanged spill counts.

## Identities and limits

- Baseline binary SHA256: `b3193edcb0e61cb22aa50f981288cb9469e2891409dd07c213b85a36e7a48578`.
- Candidate binary SHA256: `4bc03d1f872b5d7b03a03c44c61fd3326bac127943af7bfdfd9af1eb2d45ee76`.
- Frozen full-SHA ELF SHA256: `089911545538380a0dec1d358972eeb688d3ba0d898c724a7b6f31b11884c2ce`.

Independent Astra review checked source semantics, build provenance, raw timing arithmetic and the final narrative. The original frozen benchmark harness and detailed logs remain external to this source fork; this report alone is not a complete benchmark reproduction package.

Cold translation, integer/copy controls, SVE upper-tail sentinels, unavailable-feature/denied-FP traps, page-end/step/icount behavior, big-endian-host execution and Windows workload validation remain unrun. The original helper is retained for big-endian hosts. A warm SHA result does not establish general emulator responsiveness or justify deployment.

After measurement, the test source was normalized from CRLF to LF only. The tested snapshot remains at `826d866`; its raw SHA256 is `dcd0ba5a9edec88c810878fa01c422db53053cbe90f5223e6d311d35f01746e0`. The published LF source SHA256 is `0da0cc60aebb10ddef8611976ca82f2ee37544a1da2f491f81692c02bba1d05b`. Exact comparison after newline normalization passes; production code and recorded measurements are unchanged. Historical build manifests retain the original bytes and were not rewritten.
