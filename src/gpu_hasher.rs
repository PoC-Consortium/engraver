use chan::Receiver;
use ocl::{gpu_hash, gpu_hash_and_transfer_to_host, gpu_transfer_to_host, GpuContext};
use std::sync::mpsc::Sender;
use std::sync::{Arc, Mutex};

pub struct SafePointer {
    pub ptr: *mut u8,
}
unsafe impl Send for SafePointer {}
unsafe impl Sync for SafePointer {}

pub struct GpuTask {
    pub cache: SafePointer,
    pub cache_size: u64,
    pub chunk_offset: u64,
    pub numeric_id: u64,
    pub local_startnonce: u64,
    pub local_nonces: u64,
}

pub fn create_gpu_hasher_thread(
    gpu_id: u8,
    gpu_context: Arc<Mutex<GpuContext>>,
    tx: Sender<(u8, u8, u64)>,
    rx_hasher_task: Receiver<Option<GpuTask>>,
) -> impl FnOnce() {
    move || {
        let mut first_run = true;
        let mut buffer_id = 0u8;
        let mut last_task = GpuTask {
            cache: SafePointer { ptr: &mut 0u8 },
            cache_size: 0,
            chunk_offset: 0,
            numeric_id: 0,
            local_startnonce: 0,
            local_nonces: 0,
        };
        for task in rx_hasher_task {
            // check if new task or termination
            match task {
                // new task
                Some(task) => {
                    // first run - just hash
                    if first_run {
                        if task.local_nonces != 0 {
                            first_run = false;
                            gpu_hash(&gpu_context, &task);
                            buffer_id = 1 - buffer_id;
                            last_task = task;
                            tx.send((gpu_id, 1u8, 0))
                                .expect("GPU task can't communicate with scheduler thread.");
                        }
                    // last run - just transfer
                    } else if task.local_nonces == 0 {
                        gpu_transfer_to_host(&gpu_context, buffer_id, &last_task);
                        first_run = true;
                        buffer_id = 0;
                        tx.send((gpu_id, 0u8, last_task.local_nonces))
                            .expect("GPU task can't communicate with scheduler thread.");
                    // normal run - hash and transfer async
                    } else {
                        gpu_hash_and_transfer_to_host(
                            &gpu_context,
                            buffer_id,
                            &task,
                            &last_task,
                        );
                        buffer_id = 1 - buffer_id;
                        tx.send((gpu_id, 0u8, last_task.local_nonces))
                            .expect("GPU task can't communicate with scheduler thread.");
                        last_task = task;
                        tx.send((gpu_id, 1u8, 0))
                            .expect("GPU task can't communicate with scheduler thread.");
                    }
                }
                // termination
                None => {
                    break;
                }
            }
        }
    }
}
