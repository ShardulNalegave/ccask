# ccask

**ccask** is a pure‑C re‑implementation of the Bitcask log‑structured key/value store. It provides a fast, append‑only on‑disk storage engine with an in‑memory hash index (the “keydir”), hint files for fast recovery, and support threading and CRC‑protected records.

> Inspired by [Bitcask: A Log‑Structured Hash Table for Fast Key/Value Data](https://riak.com/assets/bitcask-intro.pdf)
