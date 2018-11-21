extern crate ocl_core as core;
extern crate rayon;

use self::core::{
    ArgVal, ContextProperties, DeviceInfo, Event, KernelWorkGroupInfo, PlatformInfo, Status,
};
use gpu_hasher::GpuTask;
use ocl::rayon::prelude::*;
use std::ffi::CString;
use std::process;
use std::slice::{from_raw_parts, from_raw_parts_mut};
use std::sync::{Arc, Mutex};
use std::u64;

static SRC: &'static str = include_str!("ocl/kernel.cl");

const NONCE_SIZE: u64 = (2 << 17);
const NUM_SCOOPS: u64 = 4096;
const GPU_HASHES_PER_RUN: usize = 32;
const MSHABAL512_VECTOR_SIZE: u64 = 16;
const SCOOP_SIZE: u64 = 64;

// convert the info or error to a string for printing:
macro_rules! to_string {
    ($expr:expr) => {
        match $expr {
            Ok(info) => info.to_string(),
            Err(err) => match err.api_status() {
                Some(Status::CL_KERNEL_ARG_INFO_NOT_AVAILABLE) => "Not available".into(),
                _ => err.to_string(),
            },
        }
    };
}

pub struct GpuContext {
    queue_a: core::CommandQueue,
    queue_b: core::CommandQueue,
    kernel: core::Kernel,
    ldim1: [usize; 3],
    gdim1: [usize; 3],
    mapping: bool,
    buffer_ptr_host_a: Option<core::MemMap<u8>>,
    buffer_ptr_host_b: Option<core::MemMap<u8>>,
    buffer_gpu_a: core::Mem,
    buffer_gpu_b: core::Mem,
    pub worksize: usize,
}

// Ohne Gummi im Bahnhofsviertel... das wird noch Konsequenzen haben
unsafe impl Sync for GpuContext {}

impl GpuContext {
    pub fn new(
        gpu_platform: usize,
        gpu_id: usize,
        max_nonces_per_cache: usize,
        mapping: bool,
    ) -> GpuContext {
        let platform_ids = core::get_platform_ids().unwrap();
        let platform_id = platform_ids[gpu_platform];
        let device_ids = core::get_device_ids(&platform_id, None, None).unwrap();
        let device_id = device_ids[gpu_id];
        let context_properties = ContextProperties::new().platform(platform_id);
        let context =
            core::create_context(Some(&context_properties), &[device_id], None, None).unwrap();
        let src_cstring = CString::new(SRC).unwrap();
        let program = core::create_program_with_source(&context, &[src_cstring]).unwrap();
        core::build_program(
            &program,
            None::<&[()]>,
            &CString::new("").unwrap(),
            None,
            None,
        ).unwrap();
        let queue_a = core::create_command_queue(&context, &device_id, None).unwrap();
        let queue_b = core::create_command_queue(&context, &device_id, None).unwrap();
        let kernel = core::create_kernel(&program, "calculate_nonces").unwrap();
        let kernel_workgroup_size = get_kernel_work_group_size(&kernel, device_id);
        let workgroup_count = max_nonces_per_cache / kernel_workgroup_size;
        let worksize = kernel_workgroup_size * workgroup_count;
        let gdim1 = [worksize, 1, 1];
        let ldim1 = [kernel_workgroup_size, 1, 1];

        // create buffers
        // mapping = zero copy buffers, no mapping = pinned memory for fast DMA.
        if mapping {
            let buffer_gpu_a = unsafe {
                core::create_buffer::<_, u8>(
                    &context,
                    core::MEM_READ_WRITE | core::MEM_ALLOC_HOST_PTR,
                    (NONCE_SIZE as usize) * worksize,
                    None,
                ).unwrap()
            };
            let buffer_gpu_b = unsafe {
                core::create_buffer::<_, u8>(
                    &context,
                    core::MEM_READ_WRITE | core::MEM_ALLOC_HOST_PTR,
                    (NONCE_SIZE as usize) * worksize,
                    None,
                ).unwrap()
            };
            GpuContext {
                queue_a,
                queue_b,
                kernel,
                ldim1,
                gdim1,
                mapping,
                buffer_gpu_a,
                buffer_gpu_b,
                buffer_ptr_host_a: None,
                buffer_ptr_host_b: None,
                worksize,
            }
        } else {
            let buffer_gpu_a = unsafe {
                core::create_buffer::<_, u8>(
                    &context,
                    core::MEM_READ_WRITE | core::MEM_ALLOC_HOST_PTR,
                    (NONCE_SIZE as usize) * worksize,
                    None,
                ).unwrap()
            };
            let buffer_gpu_b = unsafe {
                core::create_buffer::<_, u8>(
                    &context,
                    core::MEM_READ_WRITE | core::MEM_ALLOC_HOST_PTR,
                    (NONCE_SIZE as usize) * worksize,
                    None,
                ).unwrap()
            };
            let buffer_ptr_host_a = unsafe {
                Some(
                    core::enqueue_map_buffer::<u8, _, _, _>(
                        &queue_b,
                        &buffer_gpu_a,
                        false,
                        core::MAP_READ,
                        0,
                        gdim1[0] * NONCE_SIZE as usize,
                        None::<Event>,
                        None::<&mut Event>,
                    ).unwrap(),
                )
            };
            let buffer_ptr_host_b = unsafe {
                Some(
                    core::enqueue_map_buffer::<u8, _, _, _>(
                        &queue_b,
                        &buffer_gpu_b,
                        false,
                        core::MAP_READ,
                        0,
                        gdim1[0] * NONCE_SIZE as usize,
                        None::<Event>,
                        None::<&mut Event>,
                    ).unwrap(),
                )
            };
            GpuContext {
                queue_a,
                queue_b,
                kernel,
                ldim1,
                gdim1,
                mapping,
                buffer_gpu_a,
                buffer_gpu_b,
                buffer_ptr_host_a,
                buffer_ptr_host_b,
                worksize,
            }
        }
    }
}

