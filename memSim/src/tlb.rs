use linked_hash_set::LinkedHashSet;

pub struct Tlb {
    entries: LinkedHashSet<usize>,
    max_size: usize,
}

impl Tlb {
    pub fn new(size: usize) -> Self {
        Self {
            entries: LinkedHashSet::new(),
            max_size: size,
        }
    }

    pub fn push(&mut self, pn: usize) {
        if self.entries.len() == self.max_size {
            self.entries.pop_front();
        }
        self.entries.insert(pn);
    }

    pub fn contains(&self, pn: usize) -> bool {
        self.entries.contains(&pn)
    }

    pub fn remove(&mut self, pn: usize) {
        self.entries.remove(&pn);
    }
}
