# CRC32C experimental branch record

Production commit: `9ceb909`. Tests: `c790a2c`. This branch starts from the selected seven-patch source at `a304586` and stays independent of the SHA256SU0 experiment.

The report below records reviewed measurements. Its named raw artifacts and benchmark harness are external frozen evidence, stored on the original workstation under `E:/VMs/ARM64-Perf/research-crc32c`; they are not repository-relative files. This checkout contains the production change and standard host/TCG correctness tests, not a complete benchmark reproduction package. No Windows images, account data or executable packages are published here.

The capability-off detector patch is a private validation artifact. It is not included in this branch's production code or exposed as a runtime option. The Windows VM continues using the previously selected build; this branch's Windows application benefit is unmeasured.

## CRC32C hardware-helper results

The native candidate improved all six complete-buffer CRC workloads by
paired median, compared with the selected native baseline. Single-stream
gains were substantially larger than four-stream gains. Copy controls had
median changes within 1.3%, but individual samples and order groups varied.
These results describe the frozen generic AArch64 guest, not Windows
application performance or whole-system speedup.

## Identity and scope

- Production patch SHA256: `2d79e7876947758cf7706220d8422816aa5f5e87d449f9f9367c414d49293233`.
- Selected native baseline: `b3193edcb0e61cb22aa50f981288cb9469e2891409dd07c213b85a36e7a48578`.
- Native candidate: `396a1d01163dd2d269f7978dc60a38c7122c5ff6aefd3a131f212fd1c7130ebe`.
- Guest ELF: `f9cdd9916017b78b1724a7e72c2df6e831966f897c0654328ca032dddd059be0`.
- Raw timing artifact `guest/native-benchmark.json`: `0f1b136a85f2916087c7e70be675d4d1a477cfa2448491ef9cf81fe9cbc62634`.

The selected baseline retains its existing SHA unroll hint. The candidate
changes only the AArch64 CRC32C helper mapping and required host capability
support. IEEE CRC32, shared utility semantics, and TCG translation remain
unchanged. Builds and portable ISA evidence are recorded by the separate
builder under `prototype-build/council-builds/`.

## Functional evidence

Each of the four emulator variants passed 85,268 instruction/small-buffer
assertions, 15 complete CRC messages, and three byte-verified copy buffers:

| Variant | Artifact | Outcome |
| --- | --- | --- |
| Selected native baseline | `guest/baseline-functional.json` | Pass, exit 0 |
| Native CRC candidate | `guest/native-functional.json` | Pass, exit 0 |
| Portable CRC candidate | `guest/portable-functional.json` | Pass, exit 0 |
| Portable capability forced off | `guest/portable-functional.json` | Pass, exit 0 |

The portable candidate hash is
`1c160dec5826ca6fc2f379e952d7bb8b1ddb163ad6cb9c16dd8e37cd413cc4cd`;
the forced-off hash is
`7bd5769666325f89697d5c09275a75836433534938be408d8e5ef9009fb80d50`.
The private validation patch removes only the CPUINFO_SSE42 detector
assignment. The cached portable build uses `-O2`, not native tuning.
Its host unit results are buffer+acceleration pass for capability on, and
buffer pass plus explicit acceleration skip for capability off. Those are
not two successful acceleration tests. This proves forced fallback on a
capable host; genuinely unsupported hardware was not executed.

## Measured matrix

All 216 measured rows passed output, identity, and minimum-duration checks.
Each case used 12 pairs, six in baseline/candidate order and six reversed.
The repetition count was calibrated on the candidate to at least 0.5 s,
then held common across both variants. The minimum measured ROI was
0.487885 s, exceeding the declared 0.2 s floor. No samples were dropped.

The table reports median elapsed seconds for each variant, median paired
speedup, and median paired candidate time change. Negative time change is
faster. A ratio of medians can differ slightly from median paired speedup.
Counts differ between cases; compare baseline and candidate within a row.

| Mode | Message bytes | Repetitions | Baseline s | Candidate s | Paired speedup | Candidate time change |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Single stream | 64 | 5,767,168 | 1.129941 | 0.616922 | 1.8404x | -45.665% |
| Single stream | 4,096 | 262,144 | 2.076646 | 0.526014 | 3.9555x | -74.718% |
| Single stream | 4,194,307 | 256 | 2.146355 | 0.603745 | 3.5551x | -71.869% |
| Four streams | 64 | 524,288 | 0.657855 | 0.550099 | 1.1891x | -15.904% |
| Four streams | 4,096 | 12,288 | 0.791879 | 0.660878 | 1.2212x | -18.108% |
| Four streams | 4,194,307 | 11 | 0.781687 | 0.628198 | 1.2361x | -19.100% |
| Copy/integer control | 64 | 1,310,720 | 0.626690 | 0.629015 | 1.0095x | -0.908% |
| Copy/integer control | 4,096 | 20,480 | 0.518474 | 0.517324 | 1.0105x | -1.023% |
| Copy/integer control | 4,194,307 | 20 | 0.530915 | 0.535321 | 0.9877x | +1.252% |

Four-stream repetitions process four independent messages of the stated
size. All timings include buffer loads, CRC or copy instructions, byte
tails, per-message initialization, result checks, and a result sink. They
exclude startup, input generation, and the functional instruction suite.
There is no warm-up phase: this is a cold-first aggregate, including the
first loop translation followed by repeated execution, not isolated cold
latency or warm-only throughput.

## Variation and controls

| Mode | Message bytes | Minimum pair speedup | Maximum pair speedup | AB median | BA median |
| --- | ---: | ---: | ---: | ---: | ---: |
| Single stream | 64 | 1.7692x | 1.9214x | 1.8330x | 1.8465x |
| Single stream | 4,096 | 3.6556x | 4.0909x | 3.8867x | 3.9730x |
| Single stream | 4,194,307 | 3.2964x | 3.7843x | 3.4550x | 3.6183x |
| Four streams | 64 | 0.9449x | 1.6018x | 1.1982x | 1.1549x |
| Four streams | 4,096 | 1.0095x | 1.3322x | 1.2212x | 1.2167x |
| Four streams | 4,194,307 | 1.1570x | 1.4548x | 1.2361x | 1.2326x |
| Copy/integer control | 64 | 0.8586x | 1.0763x | 1.0295x | 0.9819x |
| Copy/integer control | 4,096 | 0.9325x | 1.2651x | 0.9862x | 1.0309x |
| Copy/integer control | 4,194,307 | 0.8587x | 1.1107x | 0.9571x | 1.0311x |

The six CRC medians improve in both order groups. The 64-byte four-stream
case includes one slower candidate pair and has substantial variation;
do not describe every run as faster. Copy-control medians show no
regression above 2% in this matrix, but their order groups change sign.
For example, the largest control is slower in AB and faster in BA order.
This supports a bounded "no material median control regression observed"
statement, not a claim of zero overhead or proof against all regressions.

The results support the small hardware-helper optimization for these
CRC-heavy workloads. They do not establish CRC instruction frequency in
Windows, a complete Windows boot improvement, or a need for a new TCG
opcode. The portable correctness profiles were not used for performance
comparisons. Further architectural changes would need their own comparison
against this accelerated helper.

`guest/analyze_native.py` validates every pair and produces
`guest/native-analysis.json`, including individual paired times. The
independent measurement review is recorded separately in
`RESULTS-REVIEW.md`. No commit, push, deployment, or package replacement
was performed by this implementation/measurement owner.