pub fn platform_info() {
    let platform_ids = core::get_platform_ids().unwrap();
    for (i, platform_id) in platform_ids.iter().enumerate() {
        println!(
            "OCL: platform {}, {} - {}",
            i,
            to_string!(core::get_platform_info(&platform_id, PlatformInfo::Name)),
            to_string!(core::get_platform_info(&platform_id, PlatformInfo::Version))
        );
        let device_ids = core::get_device_ids(&platform_id, None, None).unwrap();
        for (j, device_id) in device_ids.iter().enumerate() {
            println!(
                "OCL: device {}, {} - {}",
                j,
                to_string!(core::get_device_info(device_id, DeviceInfo::Vendor)),
                to_string!(core::get_device_info(device_id, DeviceInfo::Name))
            );
        }
    }
}

pub fn gpu_show_info(gpus: &[String]) {
    for gpu in gpus.iter() {
        let gpu = gpu.split(':').collect::<Vec<&str>>();
        let platform_id = gpu[0].parse::<usize>().unwrap();
        let gpu_id = gpu[1].parse::<usize>().unwrap();

        let platform_ids = core::get_platform_ids().unwrap();
        if platform_id >= platform_ids.len() {
            println!("Error: Selected OpenCL platform doesn't exist.");
            println!("Shutting down...");
            process::exit(0);
        }
        let platform = platform_ids[platform_id];
        let device_ids = core::get_device_ids(&platform, None, None).unwrap();
        if gpu_id >= device_ids.len() {
            println!("Error: Selected OpenCL device doesn't exist");
            println!("Shutting down...");
            process::exit(0);
        }
        let device = device_ids[gpu_id];
        match core::get_device_info(&device, DeviceInfo::GlobalMemSize).unwrap() {
            core::DeviceInfoResult::GlobalMemSize(mem) => {
                println!(
                    "GPU: {} - {} [RAM={}MiB, Cores={}]",
                    to_string!(core::get_device_info(&device, DeviceInfo::Vendor)),
                    to_string!(core::get_device_info(&device, DeviceInfo::Name)),
                    mem / 1024 / 1024,
                    to_string!(core::get_device_info(&device, DeviceInfo::MaxComputeUnits))
                );
            }
            _ => panic!("Unexpected error. Can't obtain GPU memory size."),
        }
    }
}

pub fn gpu_init(gpus: &[String], zcb: bool) -> Vec<Arc<Mutex<GpuContext>>> {
    let mut result = Vec::new();
    for gpu in gpus.iter() {
        let gpu = gpu.split(':').collect::<Vec<&str>>();
        let platform_id = gpu[0].parse::<usize>().unwrap();
        let gpu_id = gpu[1].parse::<usize>().unwrap();

        let platform_ids = core::get_platform_ids().unwrap();
        if platform_id >= platform_ids.len() {
            println!("Error: Selected OpenCL platform doesn't exist.");
            println!("Shutting down...");
            process::exit(0);
        }
        let platform = platform_ids[platform_id];
        let device_ids = core::get_device_ids(&platform, None, None).unwrap();
        if gpu_id >= device_ids.len() {
            println!("Error: Selected OpenCL device doesn't exist");
            println!("Shutting down...");
            process::exit(0);
        }
        let device = device_ids[gpu_id];
        let mut total_mem = match core::get_device_info(&device, DeviceInfo::GlobalMemSize).unwrap()
        {
            core::DeviceInfoResult::GlobalMemSize(mem) => mem,
            _ => panic!("Unexpected error. Can't obtain GPU memory size."),
        };

        // use max 25% of total gpu mem
        // todo: user limit
        let num_buffer = 2;
        let max_nonces = ((total_mem / 8 * 2) / (num_buffer * NONCE_SIZE)) as usize;
        result.push(Arc::new(Mutex::new(GpuContext::new(
            platform_id,
            gpu_id,
            max_nonces,
            zcb,
        ))));
    }
    result
}

