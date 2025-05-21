use std::{collections::{HashSet, VecDeque}, fs, usize};
mod cli;
use clap::Parser;
use cli::{Cli, PageReplacementAlgorithm};

struct PageTableEntry {
    insertion_time: usize,
    pn: usize
}

impl PageReplacementAlgorithm {
    fn algorithm(&self, ram_frames: &mut Vec<PageTableEntry>, all_pns: &Vec<usize>, present_idx: usize) -> (usize, usize) {
        match self {
            PageReplacementAlgorithm::FIFO => fifo(ram_frames, all_pns, present_idx),
            PageReplacementAlgorithm::LRU => lru(ram_frames, all_pns, present_idx),
            PageReplacementAlgorithm::OPT => opt(ram_frames, all_pns, present_idx),
        }
    }
}

fn opt(ram_frames: &mut Vec<PageTableEntry>, all_pns: &Vec<usize>, present_idx: usize) -> (usize, usize) {
    let mut longest_dist: usize = 0;
    let mut longest_pn_idx: usize = 0;

    // If we're at the final page number
    if present_idx == all_pns.len()-1 {
        // It doesn't matter who we eject
       return (0, eject(ram_frames, all_pns, present_idx, 0))
    }

    // For each page number in the page table
    for (idx, entry) in ram_frames.iter().enumerate() {
        let mut found: bool = false;

        // Find the soonest occurrence of this page number in the future
        for future_idx in present_idx+1..all_pns.len() {
            if entry.pn == all_pns[future_idx]{
                let dist = future_idx - present_idx;
                found = true;                

                // If this is farther than the last page number we found, 
                // save it to kick out
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

    (longest_pn_idx, eject(ram_frames, all_pns, present_idx, longest_pn_idx))
}


fn lru(ram_frames: &mut Vec<PageTableEntry>, all_pns: &Vec<usize>, present_idx: usize) -> (usize, usize) {
    let mut longest_dist: usize = 0;
    let mut longest_pn_idx: usize = 0;

    // If we're at the final page number
    if present_idx == all_pns.len()-1 {
        // It doesn't matter who we eject
       return (0, eject(ram_frames, all_pns, present_idx, 0))
    }

    // For each page number in the page table
    for (idx, entry) in ram_frames.iter().enumerate() {
        // Find the closest occurrence of this page number in the past
        for past_idx in 0..present_idx-1 {
            if entry.pn == all_pns[past_idx]{
                let dist = present_idx - past_idx;

                // If this is farther than the last page number we found, 
                // save it to kick out
                if dist > longest_dist {
                    longest_dist = dist;
                    longest_pn_idx = idx;
                }
                break;
            } 
        }    
    }

    (longest_pn_idx, eject(ram_frames, all_pns, present_idx, longest_pn_idx))
}

fn fifo(ram_frames: &mut Vec<PageTableEntry>, all_pns: &Vec<usize>, present_idx: usize) -> (usize, usize) {
    let mut oldest_time= usize::MAX;
    let mut oldest_idx: usize = 0;

    // For each page number in the page table
    for (idx, entry) in ram_frames.iter().enumerate() {
        // Find if this is the oldest page number inserted into the page table
        if entry.insertion_time < oldest_time {
            oldest_time = entry.insertion_time;
            oldest_idx = idx;
        }
    }

    (oldest_idx, eject(ram_frames, all_pns, present_idx, oldest_idx))
}

fn eject(ram_frames: &mut Vec<PageTableEntry>, all_pns: &Vec<usize>, present_idx: usize, replace_idx: usize) -> usize {
    let ejected_pn = ram_frames[replace_idx].pn;
    ram_frames[replace_idx] = PageTableEntry {
        insertion_time: present_idx, 
        pn: all_pns[present_idx]
    };
    ejected_pn
}

struct TLB {
    _tlb_set: HashSet::<usize>,
    _tlb_queue: VecDeque::<usize>,
    _max_size: usize,
}

impl TLB {
    fn new(size: usize) -> Self {
        Self {
            _tlb_set: HashSet::new(),
            _tlb_queue: VecDeque::with_capacity(size),
            _max_size: size,
        }
    }

    fn _remove_last_if_full(&mut self) {
        if self._tlb_queue.len()+1 == self._tlb_queue.capacity() {
            if let Some(ejected_pn) = self._tlb_queue.pop_back() {
                self._tlb_set.remove(&ejected_pn);
            }
        }
    }

    fn push(&mut self, pn: usize) {
        self._remove_last_if_full();
        self._tlb_set.insert(pn);
        self._tlb_queue.push_front(pn);
    }

    fn remove(&mut self, pn: usize) {
        let mut idx_to_remove: Option<usize> = None; 

        for (i, old_pn) in self._tlb_queue.iter().enumerate() {
            if *old_pn == pn {
                idx_to_remove = Some(i);
                break;
            }
        }

        if let Some(idx) = idx_to_remove {
            self._tlb_queue.remove(idx);
            self._tlb_set.remove(&pn);
        }
    }

    fn contains(&self, pn: usize) -> bool {
        self._tlb_set.contains(&pn)
    }
}

fn main() {
    let args = Cli::parse();

    println!("Reference Sequence file: {:?}", args.file);
    println!("Frames: {:?}", args.frames);
    println!("PRA: {:?}\n", args.pra);
    
    // CONSTANTS
    const TLB_ENTRIES: usize = 16;

    let mut tlb = TLB::new(TLB_ENTRIES);
    let mut ram_frames: Vec<PageTableEntry> = Vec::with_capacity(args.frames);

    let mut tlb_misses = 0;
    let mut page_faults = 0;

    // let backing_store = File::open("backing_store.bin");

    // 1. Get all the the page numbers fromt the file
    let contents = fs::read_to_string(args.file)
        .expect("Not able to read file!");

    // Structured as logical address, page number, offset
    let page_numbers_zipped: Vec<(i32, i32,i32)> = contents
        .split('\n')
        .map(|string| string.parse().expect("Not valid a number!"))
        .map(|address| (address, address >> 8usize & 0xFF, address & 0xFF))
        .collect();
    
    let page_numbers: Vec<usize> = page_numbers_zipped
        .iter().map(|(_, pn, _)| *pn as usize).collect();

    // Check the TLB, if in TLB move on, else look at Page Table
    for (i, page_number) in page_numbers.iter().enumerate() {
        let mut page_frame_idx = usize::MAX;        

        if !tlb.contains(*page_number) {
            if ram_frames.iter().any(|entry| entry.pn == *page_number) {
                tlb.push(*page_number);  // If it's in ram but not TLB, add to TLB
                tlb_misses += 1;  // We have a soft miss
            } else {
                // We have a hard miss
                if ram_frames.len() < ram_frames.capacity() {  // fill up normally until full
                    // Just fill up normally until full
                    page_frame_idx = ram_frames.len();  // TODO is this right??????????
                    ram_frames.push(PageTableEntry {
                        insertion_time: i,
                        pn: *page_number
                    });
                } else {  // When full, eject from the page table using strategy
                    let (frame_idx, ejected_page_number) = args.pra.algorithm(&mut ram_frames, &page_numbers, i);
                    tlb.remove(ejected_page_number);
                    page_frame_idx = frame_idx;
                }
               page_faults += 1;
            }

            tlb.push(*page_number);
        }

        // Print all the juicy info
        let (logical_address, _page_number, offset) = page_numbers_zipped[i];
        // NOTE DON'T PRINT OFFSET, just SIGNED value of BYTE in offset
        // NOTE DON'T PRINT PAGE NUMBER, print index in ram_frames when page_number is inserted
        println!("{}, {}, {}", logical_address, offset, page_frame_idx);
        // From backing store, seek page_number * 256
    }
    
    // Print statistics from TLB and RAM Operations
    let addresses = page_numbers.len();
    let hits = addresses - tlb_misses;
    println!("*************************************");
    println!("Number of Translated Addresses = {}", addresses);
    println!("Page Faults = {}", page_faults);
    println!("Page Fault Rate = {}.{:#03}", page_faults / addresses, hits % addresses);
    println!("TLB Hits = {}", hits);
    println!("TLB Misses = {}", tlb_misses);
    println!("TLB Hit Rate = {}.{:#03}", hits / addresses, hits % addresses);
}
