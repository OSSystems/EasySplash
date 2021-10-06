// EasySplash - tool for animated splash screens
// Copyright (C) 2020  O.S. Systems Software LTDA.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

mod animation;
mod gstreamer;
mod message;

use crate::{animation::Animation, message::Message};

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
    Client(Client),
}

/// open the render with the specific animation
#[derive(FromArgs)]
#[argh(subcommand, name = "open")]
struct Open {
    /// paths to try to load the animation
    #[argh(positional)]
    paths: Vec<PathBuf>,

    /// runtime directory (default to '/tmp/easysplash')
    #[argh(option, default = "PathBuf::from(\"/tmp/easysplash\")")]
    runtime_dir: PathBuf,

    /// custom video-sink to use when playing the animation
    #[argh(option)]
    video_sink: Option<String>,

    /// log level to use (default to 'info')
    #[argh(option, default = "LevelFilter::Info")]
    log: LevelFilter,
}

/// control the render from the user space
#[derive(FromArgs)]
#[argh(subcommand, name = "client")]
struct Client {
    /// stop the render as soon as possible
    #[argh(switch)]
    stop: bool,

    /// runtime directory (default to '/tmp/easysplash')
    #[argh(option, default = "PathBuf::from(\"/tmp/easysplash\")")]
    runtime_dir: PathBuf,

    /// log level to use (default to info)
    #[argh(option, default = "LevelFilter::Info")]
    log: LevelFilter,
}

async fn open(args: Open) -> Result<(), anyhow::Error> {
    simple_logging::log_to_stderr(args.log);

    #[cfg(feature = "systemd")]
    systemd::daemon::notify(false, [(systemd::daemon::STATE_READY, "1")].iter())?;

    info!("starting EasySplash animation");

    match args.paths.iter().find(|p| p.exists()) {
        Some(path) => {
            let socket = message::bind_socket(args.runtime_dir).await?;
            gstreamer::play_animation(Animation::from_path(path)?, args.video_sink, socket).await?;

            Ok(())
        }
        None => Err(anyhow::format_err!("could not load any of provided animations")),
    }
}

async fn client(args: Client) -> Result<(), anyhow::Error> {
    simple_logging::log_to_stderr(args.log);

    if args.stop {
        message::send(args.runtime_dir, Message::Interrupt).await?;
    }

    Ok(())
}

#[async_std::main]
async fn main() -> Result<(), anyhow::Error> {
    let cmdline: CmdLine = argh::from_env();

    match cmdline.inner {
        Commands::Open(args) => open(args).await?,
        Commands::Client(args) => client(args).await?,
    }

    Ok(())
}
