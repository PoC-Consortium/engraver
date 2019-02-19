use libc::{c_void, size_t, uint64_t};
use std::sync::mpsc::Sender;

extern "C" {
    pub fn noncegen(
        cache: *mut c_void,
        cache_size: size_t,
        chunk_offset: size_t,
        numeric_ID: uint64_t,
        local_startnonce: uint64_t,
        local_nonces: uint64_t,
    );
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
pub struct SafeCVoid {
    pub ptr: *mut c_void,
}
unsafe impl Send for SafeCVoid {}

pub struct CpuTask {
    pub cache: SafeCVoid,
    pub cache_size: size_t,
    pub chunk_offset: size_t,
    pub numeric_id: uint64_t,
    pub local_startnonce: uint64_t,
    pub local_nonces: uint64_t,
}

pub fn hash_cpu(
    tx: Sender<(u8, u8, u64)>,
    hasher_task: CpuTask,
    simd_ext: String,
) -> impl FnOnce() {
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
                "SSE2" => noncegen_sse2(
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
        let exp_result_hash = "8f4ea236553ce1b036986313d9fc879ca8f67ba6b3c711b711e9090c9086313d";

        let check_result = |buf: &Vec<u8>| {
            let mut hasher = Sha256::new();
            hasher.input(buf);
            assert_eq!(hasher.result_str(), exp_result_hash);
        };

        if is_x86_feature_detected!("avx512f") {
            let mut buf = vec![0; 64 * plotter::NONCE_SIZE as usize];
            unsafe {
                plotter::init_shabal_avx512();
                noncegen_avx512(
                    buf.as_mut_ptr() as *mut c_void,
                    0,
                    0,
                    numeric_id,
                    start_nonce,
                    64,
                );
            }
            check_result(&buf);
        }

        if is_x86_feature_detected!("avx2") {
            let mut buf = vec![0; 64 * plotter::NONCE_SIZE as usize];
            unsafe {
                plotter::init_shabal_avx2();
                noncegen_avx2(
                    buf.as_mut_ptr() as *mut c_void,
                    0,
                    0,
                    numeric_id,
                    start_nonce,
                    64,
                );
            }
            check_result(&buf);
        }

        if is_x86_feature_detected!("avx") {
            let mut buf = vec![0; 64 * plotter::NONCE_SIZE as usize];
            unsafe {
                plotter::init_shabal_avx();
                noncegen_avx(
                    buf.as_mut_ptr() as *mut c_void,
                    0,
                    0,
                    numeric_id,
                    start_nonce,
                    64,
                );
            }
            check_result(&buf);
        }

        if is_x86_feature_detected!("sse2") {
            let mut buf = vec![0; 64 * plotter::NONCE_SIZE as usize];
            unsafe {
                plotter::init_shabal_sse2();
                noncegen_sse2(
                    buf.as_mut_ptr() as *mut c_void,
                    0,
                    0,
                    numeric_id,
                    start_nonce,
                    64,
                );
            }
            check_result(&buf);
        }

        let mut buf = vec![0; 64 * plotter::NONCE_SIZE as usize];
        unsafe {
            noncegen(
                buf.as_mut_ptr() as *mut c_void,
                0,
                0,
                numeric_id,
                start_nonce,
                64,
            );
        }
        check_result(&buf);
    }
}
