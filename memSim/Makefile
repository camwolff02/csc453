build:
	cargo build --release
	cp target/release/memSim .

clean:
	cargo clean
	rm -f memSim
	rm -f out

test_out: 
	cargo run -- tests/example_input.txt > out && diff out tests/example_output.txt

test1:
	cargo run -- tests/fifo1.txt -d 10 FIFO 

test2:
	cargo run -- tests/fifo2.txt -d 5 FIFO 

test3:
	cargo run -- tests/fifo3.txt -d 5 FIFO

test4:
	cargo run -- tests/fifo4.txt -d 5 FIFO

test5:
	cargo run -- tests/fifo5.txt -d 8 FIFO -t 5

test6:
	cargo run -- tests/lru1.txt -d 5 LRU

test7:
	cargo run -- tests/lru2.txt -d 5 LRU 

test8:
	cargo run -- tests/lru3.txt -d 3 LRU

test9:
	cargo run -- tests/opt1.txt -d 5 OPT

test10:
	cargo run -- tests/opt2.txt -d 5 OPT