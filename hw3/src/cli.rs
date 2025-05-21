use std::{fmt, path::PathBuf};
use clap::{builder::PossibleValue, Parser, ValueEnum};

#[derive(Parser, Debug)]
#[command(name = "memSim")]
pub struct Cli {
    pub file: PathBuf,

    #[arg(default_value_t=256)]
    pub frames: usize,

    #[arg(default_value_t=PageReplacementAlgorithm::FIFO)]
    pub pra: PageReplacementAlgorithm,
}

#[derive(Debug, Clone)]
pub enum PageReplacementAlgorithm {
    FIFO,
    LRU,
    OPT,
}

impl fmt::Display for PageReplacementAlgorithm {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let s = match self {
            PageReplacementAlgorithm::FIFO => "FIFO",
            PageReplacementAlgorithm::LRU => "LRU",
            PageReplacementAlgorithm::OPT => "OPT",
        };
        write!(f, "{}", s)
    }
}

impl ValueEnum for PageReplacementAlgorithm {
    fn value_variants<'a>() -> &'a [Self] {
        static VARIANTS: [PageReplacementAlgorithm; 3] = [
            PageReplacementAlgorithm::FIFO,
            PageReplacementAlgorithm::LRU,
            PageReplacementAlgorithm::OPT,
        ];
        &VARIANTS
    }

    fn to_possible_value(&self) -> Option<PossibleValue> {
        Some(match self {
            PageReplacementAlgorithm::FIFO => PossibleValue::new("FIFO"),
            PageReplacementAlgorithm::LRU => PossibleValue::new("LRU"),
            PageReplacementAlgorithm::OPT => PossibleValue::new("OPT"),
        })
    }
}