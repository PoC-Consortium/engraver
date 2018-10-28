extern crate pbr;
extern crate rayon;

use chan;
use libc::{c_void, size_t, uint64_t};
use plotter::{Buffer, PlotterTask};
use std::cmp::min;
use std::sync::mpsc::{channel, Sender};
use std::sync::Arc;

const TASK_SIZE: u64 = 64;
const NONCE_SIZE: u64 = (2 << 17);

extern "C" {
    pub fn noncegen(
        cache: *mut c_void,
        cache_size: size_t,
        chunk_offset: size_t,
        numeric_ID: uint64_t,
        local_startnonce: uint64_t,
        local_nonces: uint64_t,
    );
    pub fn noncegen_sse(
        cache: *mut c_void,
        cache_size: size_t,
        chunk_offset: size_t,
        numeric_ID: uint64_t,
        local_startnonce: uint64_t,
        local_nonces: uint64_t,
    );
    pub fn noncegen_avx(
        cache: *mut c_void,
        cache_size: size_t,
        chunk_offset: size_t,
        numeric_ID: uint64_t,
        local_startnonce: uint64_t,
        local_nonces: uint64_t,
    );
    pub fn noncegen_avx2(
        cache: *mut c_void,
        cache_size: size_t,
        chunk_offset: size_t,
        numeric_ID: uint64_t,
        local_startnonce: uint64_t,
        local_nonces: uint64_t,
    );
    pub fn noncegen_avx512(
        cache: *mut c_void,
        cache_size: size_t,
        chunk_offset: size_t,
        numeric_ID: uint64_t,
        local_startnonce: uint64_t,
        local_nonces: uint64_t,
    );
}
pub struct SafeCVoid {
    ptr: *mut c_void,
}
unsafe impl Send for SafeCVoid {}

pub struct HasherTaskInfo {
    pub cache: SafeCVoid,
    pub cache_size: size_t,
    pub chunk_offset: size_t,
    pub numeric_id: uint64_t,
    pub local_startnonce: uint64_t,
    pub local_nonces: uint64_t,
}

pub fn hash(tx: Sender<u64>, hasher_task: HasherTaskInfo, simd_ext: String) -> impl FnOnce() {
    move || {
        unsafe {
            match &*simd_ext {
                "AVX512F" => noncegen_avx512(
                    hasher_task.cache.ptr,
                    hasher_task.cache_size,
                    hasher_task.chunk_offset,
                    hasher_task.numeric_id,
                    hasher_task.local_startnonce,
                    hasher_task.local_nonces,
                ),
                "AVX2" => noncegen_avx2(
                    hasher_task.cache.ptr,
                    hasher_task.cache_size,
                    hasher_task.chunk_offset,
                    hasher_task.numeric_id,
                    hasher_task.local_startnonce,
                    hasher_task.local_nonces,
                ),
                "AVX" => noncegen_avx(
                    hasher_task.cache.ptr,
                    hasher_task.cache_size,
                    hasher_task.chunk_offset,
                    hasher_task.numeric_id,
                    hasher_task.local_startnonce,
                    hasher_task.local_nonces,
                ),
                "SSE2" => noncegen_sse(
                    hasher_task.cache.ptr,
                    hasher_task.cache_size,
                    hasher_task.chunk_offset,
                    hasher_task.numeric_id,
                    hasher_task.local_startnonce,
                    hasher_task.local_nonces,
                ),                
                _ => noncegen(
                    hasher_task.cache.ptr,
                    hasher_task.cache_size,
                    hasher_task.chunk_offset,
                    hasher_task.numeric_id,
                    hasher_task.local_startnonce,
                    hasher_task.local_nonces,
                ),
            }
        }
        tx.send(hasher_task.local_nonces)
            .expect("Pool task can't communicate with hasher thread.");
    }
}

pub fn create_hasher_task(
    task: Arc<PlotterTask>,
    thread_pool: rayon::ThreadPool,
    mut nonces_hashed: u64,
    mut pb: Option<pbr::ProgressBar<pbr::Pipe>>,
    rx_empty_buffers: chan::Receiver<Buffer>,
    tx_buffers_to_writer: chan::Sender<Buffer>,
    simd_ext: String,
) -> impl FnOnce() {
    move || {
        for buffer in rx_empty_buffers {
            let mut_bs = &buffer.get_buffer();
            let mut bs = mut_bs.lock().unwrap();
            let buffer_size = (*bs).len() as u64;
            let nonces_to_hash = min(buffer_size / NONCE_SIZE, task.nonces - nonces_hashed);

            let mut n_jobs = nonces_to_hash as usize / TASK_SIZE as usize;
            if nonces_to_hash % TASK_SIZE > 0 {
                n_jobs += 1;
            }
            let (tx, rx) = channel();

            for j in 0..nonces_to_hash / TASK_SIZE {
                let task = hash(
                    tx.clone(),
                    HasherTaskInfo {
                        cache: SafeCVoid {
                            ptr: bs.as_ptr() as *mut c_void,
                        },
                        cache_size: buffer_size / NONCE_SIZE as size_t,
                        chunk_offset: j * TASK_SIZE as size_t,
                        numeric_id: task.numeric_id,
                        local_startnonce: nonces_hashed + j * TASK_SIZE,
                        local_nonces: TASK_SIZE,
                    },
                    simd_ext.clone(),
                );

                thread_pool.spawn(task);
            }

            // hash remainder
            if nonces_to_hash % TASK_SIZE > 0 {
                let task = hash(
                    tx.clone(),
                    HasherTaskInfo {
                        cache: SafeCVoid {
                            ptr: bs.as_ptr() as *mut c_void,
                        },
                        cache_size: buffer_size / NONCE_SIZE as size_t,
                        chunk_offset: nonces_to_hash / TASK_SIZE * TASK_SIZE as size_t,
                        numeric_id: task.numeric_id,
                        local_startnonce: nonces_hashed + nonces_to_hash / TASK_SIZE * TASK_SIZE,
                        local_nonces: nonces_to_hash % TASK_SIZE,
                    },
                    simd_ext.clone(),
                );
                thread_pool.spawn(task);
            }

            //sync pool and push status to progressbar
            assert_eq!(
                rx.iter().take(n_jobs).fold(0, |a, b| {
                    match &mut pb {
                        Some(pb) => {
                            pb.add(b * 1024 * 256);
                        }
                        None => (),
                    }
                    a + b
                }),
                nonces_to_hash
            );

            nonces_hashed += nonces_to_hash;

            // queue buffer for writing
            tx_buffers_to_writer.send(buffer);

            // thread end
            if task.nonces == nonces_hashed {
                match &mut pb {
                    Some(pb) => {
                        pb.finish_print("Hasher done.");
                    }
                    None => (),
                }
                break;
            };
        }
    }
}