fn get_kernel_work_group_size(x: &core::Kernel, y: core::DeviceId) -> usize {
    match core::get_kernel_work_group_info(x, y, KernelWorkGroupInfo::WorkGroupSize).unwrap() {
        core::KernelWorkGroupInfoResult::WorkGroupSize(kws) => kws,
        _ => panic!("Unexpected error"),
    }
}

pub fn gpu_hash(gpu_context: &Arc<Mutex<GpuContext>>, task: &GpuTask) {
    let numeric_id_be: u64 = task.numeric_id.to_be();

    let mut start;
    let mut end;
    let gpu_context = gpu_context.lock().unwrap();

    core::set_kernel_arg(
        &gpu_context.kernel,
        0,
        ArgVal::mem(&gpu_context.buffer_gpu_a),
    ).unwrap();
    core::set_kernel_arg(
        &gpu_context.kernel,
        1,
        ArgVal::primitive(&task.local_startnonce),
    ).unwrap();
    core::set_kernel_arg(
        &gpu_context.kernel,
        5,
        ArgVal::primitive(&task.local_nonces),
    ).unwrap();
    core::set_kernel_arg(&gpu_context.kernel, 2, ArgVal::primitive(&numeric_id_be)).unwrap();

    for i in (0..8192).step_by(GPU_HASHES_PER_RUN) {
        if i + GPU_HASHES_PER_RUN < 8192 {
            start = i;
            end = i + GPU_HASHES_PER_RUN - 1;
        } else {
            start = i;
            end = i + GPU_HASHES_PER_RUN;
        }

        core::set_kernel_arg(&gpu_context.kernel, 3, ArgVal::primitive(&(start as i32))).unwrap();
        core::set_kernel_arg(&gpu_context.kernel, 4, ArgVal::primitive(&(end as i32))).unwrap();

        unsafe {
            core::enqueue_kernel(
                &gpu_context.queue_a,
                &gpu_context.kernel,
                1,
                None,
                &gpu_context.gdim1,
                Some(gpu_context.ldim1),
                None::<Event>,
                None::<&mut Event>,
            ).unwrap();
        }
    }
    core::finish(&gpu_context.queue_a).unwrap();
}

