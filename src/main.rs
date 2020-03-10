// EasySplash - tool for animated splash screens
// Copyright (C) 2020  O.S. Systems Software LTDA.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

mod animation;
mod gstreamer;

use crate::animation::Animation;

use argh::FromArgs;
use log::{info, LevelFilter};
use std::path::PathBuf;

/// EasySplash offers a convenient boot splash for Embedded Linux devices,
/// focusing of simplicity and easy to use.
#[derive(FromArgs)]
struct CmdLine {
    #[argh(subcommand)]
    inner: Commands,
}

#[derive(FromArgs)]
#[argh(subcommand)]
enum Commands {
    Open(Open),
}

/// open the render with the specific animation
#[derive(FromArgs)]
#[argh(subcommand, name = "open")]
struct Open {
    /// path to load the animation
    #[argh(positional)]
    path: PathBuf,

    /// log level to use (default to 'info')
    #[argh(option, default = "LevelFilter::Info")]
    log: LevelFilter,
}

#[async_std::main]
async fn main() -> Result<(), anyhow::Error> {
    let cmdline: CmdLine = argh::from_env();

    match cmdline.inner {
        Commands::Open(render) => {
            simple_logging::log_to_stderr(render.log);

            info!("starting EasySplash animation");

            gstreamer::play_animation(Animation::from_path(&render.path)?).await?;
        }
    }

    Ok(())
}
