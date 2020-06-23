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
    /// path to load the animation
    #[argh(positional)]
    path: PathBuf,

    /// runtime directory (default to '/tmp/easysplash')
    #[argh(option, default = "PathBuf::from(\"/tmp/easysplash\")")]
    runtime_dir: PathBuf,

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

#[async_std::main]
async fn main() -> Result<(), anyhow::Error> {
    let cmdline: CmdLine = argh::from_env();

    match cmdline.inner {
        Commands::Open(render) => {
            simple_logging::log_to_stderr(render.log);

            info!("starting EasySplash animation");

            let socket = message::bind_socket(render.runtime_dir).await?;
            gstreamer::play_animation(Animation::from_path(&render.path)?, socket).await?;
        }
        Commands::Client(client) => {
            simple_logging::log_to_stderr(client.log);

            if client.stop {
                message::send(client.runtime_dir, Message::Interrupt).await?;
            }
        }
    }

    Ok(())
}
