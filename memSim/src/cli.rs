use std::{fmt, path::PathBuf};
use clap::{builder::PossibleValue, Parser, ValueEnum};

#[derive(Parser, Debug)]
#[command(name = "memSim")]
pub struct Cli {
    pub file: PathBuf,

    #[arg(default_value_t=256)]
    pub frames: usize,

    #[arg(default_value_t=PageReplacementAlgorithm::Fifo)]
    pub pra: PageReplacementAlgorithm,

    #[arg(short, long, default_value_t=16)]
    pub tlb_entries: usize,

    #[arg(short, long, default_value_t=256)]
    pub page_size: usize,

    #[arg(short, long)]
    pub debug: bool
}

#[derive(Debug, Clone)]
pub enum PageReplacementAlgorithm {
    Fifo,
    Lru,
    Opt,
}

// Below is boilerplate to allow for strict all caps CLI matching
impl fmt::Display for PageReplacementAlgorithm {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let s = match self {
            PageReplacementAlgorithm::Fifo => "FIFO",
            PageReplacementAlgorithm::Lru => "LRU",
            PageReplacementAlgorithm::Opt => "OPT",
        };
        write!(f, "{}", s)
    }
}

impl ValueEnum for PageReplacementAlgorithm {
    fn value_variants<'a>() -> &'a [Self] {
        static VARIANTS: [PageReplacementAlgorithm; 3] = [
            PageReplacementAlgorithm::Fifo,
            PageReplacementAlgorithm::Lru,
            PageReplacementAlgorithm::Opt,
        ];
        &VARIANTS
    }

    fn to_possible_value(&self) -> Option<PossibleValue> {
        Some(match self {
            PageReplacementAlgorithm::Fifo => PossibleValue::new("FIFO"),
            PageReplacementAlgorithm::Lru => PossibleValue::new("LRU"),
            PageReplacementAlgorithm::Opt => PossibleValue::new("OPT"),
        })
    }
}