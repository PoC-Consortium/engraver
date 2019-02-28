use crate::shabal256::shabal256_fast;

const HASH_SIZE: usize = 32;
const HASH_CAP: usize = 4096;
const NUM_SCOOPS: usize = 4096;
const SCOOP_SIZE: usize = 64;
const NONCE_SIZE: usize = (NUM_SCOOPS * SCOOP_SIZE);
const MESSAGE_SIZE: usize = 16;

// cache:		    cache to save to
// local_num:		thread number
// numeric_id:		numeric account id
// loc_startnonce	nonce to start generation at
// local_nonces: 	number of nonces to generate
pub fn noncegen_rust(
    cache: &mut [u8],
    cache_offset: usize,
    numeric_id: u64,
    local_startnonce: u64,
    local_nonces: u64,
) {
    let numeric_id: [u32; 2] = unsafe { std::mem::transmute(numeric_id.to_be()) };

    let mut buffer = [0u8; NONCE_SIZE];
    let mut final_buffer = [0u8; HASH_SIZE];

    // prepare termination strings
    let mut t1 = [0u32; MESSAGE_SIZE];
    t1[0..2].clone_from_slice(&numeric_id);
    t1[4] = 0x80;

    let mut t2 = [0u32; MESSAGE_SIZE];
    t2[8..10].clone_from_slice(&numeric_id);
    t2[12] = 0x80;

    let mut t3 = [0u32; MESSAGE_SIZE];
    t3[0] = 0x80;

    for n in 0..local_nonces {
        // generate nonce numbers & change endianness
        let nonce: [u32; 2] = unsafe { std::mem::transmute((local_startnonce + n).to_be()) };

        // store nonce numbers in relevant termination strings
        t1[2..4].clone_from_slice(&nonce);
        t2[10..12].clone_from_slice(&nonce);

        // start shabal rounds

        // 3 cases: first 128 rounds uses case 1 or 2, after that case 3
        // case 1: first 128 rounds, hashes are even: use termination string 1
        // case 2: first 128 rounds, hashes are odd: use termination string 2
        // case 3: round > 128: use termination string 3
        // round 1
        let hash = shabal256_fast(&[], &t1);

        buffer[NONCE_SIZE - HASH_SIZE..NONCE_SIZE].clone_from_slice(&hash);
        let hash = unsafe { std::mem::transmute::<[u8; 32], [u32; 8]>(hash) };

        // store first hash into smart termination string 2
        t2[0..8].clone_from_slice(&hash);
        // round 2 - 128
        for i in (NONCE_SIZE - HASH_CAP + HASH_SIZE..=NONCE_SIZE - HASH_SIZE)
            .rev()
            .step_by(HASH_SIZE)
        {
            // check if msg can be divided into 512bit packages without a
            // remainder
            if i % 64 == 0 {
                // last msg = seed + termination
                let hash = &shabal256_fast(&buffer[i..NONCE_SIZE], &t1);
                buffer[i - HASH_SIZE..i].clone_from_slice(hash);
            } else {
                // last msg = 256 bit data + seed + termination
                let hash = &shabal256_fast(&buffer[i..NONCE_SIZE], &t2);
                buffer[i - HASH_SIZE..i].clone_from_slice(hash);
            }
        }

        // round 128-8192
        for i in (HASH_SIZE..=NONCE_SIZE - HASH_CAP).rev().step_by(HASH_SIZE) {
            let hash = &shabal256_fast(&buffer[i..i + HASH_CAP], &t3);
            buffer[i - HASH_SIZE..i].clone_from_slice(hash);
        }

        // generate final hash
        final_buffer.clone_from_slice(&shabal256_fast(&buffer[0..NONCE_SIZE], &t1));

        // XOR with final
        for i in 0..NONCE_SIZE {
            buffer[i] ^= final_buffer[i % HASH_SIZE];
        }

        // PoC2 shuffle
        let cache_size = cache.len() / NONCE_SIZE;
        for i in 0..NUM_SCOOPS {
            let offset = i * cache_size * SCOOP_SIZE + (n as usize + cache_offset) * SCOOP_SIZE;
            cache[offset..offset + HASH_SIZE]
                .clone_from_slice(&buffer[i * SCOOP_SIZE..i * SCOOP_SIZE + HASH_SIZE]);
            let mirror_offset = (4095 - i) * cache_size * SCOOP_SIZE
                + (n as usize + cache_offset) * SCOOP_SIZE
                + HASH_SIZE;
            cache[mirror_offset..mirror_offset + HASH_SIZE].clone_from_slice(
                &buffer[i * SCOOP_SIZE + HASH_SIZE..i * SCOOP_SIZE + 2 * HASH_SIZE],
            );
        }
    }
}
