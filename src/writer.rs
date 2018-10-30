extern crate pbr;

use chan;
use plotter::{Buffer, PlotterTask};
use std::cmp::min;
use std::io::{Read, Seek, SeekFrom, Write};
use std::path::Path;
use std::sync::Arc;
use utils::{open, open_r, open_using_direct_io};

const TASK_SIZE: u64 = 16384;
const SCOOP_SIZE: u64 = 64;
const NONCE_SIZE: u64 = 4096 * SCOOP_SIZE;

pub fn create_writer_task(
    task: Arc<PlotterTask>,
    mut nonces_written: u64,
    mut pb: Option<pbr::ProgressBar<pbr::Pipe>>,
    rx_buffers_to_writer: chan::Receiver<Buffer>,
    tx_empty_buffers: chan::Sender<Buffer>,
) -> impl FnOnce() {
    move || {
        for buffer in rx_buffers_to_writer {
            let mut_bs = &buffer.get_buffer();
            let mut bs = mut_bs.lock().unwrap();
            let buffer_size = (*bs).len() as u64;
            let nonces_to_write = min(buffer_size / NONCE_SIZE, task.nonces - nonces_written);

            let filename = Path::new(&task.output_path).join(format!(
                "{}_{}_{}",
                task.numeric_id, task.start_nonce, task.nonces
            ));

            let file = if task.direct_io {
                open_using_direct_io(&filename)
            } else {
                open(&filename)
            };

            let mut file = file.unwrap();

            for scoop in 0..4096 {
                let mut seek_addr = scoop * task.nonces as u64 * SCOOP_SIZE;
                seek_addr += nonces_written as u64 * SCOOP_SIZE;

                file.seek(SeekFrom::Start(seek_addr)).unwrap();

                let mut local_addr = scoop * buffer_size / NONCE_SIZE * SCOOP_SIZE;
                for _ in 0..nonces_to_write / TASK_SIZE {
                    file.write_all(
                        &bs[local_addr as usize..(local_addr + TASK_SIZE * SCOOP_SIZE) as usize],
                    ).unwrap();

                    local_addr += TASK_SIZE * SCOOP_SIZE;
                }

                // write remainder
                if nonces_to_write % TASK_SIZE > 0 {
                    file.write_all(
                        &bs[local_addr as usize
                                ..(local_addr + (nonces_to_write % TASK_SIZE) * SCOOP_SIZE)
                                    as usize],
                    ).unwrap();
                }

                if (scoop + 1) % 128 == 0 {
                    match &mut pb {
                        Some(pb) => {
                            pb.add(nonces_to_write * SCOOP_SIZE * 128);
                        }
                        None => (),
                    }
                }
            }

            nonces_written += nonces_to_write;

            // thread end
            if task.nonces == nonces_written {
                match &mut pb {
                    Some(pb) => {
                        pb.finish_print("Writer done.");
                    }
                    None => (),
                }
                break;
            }

            write_resume_info(&filename, nonces_written);
            tx_empty_buffers.send(buffer);
        }
    }
}

pub fn read_resume_info(file: &Path) -> u64 {
    let mut file = open_r(&file).unwrap();
    file.seek(SeekFrom::End(-8)).unwrap();

    let mut progress = [0u8; 4];
    let mut double_monkey = [0u8; 4];

    file.read_exact(&mut progress[0..4]).unwrap();
    file.read_exact(&mut double_monkey[0..4]).unwrap();

    if double_monkey == [0xAF, 0xFE, 0xAF, 0xFE] {
        u64::from(as_u32_le(progress))
    } else {
        0
    }
}

pub fn write_resume_info(file: &Path, nonces_written: u64) {
    let mut file = open(&file).unwrap();
    file.seek(SeekFrom::End(-8)).unwrap();

    let progress = as_u8_le(nonces_written as u32);
    let double_monkey = [0xAF, 0xFE, 0xAF, 0xFE];

    file.write_all(&progress[0..4]).unwrap();
    file.write_all(&double_monkey[0..4]).unwrap();
}

fn as_u32_le(array: [u8; 4]) -> u32 {
    u32::from(array[0])
        + (u32::from(array[1]) << 8)
        + (u32::from(array[2]) << 16)
        + (u32::from(array[3]) << 24)
}

fn as_u8_le(x: u32) -> [u8; 4] {
    let b1: u8 = (x & 0xff) as u8;
    let b2: u8 = ((x >> 8) & 0xff) as u8;
    let b3: u8 = ((x >> 16) & 0xff) as u8;
    let b4: u8 = ((x >> 24) & 0xff) as u8;
    [b1, b2, b3, b4]
}
