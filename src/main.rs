#[macro_use]
extern crate clap;
#[macro_use]
extern crate cfg_if;

mod cpu_hasher;
#[cfg(feature = "opencl")]
mod gpu_hasher;
#[cfg(feature = "opencl")]
mod ocl;
mod plotter;
mod poc_hashing;
mod scheduler;
mod shabal256;
mod utils;
mod writer;

use crate::plotter::{Plotter, PlotterTask};
use crate::utils::set_low_prio;
use clap::AppSettings::{ArgRequiredElseHelp, DeriveDisplayOrder, VersionlessSubcommands};
#[cfg(feature = "opencl")]
use clap::ArgGroup;
use clap::{App, Arg};
use std::cmp::min;

fn main() {
    let arg = App::new("Engraver")
        .version(crate_version!())
        .author(crate_authors!())
        .about(crate_description!())
        /*
        .setting(SubcommandRequiredElseHelp)
        */
        .setting(ArgRequiredElseHelp)
        .setting(DeriveDisplayOrder)
        .setting(VersionlessSubcommands)
        .arg(
            Arg::with_name("disable direct i/o")
                .short("d")
                .long("ddio")
                .help("Disables direct i/o")
                .global(true),
        ).arg(
            Arg::with_name("disable async i/o")
                .short("a")
                .long("daio")
                .help("Disables async writing (single RAM buffer mode)")
                .global(true),
        ).arg(
            Arg::with_name("low priority")
                .short("l")
                .long("prio")
                .help("Runs engraver with low priority")
                .global(true),
        ).arg(
            Arg::with_name("non-verbosity")
                .short("q")
                .long("quiet")
                .help("Runs engraver in non-verbose mode")
                .global(true),
        ).arg(
            Arg::with_name("benchmark")
                .short("b")
                .long("bench")
                .help("Runs engraver in xPU benchmark mode")
                .global(true),
        )
        /*
        .subcommand(
            SubCommand::with_name("plot")
                .about("Plots a PoC2 file for your account ID")
                .setting(ArgRequiredElseHelp)
                .setting(DeriveDisplayOrder)
                */.arg(
                    Arg::with_name("numeric id")
                        .short("i")
                        .long("id")
                        .value_name("numeric_ID")
                        .help("your numeric Account ID")
                        .takes_value(true)
                        .required_unless("ocl-devices"),
                ).arg(
                    Arg::with_name("start nonce")
                        .short("s")
                        .long("sn")
                        .value_name("start_nonce")
                        .help("where you want to start plotting")
                        .takes_value(true)
                        .required_unless("ocl-devices"),
                ).arg(
                    Arg::with_name("nonces")
                        .short("n")
                        .long("n")
                        .value_name("nonces")
                        .help("how many nonces you want to plot")
                        .takes_value(true)
                        .required_unless("ocl-devices"),
                ).arg(
                    Arg::with_name("path")
                        .short("p")
                        .long("path")
                        .value_name("path")
                        .help("target path for plotfile (optional)")
                        .takes_value(true)
                        .required(false),
                ).arg(
                    Arg::with_name("memory")
                        .short("m")
                        .long("mem")
                        .value_name("memory")
                        .help("maximum memory usage (optional)")
                        .takes_value(true)
                        .required(false),
                ).args(&[
                    Arg::with_name("cpu")
                        .short("c")
                        .long("cpu")
                        .value_name("threads")
                        .help("maximum cpu threads you want to use (optional)")
                        .required(false)
                        .takes_value(true),
                    #[cfg(feature = "opencl")]
                    Arg::with_name("gpu")
                        .short("g")
                        .long("gpu")
                        .value_name("platform_id:device_id:cores")
                        .help("GPU(s) you want to use for plotting (optional)")
                        .multiple(true)
                        .takes_value(true),
                ]).groups(&[#[cfg(feature = "opencl")]
                ArgGroup::with_name("processing")
                    .args(&["cpu", "gpu"])
                    .multiple(true)])
                    /*
                    .arg(
                    Arg::with_name("ssd buffer")
                        .short("b")
                        .long("ssd_cache")
                        .value_name("ssd_cache")
                        .help("*path to ssd cache for staging (optional)")
                        .takes_value(true)
                        .required(false),
                        
                ),
                
        ).subcommand(
            SubCommand::with_name("encode")
                .about("*Individualizes a PoC3 reference file for your account ID")
                .display_order(2)
                .arg(
                    Arg::with_name("numeric id")
                        .short("i")
                        .long("numeric_ID")
                        .value_name("numeric ID")
                        .help("numeric Account ID")
                        .takes_value(true),
                ),
        ).subcommand(
            SubCommand::with_name("decode")
                .about("*Restores a PoC3 reference file from an individualized file")
                .display_order(3)
                .arg(
                    Arg::with_name("numeric id")
                        .short("i")
                        .long("numeric_ID")
                        .value_name("numeric ID")
                        .help("numeric Account ID")
                        .takes_value(true)
                        .required(true),
                ),
                
        )*/;

    #[cfg(feature = "opencl")]
    let arg = arg
        .arg(
            Arg::with_name("ocl-devices")
                .short("o")
                .long("opencl")
                .help("Display OpenCL platforms and devices")
                .global(true),
        )
        .arg(
            Arg::with_name("zero-copy")
                .short("z")
                .long("zcb")
                .help("Enables zero copy buffers for shared mem (integrated) gpus")
                .global(true),
        );
    let matches = &arg.get_matches();

    if matches.is_present("low priority") {
        set_low_prio();
    }

    if matches.is_present("ocl-devices") {
        #[cfg(feature = "opencl")]
        ocl::platform_info();
        return;
    }

    // plotting
    /* subcommand
    if let Some(matches) = matches.subcommand_matches("plot") {
    */
    let numeric_id = value_t!(matches, "numeric id", u64).unwrap_or_else(|e| e.exit());
    let start_nonce = value_t!(matches, "start nonce", u64).unwrap_or_else(|e| e.exit());
    let nonces = value_t!(matches, "nonces", u64).unwrap_or_else(|e| e.exit());
    let output_path = value_t!(matches, "path", String).unwrap_or_else(|_| {
        std::env::current_dir()
            .unwrap()
            .into_os_string()
            .into_string()
            .unwrap()
    });
    let mem = value_t!(matches, "memory", String).unwrap_or_else(|_| "0B".to_owned());
    let cpu_threads = value_t!(matches, "cpu", u8).unwrap_or(0u8);

    let gpus = if matches.occurrences_of("gpu") > 0 {
        let gpu = values_t!(matches, "gpu", String);
        Some(gpu.unwrap())
    } else {
        None
    };

    // work out number of cpu threads to use
    let cores = sys_info::cpu_num().unwrap() as u8;
    let cpu_threads = if cpu_threads == 0 {
        cores
    } else {
        min(cores, cpu_threads)
    };

    // special case: dont use cpu if only a gpu is defined
    #[cfg(feature = "opencl")]
    let cpu_threads = if matches.occurrences_of("gpu") > 0 && matches.occurrences_of("cpu") == 0 {
        0u8
    } else {
        cpu_threads
    };

    let p = Plotter::new();
    p.run(PlotterTask {
        numeric_id,
        start_nonce,
        nonces,
        output_path,
        mem,
        cpu_threads,
        gpus,
        direct_io: !matches.is_present("disable direct i/o"),
        async_io: !matches.is_present("disable async i/o"),
        quiet: matches.is_present("non-verbosity"),
        benchmark: matches.is_present("benchmark"),
        zcb: matches.is_present("zero-copy"),
    });
}
