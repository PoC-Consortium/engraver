extern crate fs2;
use std::fs::{File, OpenOptions};
use std::io;
use std::path::Path;

cfg_if! {
    if #[cfg(unix)] {
        #[cfg(linux)]
        extern crate thread_priority;
        use std::process::Command;
        use std::os::unix::fs::OpenOptionsExt;
        use utils::fs2::FileExt;
        #[cfg(linux)]
        use self::thread_priority::*;

        const O_DIRECT: i32 = 0o0_040_000;

        pub fn set_low_prio() {
            // todo: low prio for macos
            #[cfg(linux)]
            let thread_id = thread_native_id();
            #[cfg(linux)]
            set_thread_priority(
                thread_id,
                ThreadPriority::Min,
                ThreadSchedulePolicy::Normal(NormalThreadSchedulePolicy::Normal)
            ).unwrap();
        }

        pub fn open_using_direct_io<P: AsRef<Path>>(path: P) -> io::Result<File> {
            OpenOptions::new()
                .write(true)
                .create(true)
                .custom_flags(O_DIRECT)
                .open(path)
        }

        pub fn open<P: AsRef<Path>>(path: P) -> io::Result<File> {
            OpenOptions::new()
                .write(true)
                .create(true)
                .open(path)
        }

        pub fn open_r<P: AsRef<Path>>(path: P) -> io::Result<File> {
            OpenOptions::new()
                .read(true)
                .open(path)
        }
        // On unix, get the device id from 'df' command
        fn get_device_id_unix(path: &str) -> String {
            let output = Command::new("df")
                 .arg(path)
                 .output()
                 .expect("failed to execute 'df --output=source'");
             let source = String::from_utf8(output.stdout).expect("not utf8");
             source.split('\n').collect::<Vec<&str>>()[1].split(' ').collect::<Vec<&str>>()[0].to_string()
         }

        // On macos, use df and 'diskutil info <device>' to get the Device Block Size line
        // and extract the size
        fn get_sector_size_macos(path: &str) -> u64 {
            let source = get_device_id_unix(path);
            let output = Command::new("diskutil")
                .arg("info")
                .arg(source)
                .output()
                .expect("failed to execute 'diskutil info'");
            let source = String::from_utf8(output.stdout).expect("not utf8");
            let mut sector_size: u64 = 0;
            for line in source.split('\n').collect::<Vec<&str>>() {
                if line.trim().starts_with("Device Block Size") {
                    // e.g. in reverse: "Bytes 512 Size Block Device"
                    let source = line.rsplit(' ').collect::<Vec<&str>>()[1];

                    sector_size = source.parse::<u64>().unwrap();
                }
            }
            if sector_size == 0 {
                panic!("Abort: Unable to determine disk physical sector size from diskutil info")
            }
            sector_size
        }

        // On unix, use df and lsblk to extract the device sector size
        fn get_sector_size_unix(path: &str) -> u64 {
            let source = get_device_id_unix(path);
            let output = Command::new("lsblk")
                .arg(source)
                .arg("-o")
                .arg("PHY-SeC")
                .output()
                .expect("failed to execute 'lsblk -o LOG-SeC'");

            let sector_size = String::from_utf8(output.stdout).expect("not utf8");
            let sector_size = sector_size.split('\n').collect::<Vec<&str>>().get(1).unwrap_or_else(|| {
                println!("failed to determine sector size, defaulting to 4096.");
                &"4096"
            }).trim();

            sector_size.parse::<u64>().unwrap()
        }

        pub fn get_sector_size(path: &str) -> u64 {
            if cfg!(target_os = "macos") {
                get_sector_size_macos(path)
            } else {
                get_sector_size_unix(path)
            }
        }

        pub fn preallocate(file: &Path, size_in_bytes: u64, use_direct_io: bool) {
            let file = if use_direct_io {
                open_using_direct_io(&file)
            } else {
                open(&file)
            };
            let file = file.unwrap();
            file.allocate(size_in_bytes).unwrap();
        }

    } else {
        use std::ffi::CString;
        use std::ptr::null_mut;
        use std::iter::once;
        use std::ffi::OsStr;
        use std::os::windows::io::AsRawHandle;
        use std::os::windows::ffi::OsStrExt;
        use std::os::windows::fs::OpenOptionsExt;
        use core::mem::size_of_val;
        use winapi::um::errhandlingapi::GetLastError;
        use winapi::um::fileapi::{GetDiskFreeSpaceA,SetFileValidData};
        use winapi::um::handleapi::CloseHandle;
        use winapi::um::processthreadsapi::{SetThreadIdealProcessor,GetCurrentThread,OpenProcessToken,GetCurrentProcess,SetPriorityClass};
        use winapi::um::securitybaseapi::AdjustTokenPrivileges;
        use winapi::um::winbase::LookupPrivilegeValueW;
        use winapi::um::winnt::{LUID,TOKEN_ADJUST_PRIVILEGES,TOKEN_PRIVILEGES,LUID_AND_ATTRIBUTES,SE_PRIVILEGE_ENABLED,SE_MANAGE_VOLUME_NAME};

        const FILE_FLAG_NO_BUFFERING: u32 = 0x2000_0000;
        const FILE_FLAG_WRITE_THROUGH: u32 = 0x8000_0000;
        const BELOW_NORMAL_PRIORITY_CLASS: u32 = 0x0000_4000;

        pub fn open_using_direct_io<P: AsRef<Path>>(path: P) -> io::Result<File> {
            OpenOptions::new()
                .write(true)
                .create(true)
                .custom_flags(FILE_FLAG_NO_BUFFERING)
                .open(path)
        }

        pub fn open<P: AsRef<Path>>(path: P) -> io::Result<File> {
            OpenOptions::new()
                .write(true)
                .create(true)
                .custom_flags(FILE_FLAG_WRITE_THROUGH)
                .open(path)
        }

        pub fn open_r<P: AsRef<Path>>(path: P) -> io::Result<File> {
            OpenOptions::new()
                .read(true)
                .open(path)
        }

        pub fn preallocate(file: &Path, size_in_bytes: u64, use_direct_io: bool) {
            let mut result = true;
            result &= obtain_priviledge();

            let file = if use_direct_io {
                open_using_direct_io(&file)
            } else {
                open(&file)
            };
            let file = file.unwrap();

            file.set_len(size_in_bytes).unwrap();

            if result {
                let handle = file.as_raw_handle();
                unsafe{
                    let temp = SetFileValidData(handle, size_in_bytes as i64);
                    result &= temp == 1;
                }
            }

            if !result {
                println!("FAILED, administrative rights missing");
                print!("Slow file pre-allocation...");
            }
        }

        pub fn obtain_priviledge() -> bool {
            let mut result = true;

            let privilege_encoded: Vec<u16> = OsStr::new(SE_MANAGE_VOLUME_NAME)
                .encode_wide()
                .chain(once(0))
                .collect();

            let luid = LUID{
                HighPart: 0i32,
                LowPart: 0u32

            };

            unsafe {
                let mut htoken = null_mut();
                let mut tp = TOKEN_PRIVILEGES{
                    PrivilegeCount: 1,
                    Privileges: [LUID_AND_ATTRIBUTES{
                    Luid: luid,
                    Attributes: SE_PRIVILEGE_ENABLED,
                    }]
                };

                let temp = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &mut htoken);
                 result &= temp == 1;

                let temp = LookupPrivilegeValueW(null_mut(), privilege_encoded.as_ptr(), &mut tp.Privileges[0].Luid);
                result &= temp == 1;

                let temp = AdjustTokenPrivileges(htoken, 0, &mut tp, size_of_val(&tp) as u32, null_mut(), null_mut());

                CloseHandle(htoken);
                result &= temp == 1;
                result &=
                    GetLastError() == 0u32
            }
            result
        }

        pub fn get_sector_size(path: &str) -> u64 {
            let path_encoded = Path::new(path);
            let parent_path_encoded = CString::new(path_encoded.to_str().unwrap()).unwrap();
            let mut sectors_per_cluster  = 0u32;
            let mut bytes_per_sector  = 0u32;
            let mut number_of_free_cluster  = 0u32;
            let mut total_number_of_cluster  = 0u32;
            if unsafe {
                GetDiskFreeSpaceA(
                    parent_path_encoded.as_ptr(),
                    &mut sectors_per_cluster,
                    &mut bytes_per_sector,
                    &mut number_of_free_cluster,
                    &mut total_number_of_cluster
                )
            } == 0  {
                panic!("get sector size, filename={}",path);
            };
            u64::from(bytes_per_sector)
        }

        pub fn set_thread_ideal_processor(id: usize){
            // Set core affinity for current thread.
        unsafe {
            SetThreadIdealProcessor(
                GetCurrentThread(),
                id as u32
            );
            }
        }
        pub fn set_low_prio() {
            unsafe{
                SetPriorityClass(GetCurrentProcess(),BELOW_NORMAL_PRIORITY_CLASS);
            }
        }

    }
}

pub fn free_disk_space(path: &str) -> u64 {
    fs2::free_space(Path::new(&path)).unwrap()
}
