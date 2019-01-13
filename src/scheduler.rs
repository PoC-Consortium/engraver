extern crate pbr;
extern crate rayon;

use chan;
use cpu_hasher::{hash_cpu, CpuTask, SafeCVoid};
#[cfg(feature = "opencl")]
use gpu_hasher::{create_gpu_hasher_thread, GpuTask, SafePointer};
use libc::{c_void, size_t};
#[cfg(feature = "opencl")]
use ocl::gpu_init;
use plotter::{Buffer, PlotterTask};
use std::cmp::min;
use std::sync::mpsc::channel;
use std::sync::Arc;
#[cfg(feature = "opencl")]
use std::thread;
use plotter::NONCE_SIZE;

const CPU_TASK_SIZE: u64 = 64;

pub fn create_scheduler_thread(
    task: Arc<PlotterTask>,
    thread_pool: rayon::ThreadPool,
    mut nonces_hashed: u64,
    mut pb: Option<pbr::ProgressBar<pbr::Pipe>>,
    rx_empty_buffers: chan::Receiver<Buffer>,
    tx_buffers_to_writer: chan::Sender<Buffer>,
    simd_ext: String,
) -> impl FnOnce() {
    move || {
        // synchronisation chanel for all hashing devices (CPU+GPU)
        // message protocol:    (hash_device_id: u8, message: u8, nonces processed: u64)
        // hash_device_id:      0=CPU, 1=GPU0, 2=GPU1...
        // message:             0 = data ready to write
        //                      1 = device ready to compute next hashing batch
        // nonces_processed:    nonces hashed / nonces writen to host buffer
        let (tx, rx) = channel();

        // create gpu threads and channels
        #[cfg(feature = "opencl")]
        let gpu_contexts = match &task.gpus {
            Some(x) => Some(gpu_init(&x, task.zcb)),
            None => None,
        };

        #[cfg(feature = "opencl")]
        let gpus = match gpu_contexts {
            Some(x) => x,
            None => Vec::new(),
        };
        #[cfg(feature = "opencl")]
        let mut gpu_threads = Vec::new();
        #[cfg(feature = "opencl")]
        let mut gpu_channels = Vec::new();

        #[cfg(feature = "opencl")]
        for (i, gpu) in gpus.iter().enumerate() {
            gpu_channels.push(chan::unbounded());
            gpu_threads.push(thread::spawn({
                create_gpu_hasher_thread(
                    (i + 1) as u8,
                    gpu.clone(),
                    tx.clone(),
                    gpu_channels.last().unwrap().1.clone(),
                )
            }));
        }

        for buffer in rx_empty_buffers {
            let mut_bs = &buffer.get_buffer();
            let mut bs = mut_bs.lock().unwrap();
            let buffer_size = (*bs).len() as u64;
            let nonces_to_hash = min(buffer_size / NONCE_SIZE, task.nonces - nonces_hashed);

            let mut requested = 0u64;
            let mut processed = 0u64;

            // kickoff first gpu and cpu runs
            #[cfg(feature = "opencl")]
            for (i, gpu) in gpus.iter().enumerate() {
                // schedule next gpu task
                let mut gpu = gpu.lock().unwrap();
                let task_size = min(gpu.worksize as u64, nonces_to_hash - requested);
                if task_size > 0 {
                    gpu_channels[i].0.send(Some(GpuTask {
                        cache: SafePointer {
                            ptr: bs.as_mut_ptr(),
                        },
                        cache_size: buffer_size / NONCE_SIZE,
                        chunk_offset: requested,
                        numeric_id: task.numeric_id,
                        local_startnonce: task.start_nonce + nonces_hashed + requested,
                        local_nonces: task_size,
                    })).unwrap();
                }
                requested += task_size;
                //println!("Debug: Device: {} started. {} nonces assigned. Total requested: {}\n\n\n",i+1,task_size,requested);
            }

            for _ in 0..task.cpu_threads {
                let task_size = min(CPU_TASK_SIZE, nonces_to_hash - requested);
                if task_size > 0 {
                    let task = hash_cpu(
                        tx.clone(),
                        CpuTask {
                            cache: SafeCVoid {
                                ptr: bs.as_ptr() as *mut c_void,
                            },
                            cache_size: (buffer_size / NONCE_SIZE) as usize,
                            chunk_offset: requested as usize,
                            numeric_id: task.numeric_id,
                            local_startnonce: task.start_nonce + nonces_hashed + requested,
                            local_nonces: task_size,
                        },
                        simd_ext.clone(),
                    );
                    thread_pool.spawn(task);
                }
                requested += task_size;
            }

            // control loop
            let rx = &rx;
            for msg in rx {
                match msg.1 {
                    // process a request for work: provide a task or signal completion
                    1 => {
                        let task_size = match msg.0 {
                            0 => {
                                // schedule next cpu task
                                let task_size = min(CPU_TASK_SIZE, nonces_to_hash - requested);
                                if task_size > 0 {
                                    let task = hash_cpu(
                                        tx.clone(),
                                        CpuTask {
                                            cache: SafeCVoid {
                                                ptr: bs.as_ptr() as *mut c_void,
                                            },
                                            cache_size: (buffer_size / NONCE_SIZE) as usize,
                                            chunk_offset: requested as size_t,
                                            numeric_id: task.numeric_id,
                                            local_startnonce: task.start_nonce
                                                + nonces_hashed
                                                + requested,
                                            local_nonces: task_size,
                                        },
                                        simd_ext.clone(),
                                    );
                                    thread_pool.spawn(task);
                                }
                                task_size
                            }
                            _ => {
                                // schedule next gpu task
                                #[cfg(feature = "opencl")]
                                let mut gpu = gpus[(msg.0 - 1) as usize].lock().unwrap();
                                #[cfg(feature = "opencl")]
                                let task_size =
                                    min(gpu.worksize as u64, nonces_to_hash - requested);

                                // optimisation: leave some work for cpu in dual mode
                                #[cfg(feature = "opencl")]
                                let task_size = if task_size < gpu.worksize as u64
                                    && task.cpu_threads > 0
                                    && task_size > CPU_TASK_SIZE
                                {
                                    task_size / 2
                                } else {
                                    task_size
                                };

                                #[cfg(not(feature = "opencl"))]
                                let task_size = 0;

                                #[cfg(feature = "opencl")]
                                gpu_channels[(msg.0 - 1) as usize].0.send(Some(GpuTask {
                                    cache: SafePointer {
                                        ptr: bs.as_mut_ptr(),
                                    },
                                    cache_size: buffer_size / NONCE_SIZE,
                                    chunk_offset: requested,
                                    numeric_id: task.numeric_id,
                                    local_startnonce: task.start_nonce + nonces_hashed + requested,
                                    local_nonces: task_size,
                                })).unwrap();
                                task_size
                            }
                        };

                        requested += task_size;
                        //println!("Debug: Device: {} asked for work. {} nonces assigned. Total requested: {}\n\n\n",msg.0,task_size,requested);
                    }
                    // process work completed message
                    0 => {
                        processed += msg.2;
                        match &mut pb {
                            Some(pb) => {
                                pb.add(msg.2 * NONCE_SIZE);
                            }
                            None => (),
                        }
                    }
                    _ => {}
                }
                if processed == nonces_to_hash {
                    break;
                }
            }

            nonces_hashed += nonces_to_hash;

            // queue buffer for writing
            tx_buffers_to_writer.send(buffer).unwrap();

            // thread end
            if task.nonces == nonces_hashed {
                match &mut pb {
                    Some(pb) => {
                        pb.finish_print("Hasher done.");
                    }
                    None => (),
                }
                // shutdown gpu threads
                #[cfg(feature = "opencl")]
                for gpu in &gpu_channels {
                    gpu.0.send(None).unwrap();
                }
                break;
            };
        }
    }
}
