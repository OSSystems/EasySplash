// EasySplash - tool for animated splash screens
// Copyright (C) 2020  O.S. Systems Software LTDA.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

use async_std::{
    io,
    os::unix::net::{UnixListener, UnixStream},
    prelude::*,
};
use log::trace;
use std::{fs, path::PathBuf};

#[derive(Debug)]
pub(crate) enum Message {
    /// Send a interrupt request for the animation.
    Interrupt,
}

pub(crate) async fn bind_socket(runtime_dir: PathBuf) -> Result<UnixListener, io::Error> {
    let path = unix_socket_file(runtime_dir, true).await?;
    if path.exists() {
        fs::remove_file(&path)?;
    }

    trace!("Binding to {:?} unix socket", &path);
    UnixListener::bind(&path).await
}

pub(crate) async fn send(runtime_dir: PathBuf, msg: Message) -> Result<(), io::Error> {
    trace!("Sending stop request");
    UnixStream::connect(unix_socket_file(runtime_dir, false).await?)
        .await?
        .write_all(&[msg.into()])
        .await
}

impl From<u8> for Message {
    fn from(v: u8) -> Self {
        match v {
            0x1 => Message::Interrupt,
            _ => unreachable!("invalid message code"),
        }
    }
}

impl From<Message> for u8 {
    fn from(msg: Message) -> Self {
        match msg {
            Message::Interrupt => 0x1,
        }
    }
}

async fn unix_socket_file(
    runtime_dir: PathBuf,
    create_missing: bool,
) -> Result<PathBuf, io::Error> {
    if create_missing && !runtime_dir.exists() {
        fs::create_dir_all(&runtime_dir)?;
    }

    Ok(runtime_dir.join("ipc.socket"))
}
