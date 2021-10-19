use std::path::Path;

fn main() {
    // Tell Cargo that if the given file changes, to rerun this build script.
    println!("cargo:rerun-if-changed=src/binary_parser/record.c");
    println!("cargo:rerun-if-changed=src/binary_parser/stream.c");
    println!("cargo:rerun-if-changed=src/binary_parser/stream.h");

    // Build a C file and statically link it.
    cc::Build::new()
        .file("src/binary_parser/stream.c")
        .file("src/binary_parser/record.c")
        .include(Path::new("/usr/local/include/traceevent"))
        .include(Path::new("/usr/local/include/tracefs"))
        .include(Path::new("/usr/local/include/trace-cmd"))
        .flag("-ltraceevent")
        .flag("-ltracefs")
        .flag("-ltracecmd")
        .compile("binparse");

    // Linker options for rustc.
    println!("cargo:rustc-link-search=/usr/local/include/traceevent");
    println!("cargo:rustc-link-search=/usr/local/include/tracefs");
    println!("cargo:rustc-link-search=/usr/local/include/trace-cmd");
    println!("cargo:rustc-link-lib=traceevent");
    println!("cargo:rustc-link-lib=tracefs");
    println!("cargo:rustc-link-lib=tracecmd");

    // Generate bindings (reminder)
    // bindgen -o src/bindings.rs src/binary_parser/stream.h -- -I/usr/local/include/traceevent -I/usr/local/include/tracefs -I/usr/local/include/trace-cmd
}