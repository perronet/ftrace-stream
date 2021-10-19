use std::ptr;
use std::ffi::CString;
use std::mem::MaybeUninit;

#[allow(non_camel_case_types, non_snake_case, non_upper_case_globals)]
pub mod bindings;

use bindings::*;

// In seconds
const TRACE_DURATION: u64 = 10;

fn c_str(s: &str) -> CString {
    CString::new(s).unwrap()
}

unsafe fn setup_ftrace(tracefs: *mut tracefs_instance) {
    // Set clock (also empties the buffer)
    tracefs_instance_file_write(tracefs, c_str("trace_clock").as_ptr(), c_str("mono").as_ptr());
    // Activate events
    tracefs_instance_file_write(tracefs, c_str("events/sched/sched_wakeup/enable").as_ptr(), c_str("1").as_ptr());
    tracefs_instance_file_write(tracefs, c_str("events/sched/sched_wakeup_new/enable").as_ptr(), c_str("1").as_ptr());
    tracefs_instance_file_write(tracefs, c_str("events/sched/sched_switch/enable").as_ptr(), c_str("1").as_ptr());
    tracefs_instance_file_write(tracefs, c_str("events/sched/sched_process_exit/enable").as_ptr(), c_str("1").as_ptr());
}

unsafe fn start_tracing(tracefs: *mut tracefs_instance) {
    tracefs_instance_file_write(tracefs, c_str("tracing_on").as_ptr(), c_str("1").as_ptr());
}

unsafe fn stop_tracing(tracefs: *mut tracefs_instance) {
    tracefs_instance_file_write(tracefs, c_str("tracing_on").as_ptr(), c_str("0").as_ptr());
}

unsafe fn shutdown_ftrace(tracefs: *mut tracefs_instance) {
    stop_tracing(tracefs);
    tracefs_instance_file_write(tracefs, c_str("set_event").as_ptr(), c_str("").as_ptr());
    tracefs_instance_file_write(tracefs, c_str("trace").as_ptr(), c_str("").as_ptr());
    tracefs_instance_file_write(tracefs, c_str("set_event_pid").as_ptr(), c_str("").as_ptr());
    tracefs_instance_destroy(tracefs);
}

fn main() {
    unsafe {
        let tracefs: *mut tracefs_instance = tracefs_instance_create(ptr::null());
        let cpu_cnt: i32 = tracecmd_count_cpus();
        let mut event = MaybeUninit::<rbftrace_event>::uninit();
        
        setup_ftrace(tracefs);

        // Array of recorder_data: one per cpu
        let recorders: *mut recorder_data = create_recorders(tracefs, cpu_cnt);

        start_tracing(tracefs);

        let start = std::time::Instant::now();
        loop {
            let ret = read_stream(recorders, cpu_cnt, event.as_mut_ptr());

            let elapsed = start.elapsed();
            if ret >= 1 && elapsed.as_secs() < TRACE_DURATION {
                print_event(event.as_mut_ptr());
            } else {
                break;
            }
        }

        stop_tracing(tracefs);
        stop_threads(recorders, cpu_cnt);
        wait_threads(recorders, cpu_cnt);
        shutdown_ftrace(tracefs);
    }
}
