use clap::Parser;
use std::{
    fs,
    io::{Read, Seek, SeekFrom},
};
mod cli;
use cli::{Cli, PageReplacementAlgorithm};
mod tlb;
use tlb::Tlb;

struct PageTableEntry {
    insertion_time: usize,
    pn: usize,
}

impl PageReplacementAlgorithm {
    fn algorithm(
        &self,
        ram_frames: &mut Vec<PageTableEntry>,
        all_pns: &Vec<usize>,
        present_idx: usize,
    ) -> (usize, usize) {
        match self {
            PageReplacementAlgorithm::Fifo => fifo(ram_frames, all_pns, present_idx),
            PageReplacementAlgorithm::Lru => lru(ram_frames, all_pns, present_idx),
            PageReplacementAlgorithm::Opt => opt(ram_frames, all_pns, present_idx),
        }
    }
}

fn opt(
    ram_frames: &mut Vec<PageTableEntry>,
    all_pns: &Vec<usize>,
    present_idx: usize,
) -> (usize, usize) {
    let mut longest_dist: usize = 0;
    let mut longest_pn_idx: usize = 0;

    // If we're at the final page number
    if present_idx == all_pns.len() - 1 {
        // It doesn't matter who we eject
        return (0, eject(ram_frames, all_pns, present_idx, 0));
    }

    // For each page number in the page table
    for (idx, entry) in ram_frames.iter().enumerate() {
        let mut found: bool = false;

        // Find the soonest occurrence of this page number in the future
        for (future_idx, pn) in all_pns.iter().enumerate().skip(present_idx + 1) {
            if entry.pn == *pn {
                let dist = future_idx - present_idx;
                found = true;

                // If this is farther than the last page number we found, save it to kick out
                if dist > longest_dist {
                    longest_dist = dist;
                    longest_pn_idx = idx;
                }
                break;
            }
        }

        // If we didn't find any future occurrences of this page number
        if !found {
            // We should kick it out, it will never be used again
            longest_pn_idx = idx;
            break;
        }
    }

    (
        longest_pn_idx,
        eject(ram_frames, all_pns, present_idx, longest_pn_idx),
    )
}

fn lru(
    ram_frames: &mut Vec<PageTableEntry>,
    all_pns: &Vec<usize>,
    present_idx: usize,
) -> (usize, usize) {
    let mut longest_dist: usize = 0;
    let mut longest_pn_idx: usize = 0;

    // If we're at the final page number
    if present_idx == all_pns.len() - 1 {
        // It doesn't matter who we eject
        return (0, eject(ram_frames, all_pns, present_idx, 0));
    }

    // For each page number in the page table
    for (idx, entry) in ram_frames.iter().enumerate() {
        // Find the closest occurrence of this page number in the past
        for (past_idx, pn) in all_pns.iter().enumerate().take(present_idx - 1) {
            if entry.pn == *pn {
                let dist = present_idx - past_idx;

                // If this is farther than the last page number we found, save it to kick out
                if dist > longest_dist {
                    longest_dist = dist;
                    longest_pn_idx = idx;
                }
                break;
            }
        }
    }

    (
        longest_pn_idx,
        eject(ram_frames, all_pns, present_idx, longest_pn_idx),
    )
}

fn fifo(
    ram_frames: &mut Vec<PageTableEntry>,
    all_pns: &Vec<usize>,
    present_idx: usize,
) -> (usize, usize) {
    let mut oldest_time = usize::MAX;
    let mut oldest_idx: usize = 0;

    // For each page number in the page table
    for (idx, entry) in ram_frames.iter().enumerate() {
        // Find if this is the oldest page number inserted into the page table
        if entry.insertion_time < oldest_time {
            oldest_time = entry.insertion_time;
            oldest_idx = idx;
        }
    }

    (
        oldest_idx,
        eject(ram_frames, all_pns, present_idx, oldest_idx),
    )
}

