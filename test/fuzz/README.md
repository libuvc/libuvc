# Fuzzing libuvc

libuvc parses USB descriptors that come straight from the device, before any
of the data has been validated. A malicious or simply broken camera therefore
controls the input to every parser in `src/device.c`, which makes that code
the natural fuzzing target.

`fuzz_descriptor` drives those parsers with a synthetic USB configuration
whose class-specific descriptor bytes come from the fuzzer. It is the same
shape as the reproducer in [libuvc/libuvc#300][300], and finds that bug from
an empty corpus in under a minute.

The entry point is `uvc_scan_control()` — the same function `uvc_open()`
calls once it has a configuration descriptor. It selects the VideoControl
interface, walks its class-specific blocks and dispatches each to the
parsers, so one call covers the whole path in the #300 report and the fuzzer
re-implements none of libuvc's descriptor walking.

It takes a `uvc_device_handle_t` only to look up a device-specific quirk and
tolerates `NULL`, which is what makes it reachable without a USB device.

`uvc_scan_streaming()` is driven separately. `uvc_scan_control()` only
reaches it through a VideoControl header that names a valid interface, so
calling it directly gets there in far fewer mutations.

[300]: https://github.com/libuvc/libuvc/issues/300

## What is not fuzzed, and why

The pixel format converters in `src/frame.c` read the same untrusted bytes,
but fuzzing them adds nothing: they are branch-free arithmetic over the
payload, so the pixel values themselves never change the control flow. The
only thing that does is the relationship between the frame's declared
`width`/`height` and how many bytes it actually holds, and `test_frame.cc`
already enumerates that exhaustively across every converter and a range of
geometries. A fuzzer would explore a space the unit tests already cover.

## Building

The fuzzers need clang, and are only useful with sanitizers on:

```sh
cmake -B build-fuzz \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DBUILD_TESTING=ON \
  -DBUILD_FUZZERS=ON \
  -DENABLE_SANITIZERS=ON
cmake --build build-fuzz --target fuzz_descriptor
```

## Running

```sh
mkdir -p corpus
./build-fuzz/test/fuzz_descriptor -max_total_time=60 corpus
```

From an empty corpus this finds an out-of-bounds read in a few seconds. To
keep going past the first crash and collect distinct ones:

```sh
./build-fuzz/test/fuzz_descriptor -fork=4 -ignore_crashes=1 \
  -max_total_time=300 -artifact_prefix=./crashes/ corpus
```

Replay a single input:

```sh
./build-fuzz/test/fuzz_descriptor crashes/crash-<hash>
```

## Input format

    byte 0    how many interfaces the configuration declares (1 to 8)
    byte 1    which interface index to scan
    byte 2..  the interface's `extra` (class-specific) descriptor block

Keeping the first two out of the descriptor bytes matters for #300: that bug
needs an interface index the configuration does not actually have, so the
fuzzer has to vary the count and the index independently of the block.

Both entry points do their own bounds handling on the block, so the harness
imposes none of its own: whatever `bLength` says is what the library sees.

## Regression corpus

`test/corpus/fuzz_descriptor/` holds two kinds of input, replayed as an
ordinary CTest case (`fuzz_descriptor_corpus`) with `-runs=0` so they are
checked on every test run without anyone having to fuzz:

- `crash-*` are inputs that crash. `crash-scan-streaming-oob` is #300 --
  four bytes, found from an empty corpus. **It still crashes**, because the
  bug is unfixed, so this test fails until it is.
- `seed-*` are well-formed descriptors -- a VC header with and without a
  streaming interface, a VS input header, a camera input terminal. They give
  the fuzzer valid structure to mutate rather than making it rediscover the
  descriptor format byte by byte.

Add a new crash by copying the input libFuzzer wrote into that directory.

## OSS-Fuzz

The build honours `$LIB_FUZZING_ENGINE` when it is set, and otherwise falls
back to `-fsanitize=fuzzer`, so the same target works under OSS-Fuzz's build
environment without a separate build file.
