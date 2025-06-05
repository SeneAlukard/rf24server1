# gbs

Ground base station sample using a copy of headers from the
`rf24drone` project. It demonstrates handling of join requests,
telemetry packets and command broadcasting using the same folder
layout as the drone library.

The source now lives in the repository root using `include/` and
`src/` just like the drone project. Only the drone library itself
remains under `lib/`.
