from argparse import ArgumentParser

parser = ArgumentParser(
    prog="program Name", description="Description", epilog="bottom text"
)

parser.add_argument(
    "reference sequence file",
    help="contains the list of logical memory addresses"
)
parser.add_argument(
    "frames", help="positive integer <=256, the number of frames in memory"
)
parser.add_argument(
    "PRA", help="Physical Resource Allocation Strategy. FIFO, LRU, or OPT"
)


class VirtualMemoryManager:
    """Virtual Memory Manager Class

    Attributes:
        tlb: The Translation Lookaside Buffer, a small cache for memory
            translation.
        page_table: Keeps track of every page of a process. Provides the same
            page to frame translation as the TLB.
        backing_store: Where all the pages are originally found.
        physical_memory:
        reference_sequence:
    """

    def __init__(self, tlb_size: int = 16, page_size: int = 2**8):
        """Virtual Memory Manager Constructor

        Args:
            tlb_size: The length of the Translation Lookup Buffer
        """
        self.tlb: list[int] = [0] * tlb_size
        self.page_table = [0] * page_size
        self.backing_store = None
        self.physical_memory = None
        self.reference_sequence = None


def main():
    args = parser.parse_args()
    args  # prevent not accessed warning

    vmm = VirtualMemoryManager()
    vmm  # prevent not accessed warning

    with open(args.reference_sequence_file) as f:
        address = int(f.readline())


if __name__ == "__main__":
    main()
