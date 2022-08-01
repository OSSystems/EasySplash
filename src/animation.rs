// EasySplash - tool for animated splash screens
// Copyright (C) 2020  O.S. Systems Software LTDA.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

use derive_more::{Display, Error, From};
use log::{debug, error, info, trace};
use serde::Deserialize;
use std::{
    fs, io,
    io::Read,
    iter::Iterator,
    path::{Path, PathBuf},
};

#[derive(Display, From, Error, Debug)]
pub(crate) enum Error {
    #[display(fmt = "Fail to read the manifest file. Cause: {}", _0)]
    Io(io::Error),

    #[display(fmt = "Failed to parse the manifest file. Cause: {}", _0)]
    TomlParser(toml::de::Error),

    #[display(fmt = "The animation part '{}' is missing.", "_0.display()")]
    MissingPart(#[error(not(source))] PathBuf),

    #[display(fmt = "The part with mode set as 'forever' must be the last one")]
    WrongModeForeverPart,
}

#[derive(Debug, Deserialize, PartialEq)]
#[serde(deny_unknown_fields)]
pub(crate) struct Animation {
    #[serde(rename = "part")]
    parts: Vec<Part>,
}

#[derive(Debug, Deserialize, PartialEq)]
#[serde(deny_unknown_fields)]
pub(crate) struct Part {
    file: PathBuf,
    #[serde(default)]
    mode: Mode,
    #[serde(default)]
    repeat: usize,
}

impl Animation {
    pub(crate) fn from_path(path: &Path) -> Result<Self, Error> {
        info!("loading manifest from {:?}", path);

        let mut buf = String::new();
        fs::File::open(path.join("animation.toml"))?.read_to_string(&mut buf)?;

        let mut animation = toml::from_str::<Animation>(&buf)?;

        // We ensure we have the full path to the animation files, while we also
        // ensure all files exists.
        for part in &mut animation.parts {
            part.file = fs::canonicalize(path)?.join(&part.file);
            if !part.file.exists() {
                error!("part {:?} is missing", part.file);
                return Err(Error::MissingPart(part.file.to_path_buf()));
            }
            trace!("part {:?} was found", part.file);
        }

        animation.validate_modes()?;

        Ok(animation)
    }

    fn validate_modes(&self) -> Result<(), Error> {
        // Ensure if there is any part with `Mode::Forever` it is the last one,
        // or it will never be played.
        let mut iter = self.parts.iter();
        if iter.any(|p| p.mode == Mode::Forever) && iter.next().is_some() {
            return Err(Error::WrongModeForeverPart);
        }

        Ok(())
    }
}

pub(crate) struct AnimationIter<'a> {
    inner: &'a Animation,
    current_part: usize,
    repeat: usize,
}

impl<'a> IntoIterator for &'a Animation {
    type IntoIter = AnimationIter<'a>;
    type Item = &'a Part;

    fn into_iter(self) -> Self::IntoIter {
        AnimationIter { inner: self, current_part: 0, repeat: 0 }
    }
}

// Provides an iterator which respects the number of times each part must be
// played.
impl<'a> Iterator for AnimationIter<'a> {
    type Item = &'a Part;

    fn next(&mut self) -> Option<Self::Item> {
        let part = self.inner.parts.get(self.current_part)?;

        // In case of `Mode::Forever` we just return the part.
        if let Mode::Forever = part.mode {
            return Some(part);
        }

        // Otherwise, when we iterate the number of times which are required by
        // the `current_part`, we move to the next.
        let repeat = part.repeat;
        if self.repeat > repeat {
            self.current_part += 1;
            self.repeat = 0;
        }

        // Get the required part for returning it.
        let part = self.inner.parts.get(self.current_part)?;

        if repeat > 0 {
            debug!(
                "iterator: part {:?} (current: {} / number of times: {})",
                part.file,
                self.repeat + 1,
                repeat + 1
            );
        } else {
            debug!("iterator: part {:?} (once)", part.file);
        }

        // Account for the number of repetitions we need to do.
        self.repeat += 1;

        Some(part)
    }
}

impl Part {
    pub(crate) fn url(&self) -> String {
        format!("file://{}", self.file.to_string_lossy())
    }

    pub(crate) fn is_interruptable(&self) -> bool {
        self.mode == Mode::Interruptable || self.mode == Mode::Forever
    }
}

#[derive(Debug, Deserialize, PartialEq, Clone)]
#[serde(rename_all = "snake_case")]
pub(crate) enum Mode {
    Complete,
    Interruptable,
    Forever,
}

impl Default for Mode {
    fn default() -> Self {
        Mode::Complete
    }
}

#[cfg(test)]
mod test {
    use super::*;

    fn valid_toml() -> Animation {
        let manifest_toml = toml::toml! {
            [[part]]
            file = "part1.mp4"

            [[part]]
            file = "part2.mp4"
            repeat = 1

            [[part]]
            file = "part3.mp4"
            mode = "interruptable"
        };

        manifest_toml.try_into::<Animation>().expect("Failed to parse TOML")
    }

    #[test]
    fn forever_mode_must_be_last_part() {
        let animation = toml::toml! {
            [[part]]
            file = "part1.mp4"
            mode = "forever"

            [[part]]
            file = "part2.mp4"
        }
        .try_into::<Animation>()
        .expect("Failed to parse TOML");

        animation.validate_modes().unwrap_err();
    }

    #[test]
    fn manifest_parse() {
        assert_eq!(
            valid_toml(),
            Animation {
                parts: vec![
                    Part { file: "part1.mp4".into(), mode: Mode::Complete, repeat: 0 },
                    Part { file: "part2.mp4".into(), mode: Mode::Complete, repeat: 1 },
                    Part { file: "part3.mp4".into(), mode: Mode::Interruptable, repeat: 0 },
                ]
            }
        );
    }

    #[test]
    fn iterator_takes_repeat_under_consideration_when_returning() {
        let animation = valid_toml();
        let mut animation: AnimationIter = animation.into_iter();

        assert_eq!(
            animation.next(),
            Some(&Part { file: "part1.mp4".into(), mode: Mode::Complete, repeat: 0 })
        );

        assert_eq!(
            animation.next(),
            Some(&Part { file: "part2.mp4".into(), mode: Mode::Complete, repeat: 1 })
        );

        assert_eq!(
            animation.next(),
            Some(&Part { file: "part2.mp4".into(), mode: Mode::Complete, repeat: 1 })
        );

        assert_eq!(
            animation.next(),
            Some(&Part { file: "part3.mp4".into(), mode: Mode::Interruptable, repeat: 0 })
        );
    }

    #[test]
    fn part_with_mode_forever_keeps_returning_on_iter() {
        let animation = toml::toml! {
            [[part]]
            file = "part1.mp4"

            [[part]]
            file = "part2.mp4"
            mode = "forever"
        }
        .try_into::<Animation>()
        .expect("Failed to parse TOML");

        let mut animation: AnimationIter = animation.into_iter();

        assert_eq!(
            animation.next(),
            Some(&Part { file: "part1.mp4".into(), mode: Mode::Complete, repeat: 0 })
        );

        assert_eq!(
            animation.next(),
            Some(&Part { file: "part2.mp4".into(), mode: Mode::Forever, repeat: 0 })
        );

        assert_eq!(
            animation.next(),
            Some(&Part { file: "part2.mp4".into(), mode: Mode::Forever, repeat: 0 })
        );
    }
}
