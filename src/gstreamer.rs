// EasySplash - tool for animated splash screens
// Copyright (C) 2020  O.S. Systems Software LTDA.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

use crate::animation::Animation;

use async_std::{io, prelude::*, sync};
use derive_more::{Display, Error, From};
use gst::{prelude::*, MessageView};
use log::{debug, error, trace};

#[derive(Display, From, Error, Debug)]
pub(crate) enum Error {
    #[display(fmt = "No animation parts to play")]
    NoAnimation,

    #[display(transparent)]
    Io(io::Error),

    #[display(transparent)]
    Bool(gst::glib::error::BoolError),

    #[display(transparent)]
    Glib(gst::glib::error::Error),

    #[display(transparent)]
    StateChange(gst::StateChangeError),

    #[display(transparent)]
    ChannelReceiver(sync::RecvError),
}

pub(crate) async fn play_animation(animation: Animation) -> Result<(), Error> {
    gst::init()?;
    debug!("Using {} as player", gst::version_string());

    let playbin = gst::ElementFactory::make("playbin", None)?;

    // TODO: We are not yet handling the animation height and width properties.

    // The pipeline is feed by the `feed_pipeline` and the control messages are
    // handled by the `handle_message` future.
    //
    // Any future which finishes, allow the flow to continue.
    feed_pipeline(playbin.clone(), animation).await?;

    playbin.set_state(gst::State::Null)?;

    Ok(())
}

async fn feed_pipeline(playbin: gst::Element, animation: Animation) -> Result<(), Error> {
    // Acquire the iterator so we can walk on the animation parts.
    let mut parts = animation.into_iter();

    // Current playing part.
    let mut current_part = parts.next().ok_or(Error::NoAnimation)?;

    // Queue first animation part and ask GStreamer to start playing it.
    playbin.set_property("uri", &current_part.url())?;
    playbin.set_state(gst::State::Playing)?;

    // We need to wait for stream to start and then we can queue the next
    // part. We do that so we have a gapless playback.
    let bus = playbin.get_bus().expect("failed to get pipeline bus");
    let mut messages = bus.stream();
    while let Some(msg) = messages.next().await {
        match msg.view() {
            MessageView::Error(err) => {
                error!("{}", err.get_error());
                break;
            }
            MessageView::Eos(_) => {
                trace!("end of stream message recived, finishing");
                break;
            }
            MessageView::StreamStart(_) => {
                // If we have more animation parts to play, queue the next.
                if let Some(part) = parts.next() {
                    current_part = part;

                    trace!("video has started, queuing next part");
                    playbin.set_property("uri", &current_part.url())?;
                }
            }
            _ => (),
        };
    }

    Ok(())
}