pub fn gpu_transfer_to_host(
    gpu_context: &Arc<Mutex<GpuContext>>,
    buffer_id: u8,
    transfer_task: &GpuTask,
) {
    let mut gpu_context = gpu_context.lock().unwrap();

    // get mem mapping
    let map = if gpu_context.mapping {
        Some(
            // map to host (zero copy buffer)
            unsafe {
                if buffer_id == 1 {
                    core::enqueue_map_buffer::<u8, _, _, _>(
                        &gpu_context.queue_b,
                        &gpu_context.buffer_gpu_a,
                        true,
                        core::MAP_READ,
                        0,
                        gpu_context.gdim1[0] * NONCE_SIZE as usize,
                        None::<Event>,
                        None::<&mut Event>,
                    ).unwrap()
                } else {
                    core::enqueue_map_buffer::<u8, _, _, _>(
                        &gpu_context.queue_b,
                        &gpu_context.buffer_gpu_b,
                        true,
                        core::MAP_READ,
                        0,
                        gpu_context.gdim1[0] * NONCE_SIZE as usize,
                        None::<Event>,
                        None::<&mut Event>,
                    ).unwrap()
                }
            },
        )
    } else {
        None
    };

    let buffer = if gpu_context.mapping {
        // map to host (zero copy buffer)
        map.as_ref().unwrap().as_ptr()
    } else {
        // get pointer
        let ptr = if buffer_id == 1 {
            gpu_context.buffer_ptr_host_a.as_mut().unwrap().as_mut_ptr()
        } else {
            gpu_context.buffer_ptr_host_b.as_mut().unwrap().as_mut_ptr()
        };
        // copy to host
        let slice = unsafe { from_raw_parts_mut(ptr, gpu_context.worksize * NONCE_SIZE as usize) };
        unsafe {
            if buffer_id == 1 {
                core::enqueue_read_buffer(
                    &gpu_context.queue_b,
                    &gpu_context.buffer_gpu_a,
                    true,
                    0,
                    slice,
                    None::<Event>,
                    None::<&mut Event>,
                ).unwrap();
            } else {
                core::enqueue_read_buffer(
                    &gpu_context.queue_b,
                    &gpu_context.buffer_gpu_b,
                    true,
                    0,
                    slice,
                    None::<Event>,
                    None::<&mut Event>,
                ).unwrap();
            }
        }
        ptr
    };

    // simd shabal words unpack + POC Shuffle + scatter nonces into optimised cache
    unsafe {
        let buffer = from_raw_parts(buffer, gpu_context.worksize * NONCE_SIZE as usize);
        let iter: Vec<u64> = (0..transfer_task.local_nonces).step_by(16).collect();
        iter.par_iter().for_each(|n| {
            // get global buffer
            let data = from_raw_parts_mut(
                transfer_task.cache.ptr,
                NONCE_SIZE as usize * transfer_task.cache_size as usize,
            );
            for i in 0..(NUM_SCOOPS * 2) {
                for j in (0..32).step_by(4) {
                    for k in 0..MSHABAL512_VECTOR_SIZE {
                        let data_offset = (((i & 1) * (4095 - (i >> 1)) + ((i + 1) & 1) * (i >> 1))
                            * SCOOP_SIZE
                            * transfer_task.cache_size
                            + (*n + k + transfer_task.chunk_offset) * SCOOP_SIZE
                            + (i & 1) * 32
                            + j) as usize;
                        let buffer_offset = (*n * NONCE_SIZE
                            + (i * 32 + j) * MSHABAL512_VECTOR_SIZE
                            + k * 4) as usize;
                        data[data_offset..(data_offset + 4)]
                            .clone_from_slice(&buffer[buffer_offset..(buffer_offset + 4)]);
                    }
                }
            }
        })
    }
    // unmap
    if gpu_context.mapping {
        // map to host (zero copy buffer)
        if buffer_id == 1 {
            core::enqueue_unmap_mem_object(
                &gpu_context.queue_a,
                &gpu_context.buffer_gpu_a,
                &map.unwrap(),
                None::<Event>,
                None::<&mut Event>,
            ).unwrap()
        } else {
            core::enqueue_unmap_mem_object(
                &gpu_context.queue_a,
                &gpu_context.buffer_gpu_b,
                &map.unwrap(),
                None::<Event>,
                None::<&mut Event>,
            ).unwrap()
        };
        core::finish(&gpu_context.queue_a).unwrap();
    }
}