fn main() {
    /**********STRUCTURE DECLARATION**********/
    let args = Cli::parse();

    let mut tlb = Tlb::new(args.tlb_entries);
    let mut ram_frames: Vec<PageTableEntry> = Vec::with_capacity(args.frames);

    let mut tlb_misses = 0;
    let mut page_faults = 0;

    let mut backing_store =
        fs::File::open("BACKING_STORE.bin").expect("Unable to read BACKING_STORE.bin!");

    /**********FILE PARSING**********/
    // Get all the the page numbers fromt the file
    let contents = fs::read_to_string(&args.file)
        .unwrap_or_else(|_| panic!("Unable to read file: {}", args.file.display()));

    // Structured as logical address, page number, offset
    let page_numbers_zipped: Vec<(usize, usize, usize)> = contents
        .split('\n')
        .filter_map(|string| string.parse().ok())
        .map(|address| (address, (address >> 8usize) & 0xFF, address & 0xFF))
        .collect();

    let page_numbers: Vec<usize> = page_numbers_zipped.iter().map(|(_, pn, _)| *pn).collect();

    /**********RUNNING SIMULATOR**********/
    // Check the TLB, if in TLB move on, else look at Page Table
    for (i, page_number) in page_numbers.iter().enumerate() {
        let mut page_frame_idx = usize::MAX;

        if !tlb.contains(*page_number) {
            // If this page number is in RAM but not in TLB
            if let Some(idx) = ram_frames.iter().position(|entry| entry.pn == *page_number) {
                // We have a soft miss
                if !args.debug {
                    page_frame_idx = idx;
                }
            } else {
                // We have a hard miss
                if ram_frames.len() < ram_frames.capacity() {
                    // Just fill up normally until full
                    page_frame_idx = ram_frames.len();
                    ram_frames.push(PageTableEntry {
                        insertion_time: i,
                        pn: *page_number,
                    });
                } else {
                    // When full, eject from the page table using strategy
                    let (frame_idx, ejected_page_number) =
                        args.pra.algorithm(&mut ram_frames, &page_numbers, i);
                    tlb.remove(ejected_page_number);
                    page_frame_idx = frame_idx;
                }

                page_faults += 1;
            }

            tlb.push(*page_number);
            tlb_misses += 1;
        } else if !args.debug {
            // We have a hit, just get the index
            if let Some(idx) = ram_frames.iter().position(|entry| entry.pn == *page_number) {
                page_frame_idx = idx;
            }
        }

        // Print all the juicy info
        let (logical_address, _page_number, offset) = page_numbers_zipped[i];

        // Fetch page from backing store at page_number * PAGE_SIZE, print hex
        if backing_store
            .seek(SeekFrom::Start((page_number * args.page_size) as u64))
            .is_ok()
        {
            let mut page = vec![0u8; args.page_size];

            if let Ok(()) = backing_store.read_exact(&mut page) {
                let value = page[offset] as i8;

                print!("{}, {}, {}, ", logical_address, value, page_frame_idx);
                if !args.debug {
                    for byte in &page {
                        print!("{:02X}", byte);
                    }
                }
                println!();
            }
        }
    }

    print_statistics(tlb_misses, page_faults, page_numbers.len(), args.debug);
}

/* HELPER FUNCTIONS */
fn eject(
    ram_frames: &mut [PageTableEntry], // Pass a slice instead of a vector
    all_pns: &[usize],
    present_idx: usize,
    replace_idx: usize,
) -> usize {
    let ejected_pn = ram_frames[replace_idx].pn;
    ram_frames[replace_idx] = PageTableEntry {
        insertion_time: present_idx,
        pn: all_pns[present_idx],
    };
    ejected_pn
}

fn print_statistics(tlb_misses: usize, page_faults: usize, addresses: usize, debug: bool) {
    /* Prints statistics from TLB and RAM Operations */
    let hits = addresses - tlb_misses;
    if debug {
        println!("***********************************");
    }
    println!("Number of Translated Addresses = {}", addresses);
    println!("Page Faults = {}", page_faults);
    if !debug {
        println!(
            "Page Fault Rate = {:.3}",
            page_faults as f64 / addresses as f64,
        );
    }
    println!("TLB Hits = {}", hits);
    println!("TLB Misses = {}", tlb_misses);
    if !debug {
        println!("TLB Hit Rate = {:.3}", hits as f64 / addresses as f64,);
    }
}
