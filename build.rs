use std::path::Path;
use std::process::Command;

fn main() {
    // Tell Cargo that if the given file changes, to rerun this build script.
    println!("cargo:rerun-if-changed=src/binary_parser/record.c");
    println!("cargo:rerun-if-changed=src/binary_parser/stream.c");
    println!("cargo:rerun-if-changed=src/binary_parser/stream.h");

    // Compile the trace-cmd libraries.
    let mut build_libs_command = Command::new("./build_libs.sh");
    build_libs_command.current_dir("src/binary_parser/lib");
    build_libs_command.status().expect("Failed to build trace-cmd libraries.");

    // Build our own library and statically link it to the rust binary.
    cc::Build::new()
        .file("src/binary_parser/stream.c")
        .file("src/binary_parser/record.c")
        .include(Path::new("src/binary_parser/lib/build_output/usr/include/traceevent"))
        .include(Path::new("src/binary_parser/lib/build_output/usr/include/tracefs"))
        .include(Path::new("src/binary_parser/lib/build_output/usr/include/trace-cmd"))
        .flag("-Lsrc/binary_parser/lib/build_output/usr/lib64")
        .compile("binparse");

    // Linker options for rustc (link trace-cmd libraries).
    println!("cargo:rustc-link-lib=dylib=traceevent");
    println!("cargo:rustc-link-lib=dylib=tracefs");
    println!("cargo:rustc-link-lib=dylib=tracecmd");
    println!("cargo:rustc-link-search=native=src/binary_parser/lib/build_output/usr/lib64");

    // Without this we won't be able to find the library when running cargo run
    // Note: this *only works* with cargo run
    println!("cargo:rustc-env=LD_LIBRARY_PATH=src/binary_parser/lib/build_output/usr/lib64");

    // Generate bindings (reminder)
    // bindgen -o src/bindings.rs src/binary_parser/stream.h -- -I/usr/local/include/traceevent -I/usr/local/include/tracefs -I/usr/local/include/trace-cmd
}