pub fn gpu_hash_and_transfer_to_host(
    gpu_context: &Arc<Mutex<GpuContext>>,
    buffer_id: u8,
    hasher_task: &GpuTask,
    transfer_task: &GpuTask,
) {
    let mut gpu_context = gpu_context.lock().unwrap();

    // get mem mapping
    let map = if gpu_context.mapping {
        Some(
            // map to host (zero copy buffer)
            unsafe {
                if buffer_id == 1 {
                    core::enqueue_map_buffer::<u8, _, _, _>(
                        &gpu_context.queue_b,
                        &gpu_context.buffer_gpu_a,
                        true,
                        core::MAP_READ,
                        0,
                        gpu_context.gdim1[0] * NONCE_SIZE as usize,
                        None::<Event>,
                        None::<&mut Event>,
                    ).unwrap()
                } else {
                    core::enqueue_map_buffer::<u8, _, _, _>(
                        &gpu_context.queue_b,
                        &gpu_context.buffer_gpu_b,
                        true,
                        core::MAP_READ,
                        0,
                        gpu_context.gdim1[0] * NONCE_SIZE as usize,
                        None::<Event>,
                        None::<&mut Event>,
                    ).unwrap()
                }
            },
        )
    } else {
        None
    };

    let buffer = if gpu_context.mapping {
        // map to host (zero copy buffer)
        map.as_ref().unwrap().as_ptr()
    } else {
        // get pointer
        let ptr = if buffer_id == 1 {
            gpu_context.buffer_ptr_host_a.as_mut().unwrap().as_mut_ptr()
        } else {
            gpu_context.buffer_ptr_host_b.as_mut().unwrap().as_mut_ptr()
        };
        // copy to host
        let slice = unsafe { from_raw_parts_mut(ptr, gpu_context.worksize * NONCE_SIZE as usize) };
        unsafe {
            if buffer_id == 1 {
                core::enqueue_read_buffer(
                    &gpu_context.queue_b,
                    &gpu_context.buffer_gpu_a,
                    true,
                    0,
                    slice,
                    None::<Event>,
                    None::<&mut Event>,
                ).unwrap();
            } else {
                core::enqueue_read_buffer(
                    &gpu_context.queue_b,
                    &gpu_context.buffer_gpu_b,
                    true,
                    0,
                    slice,
                    None::<Event>,
                    None::<&mut Event>,
                ).unwrap();
            }
        }
        ptr
    };

    let numeric_id_be: u64 = hasher_task.numeric_id.to_be();

    let mut start;
    let mut end;

    core::set_kernel_arg(
        &gpu_context.kernel,
        0,
        ArgVal::mem(if buffer_id == 0 {
            &gpu_context.buffer_gpu_a
        } else {
            &gpu_context.buffer_gpu_b
        }),
    ).unwrap();
    core::set_kernel_arg(
        &gpu_context.kernel,
        1,
        ArgVal::primitive(&hasher_task.local_startnonce),
    ).unwrap();
    core::set_kernel_arg(
        &gpu_context.kernel,
        5,
        ArgVal::primitive(&hasher_task.local_nonces),
    ).unwrap();
    core::set_kernel_arg(&gpu_context.kernel, 2, ArgVal::primitive(&numeric_id_be)).unwrap();

    for i in (0..8192).step_by(GPU_HASHES_PER_RUN) {
        if i + GPU_HASHES_PER_RUN < 8192 {
            start = i;
            end = i + GPU_HASHES_PER_RUN - 1;
        } else {
            start = i;
            end = i + GPU_HASHES_PER_RUN;
        }
        core::set_kernel_arg(&gpu_context.kernel, 3, ArgVal::primitive(&(start as i32))).unwrap();
        core::set_kernel_arg(&gpu_context.kernel, 4, ArgVal::primitive(&(end as i32))).unwrap();
        unsafe {
            core::enqueue_kernel(
                &gpu_context.queue_a,
                &gpu_context.kernel,
                1,
                None,
                &gpu_context.gdim1,
                Some(gpu_context.ldim1),
                None::<Event>,
                None::<&mut Event>,
            ).unwrap();
        }
    }
    core::finish(&gpu_context.queue_b).unwrap();

    // simd shabal words unpack + POC Shuffle + scatter nonces into optimised cache
    // todo remove duplicate code parts
    unsafe {
        let buffer = from_raw_parts(buffer, gpu_context.worksize * NONCE_SIZE as usize);
        let iter: Vec<u64> = (0..transfer_task.local_nonces).step_by(16).collect();
        iter.par_iter().for_each(|n| {
            // get global buffer
            let data = from_raw_parts_mut(
                transfer_task.cache.ptr,
                NONCE_SIZE as usize * transfer_task.cache_size as usize,
            );
            for i in 0..(NUM_SCOOPS * 2) {
                for j in (0..32).step_by(4) {
                    for k in 0..MSHABAL512_VECTOR_SIZE {
                        let data_offset = (((i & 1) * (4095 - (i >> 1)) + ((i + 1) & 1) * (i >> 1))
                            * SCOOP_SIZE
                            * transfer_task.cache_size
                            + (*n + k + transfer_task.chunk_offset) * SCOOP_SIZE
                            + (i & 1) * 32
                            + j) as usize;

                        let buffer_offset = (*n * NONCE_SIZE
                            + (i * 32 + j) * MSHABAL512_VECTOR_SIZE
                            + k * 4) as usize;
                        data[data_offset..(data_offset + 4)]
                            .clone_from_slice(&buffer[buffer_offset..(buffer_offset + 4)]);
                    }
                }
            }
        })
    }
    // unmap
    if gpu_context.mapping {
        // map to host (zero copy buffer)
        if buffer_id == 1 {
            core::enqueue_unmap_mem_object(
                &gpu_context.queue_a,
                &gpu_context.buffer_gpu_a,
                &map.unwrap(),
                None::<Event>,
                None::<&mut Event>,
            ).unwrap()
        } else {
            core::enqueue_unmap_mem_object(
                &gpu_context.queue_a,
                &gpu_context.buffer_gpu_b,
                &map.unwrap(),
                None::<Event>,
                None::<&mut Event>,
            ).unwrap()
        };
    }
    core::finish(&gpu_context.queue_a).unwrap();
}
