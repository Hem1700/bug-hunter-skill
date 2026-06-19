---
id: c-decompressor-unbounded-output
languages: [c, cpp]
cwe: 787
severity-typical: high
seed-cves: []
---

# Decompressor output not bounded to destination

The pattern: a wrapper around a decompression library (zlib, lz4, lzma, zstd, …) passes the destination pointer but sets `avail_out` to a hard-coded value instead of the actual destination buffer size. Result: decompressing crafted data overflows the destination buffer; any post-decompress size check is too late.

The U-Boot cramfs symlink case from the reference session is exactly this — `avail_out = 4096 * 2;` regardless of the destination's actual `size + 1` allocation.

## Detection
```
grep -rnE 'avail_out\s*=\s*[^=]' <subsys>
grep -rnE '(inflate|LZ4_decompress|lzma_run|zstd_decompress)' <subsys>
```

## When the pattern is buggy
- `avail_out` (or the equivalent output-size argument) is a fixed constant or unrelated to the actual buffer size
- The destination is sized from untrusted metadata (file inode, packet field)
- The "validity check" happens *after* decompression rather than via the output bound

## Common false positives
- `avail_out` is computed from `sizeof(dst)` or the dynamic allocation size
- The function operates only on trusted, locally-built buffers

## Verify (Phase 4)
Craft a small "declared size" with compressed data that inflates to a large size. ASan catches the OOB write into the heap allocation.

## Fix shape
Plumb the real destination size into the wrapper:
```c
int decompress_block(void *dst, size_t dst_size, const void *src, size_t src_size) {
    stream.avail_out = dst_size;   /* not a constant */
    ...
}
```
