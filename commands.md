make all

### phase 1:
./sender.o 127.0.0.1 8080 input.txt stop-and-wait
./receiver.o 8080 output.txt


### phase 2:
./sender.o 127.0.0.1 8080 input.txt sliding-window
./receiver.o 8080 output.txt


### phase 3:
./sender.o 127.0.0.1 8080 input.txt sliding-window
./receiver.o 8080 output.txt --loss 0.1 --delay 100

### phase 4:
./sender.o 127.0.0.1 8080 input.txt congestion-control
./receiver.o 8080 output.txt --loss 0.1 --delay 100

### phase 6:
./sender.o 127.0.0.1 8080 input.txt fast-retransmit
./receiver.o 8080 output.txt --loss 0.1 --delay 100