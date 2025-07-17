# ccask

**ccask** is a pure‑C re‑implementation of the Bitcask log‑structured key/value store. It provides a fast, append‑only on‑disk storage engine with an in‑memory hash index (the “keydir”), hint files for fast recovery, and support threading and CRC‑protected records.

> Inspired by [Bitcask: A Log‑Structured Hash Table for Fast Key/Value Data](https://riak.com/assets/bitcask-intro.pdf)

## Getting Started
**ccask** can be used as an embedded key-value database in any other application. The public API is accessible through the `ccask.h` header file.

```c

#include "ccask.h"

int main() {
    ccask_options_t opts;
    opts.data_dir = "<path-to-data-directory>";
    opts.writer_ringbuf_capacity = 100;
    opts.datafile_rotate_threshold = 100;
    ccask_init(opts);

    /* You can run get, put, delete, etc operations here */

    ccask_shutdown();
    return 0;
}

```
