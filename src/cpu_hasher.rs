use crate::poc_hashing::noncegen_rust;
use libc::{c_void, size_t, uint64_t};
use std::slice::from_raw_parts_mut;
use std::sync::mpsc::Sender;

const NUM_SCOOPS: usize = 4096;
const SCOOP_SIZE: usize = 64;
const NONCE_SIZE: usize = (NUM_SCOOPS * SCOOP_SIZE);

extern "C" {
    pub fn init_shabal_sse2() -> ();
    pub fn init_shabal_avx() -> ();
    pub fn init_shabal_avx2() -> ();
    pub fn init_shabal_avx512f() -> ();
    pub fn noncegen_sse2(
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
pub struct SafePointer {
    pub ptr: *mut u8,
}
unsafe impl Send for SafePointer {}
unsafe impl Sync for SafePointer {}

pub struct CpuTask {
    pub cache: SafePointer,
    pub cache_size: usize,
    pub chunk_offset: usize,
    pub numeric_id: u64,
    pub local_startnonce: u64,
    pub local_nonces: u64,
}

#[derive(Debug, Clone)]
pub enum SimdExtension {
    AVX512f,
    AVX2,
    AVX,
    SSE2,
    None,
}

pub fn init_simd() -> SimdExtension {
    if is_x86_feature_detected!("avx512f") {
        unsafe {
            init_shabal_avx512f();
        }
        SimdExtension::AVX512f
    } else if is_x86_feature_detected!("avx2") {
        unsafe {
            init_shabal_avx2();
        }
        SimdExtension::AVX2
    } else if is_x86_feature_detected!("avx") {
        unsafe {
            init_shabal_avx();
        }
        SimdExtension::AVX
    } else if is_x86_feature_detected!("sse2") {
        unsafe {
            init_shabal_sse2();
        }
        SimdExtension::SSE2
    } else {
        SimdExtension::None
    }
}

pub fn hash_cpu(
    tx: Sender<(u8, u8, u64)>,
    hasher_task: CpuTask,
    simd_ext: SimdExtension,
) -> impl FnOnce() {
    move || {
        unsafe {
            match simd_ext {
                SimdExtension::AVX512f => noncegen_avx512(
                    hasher_task.cache.ptr as *mut c_void,
                    hasher_task.cache_size,
                    hasher_task.chunk_offset,
                    hasher_task.numeric_id,
                    hasher_task.local_startnonce,
                    hasher_task.local_nonces,
                ),
                SimdExtension::AVX2 => noncegen_avx2(
                    hasher_task.cache.ptr as *mut c_void,
                    hasher_task.cache_size,
                    hasher_task.chunk_offset,
                    hasher_task.numeric_id,
                    hasher_task.local_startnonce,
                    hasher_task.local_nonces,
                ),
                SimdExtension::AVX => noncegen_avx(
                    hasher_task.cache.ptr as *mut c_void,
                    hasher_task.cache_size,
                    hasher_task.chunk_offset,
                    hasher_task.numeric_id,
                    hasher_task.local_startnonce,
                    hasher_task.local_nonces,
                ),
                SimdExtension::SSE2 => noncegen_sse2(
                    hasher_task.cache.ptr as *mut c_void,
                    hasher_task.cache_size,
                    hasher_task.chunk_offset,
                    hasher_task.numeric_id,
                    hasher_task.local_startnonce,
                    hasher_task.local_nonces,
                ),
                _ => {
                    let data = from_raw_parts_mut(
                        hasher_task.cache.ptr,
                        hasher_task.cache_size * NONCE_SIZE,
                    );
                    noncegen_rust(
                        data,
                        hasher_task.chunk_offset,
                        hasher_task.numeric_id,
                        hasher_task.local_startnonce,
                        hasher_task.local_nonces,
                    )
                }
            }
        }
        // report hashing done
        tx.send((0u8, 1u8, 0))
            .expect("CPU task can't communicate with scheduler thread.");
        // report data in hostmem
        tx.send((0u8, 0u8, hasher_task.local_nonces))
            .expect("CPU task can't communicate with scheduler thread.");
    }
}

#[cfg(test)]
mod test {
    extern crate crypto;
    use self::crypto::digest::Digest;
    use self::crypto::sha2::Sha256;
    use super::*;
    use crate::plotter;

    #[test]
    fn test_noncegen() {
        let numeric_id = 7900104405094198526;
        let start_nonce = 1337;
        let exp_result_hash = "eebdf7dce694cbea9539f71efc362d4b72f8792def335d7157dadb09bb6d9e5f";

        let check_result = |buf: &Vec<u8>| {
            let mut hasher = Sha256::new();
            hasher.input(buf);
            assert_eq!(hasher.result_str(), exp_result_hash);
        };

        if is_x86_feature_detected!("avx512f") {
            let mut buf = vec![0; 32 * plotter::NONCE_SIZE as usize];
            unsafe {
                init_shabal_avx512f();
                noncegen_avx512(
                    buf.as_mut_ptr() as *mut c_void,
                    32,
                    0,
                    numeric_id,
                    start_nonce,
                    32,
                );
            }
            check_result(&buf);
        }

        if is_x86_feature_detected!("avx2") {
            let mut buf = vec![0; 32 * plotter::NONCE_SIZE as usize];
            unsafe {
                init_shabal_avx2();
                noncegen_avx2(
                    buf.as_mut_ptr() as *mut c_void,
                    32,
                    0,
                    numeric_id,
                    start_nonce,
                    32,
                );
            }
            check_result(&buf);
        }

        if is_x86_feature_detected!("avx") {
            let mut buf = vec![0; 32 * plotter::NONCE_SIZE as usize];
            unsafe {
                init_shabal_avx();
                noncegen_avx(
                    buf.as_mut_ptr() as *mut c_void,
                    32,
                    0,
                    numeric_id,
                    start_nonce,
                    32,
                );
            }
            check_result(&buf);
        }

        if is_x86_feature_detected!("sse2") {
            let mut buf = vec![0; 32 * plotter::NONCE_SIZE as usize];
            unsafe {
                init_shabal_sse2();
                noncegen_sse2(
                    buf.as_mut_ptr() as *mut c_void,
                    32,
                    0,
                    numeric_id,
                    start_nonce,
                    32,
                );
            }
            check_result(&buf);
        }

        let mut buf = vec![0; 32 * plotter::NONCE_SIZE as usize];
        noncegen_rust(&mut buf, 0, numeric_id, start_nonce, 32);
        check_result(&buf);
    }
}
