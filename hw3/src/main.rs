use std::{collections::HashMap, fs::{self, File}, io::{BufRead, BufReader}, path, usize};
use clap::Parser;

#[derive(clap::Parser)]
struct Cli {
    file: path::PathBuf,
    frames: usize,
    pra: String,
}

struct Address {
    address: usize,
    value: i32,
    frame_number: i32
}

struct PageTableEntry {
    insertion_time: usize,
    pn: usize
}

fn opt(ram: &mut Vec<usize>, idx: usize, pn: usize) {
    for i in idx..ram.len() {

    }    
}

fn lru(ram: &mut Vec<usize>, idx: usize, pn: usize) {

}

fn fifo(ram: &mut Vec<usize>, idx: usize, pn: usize) {

}

fn main() -> Result<(), std::io::Error> {
    
    let args = Cli::parse();

    println!("Reference Sequence file: {:?}", args.file);
    println!("Frames: {:?}", args.frames);
    println!("PRA: {:?}", args.pra);

    let strategy = match args.pra.as_str() {
        "OPT" => opt,
        "LRU" => lru,
        "FIFO" => fifo,
        _ => panic!("Invalid arg!")
    };

    // CONSTANTS
    const page_table_entries: u32 = 2u32.pow(8);
    const tlb_entries: u32 = 16;
    const size: usize = 256;  // [Bytes], page size = frame size = block size
    let physical_memory_size: usize = size * args.frames; 

    let mut tlb = HashMap::<usize, String>::new();
    let mut page_table: Vec<PageTableEntry> = Vec::with_capacity(args.frames);
    let backing_store = File::open("backing_store.bin");

    // 1. Get all the the page numbers fromt the file
    let contents = fs::read_to_string(args.file)
        .expect("Not able to read file!");

    let page_numbers: Vec<usize> = contents
        .split('\n')
        .map(|string| {
            string.parse().expect("Not valid a number!")
        }).map(|logical_address: usize| {
            logical_address >> 16 & 0xFF
        }).collect();

    // 2. Print info from backing store at PN
    
    // 3. Check the TLB, if in TLB move on, else look aat RAM
    for (i, page_number) in page_numbers.iter().enumerate() {
        if !tlb.contains_key(&page_number) {
            if page_table.contains(&page_number) {
                // We have a soft miss
                // therefore do FIFO or TLB
            } else {
                // We have a hard miss

                fn opt(page_table: &mut Vec<(usize, usize)>, all_pns: &Vec<usize>, present_idx: usize) -> usize {
                    let mut longest_dist: usize = 0;
                    let mut longest_pn_idx: usize = 0;

                    // For each page number in the page table
                    for (i, pn) in page_table.iter().enumerate() {
                        // TODO handle bounds condition

                        // Find the soonest occurrence of this page number in the future
                        for future_idx in present_idx+1..all_pns.len() {
                            if *pn == all_pns[future_idx]{
                                let dist = future_idx - present_idx;
                                
                                // If this is farther than the last page number we found, 
                                // save it to kick out
                                if dist > longest_dist {
                                    longest_dist = dist;
                                    longest_pn_idx = i;
                                }
                                break;
                            } 
                        }    
                    }

                    let ejected = page_table[longest_pn_idx];
                    page_table[longest_pn_idx] = all_pns[present_idx];
                    ejected
                }

                fn fifo(page_table: &mut Vec<PageTableEntry>, all_pns: &Vec<usize>, present_idx: usize) -> usize {
                    let mut oldest_time= usize::MAX;
                    let mut oldest_idx: usize = 0;

                    for (idx, entry) in page_table.iter().enumerate() {
                        if entry.insertion_time < oldest_time {
                            oldest_time = entry.insertion_time;
                            oldest_idx = idx;
                        }
                    }
                    
                    let ejected = &page_table[oldest_idx];
                    page_table[oldest_idx] = PageTableEntry {
                        insertion_time: present_idx, 
                        pn: all_pns[present_idx]
                    };
                    ejected.pn
                }

                let ejected_pn = opt(&mut page_table, &page_numbers, i)
                let ejected_pn = fifo(&mut page_table, &page_numbers, i)
            }
        }
    }

    return Ok(())
}
