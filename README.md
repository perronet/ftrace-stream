# ftrace-stream
A tool to stream ftrace events in binary format. This tool was created to easily get access to binary events, which is normally done through `tracefs` (in `per_cpu/cpu0/trace_pipe_raw`). Parsing events in a binary format is an efficient alternative to text parsing (i.e. parsing events out of `trace_pipe` or `trace`).

The tool consists of:
* A core in C that uses `trace-cmd`'s libraries to create a stream of event
* A small rust wrapper with bindings to call the C functions of the core part

## Quick start

```
cargo build
./run.sh
```

## Libraries

The tool depends on the following libraries:
* [libtracefs](https://www.trace-cmd.org/Documentation/libtracefs/libtracefs.html)
* [libtraceevent](https://www.trace-cmd.org/Documentation/libtraceevent/libtraceevent.html)
* [libtracecmd](https://manpages.debian.org/experimental/trace-cmd/libtracecmd.3.en.html)

If you don't have them, run the install script before `cargo build`:

```
cd src/binary_parser/lib
sudo ./install.sh
```
