# Mini-TCP Protocol Implementation Over UDP

This project implements a reliable **Mini-TCP protocol** built on top of unreliable UDP/IP sockets. It was developed by Bahareh Einollahi and Rayhaneh Einollahi for a Computer Network course.

---

## 🛠️ Core Design & Protocol Structure

The project builds a custom transport layer over UDP using a 13-byte header and a maximum payload size of 1024 bytes per packet.

* **Packet Header Structure:** Contains sequence numbers (`seq_num`), acknowledgment numbers (`ack_num`), payload length (`length`), an Internet checksum (`checksum`), and control flags.


* **Control Flags:**
* `FLAG_DATA` (`0x01`): Indicates normal data payload.


* `FLAG_ACK` (`0x02`): Indicates an acknowledgment message.


* `FLAG_FIN` (`0x04`): Used to cleanly close the connection.




* **Checksum (`compute_checksum`):** Uses a standard 16-bit Internet Checksum to detect and catch data corruption during transmission.


* **Serialization:** Converts C++ packet structs into a flat byte array using Network Byte Order (Big Endian) via `htonl` and `htons`.



---

## 🚀 Implemented Phases & Features

### 1. Stop-and-Wait Reliable Transfer

* The sender sends a single packet and waits for an ACK before sending the next one.


* Includes a custom timeout to retransmit packets if an ACK is delayed or lost.



### 2. Sliding Window Protocol

* Upgraded to send multiple packets concurrently up to a fixed window size (`WINDOW_SIZE = 5`) to improve network efficiency and throughput.


* The receiver buffers out-of-order packets instead of dropping them, achieving cumulative acknowledgments.



### 3. Network Impairments (Loss and Delay)

* Added `--loss` and `--delay` command-line flags to test protocol resilience.


* Simulated pseudo-random packet drops and artificial RTT delays to observe cascading retransmissions and performance drops.



### 4. Congestion Control (Slow Start & Congestion Avoidance)

* Implemented dynamic congestion window (`cwnd`) and slow start threshold (`ssthresh`) variables.


* Implements **Slow Start** (exponential growth) and **Congestion Avoidance** (linear growth), resetting `cwnd` to 1 packet upon a timeout event.



### 5. Fast Retransmit & Fast Recovery

* Instead of waiting for a slow timeout timer, receiving 3 duplicate ACKs triggers an immediate **Fast Retransmit** of the lost packet.


* **Fast Recovery** halves the congestion window instead of dropping it back to 1, preserving network throughput.



### 6. Bonus Features

* **Wireshark Analysis:** Captured loopback traffic (`lo` interface) on port `8080` to visualize packet flow and congestion behavior.


* **Concurrent Connections:** Successfully ran multiple sender and receiver instances simultaneously on different ports (e.g., ports `8080` and `8081`) to handle multiple file transfers at the same time.



---

## 💻 How to Run

Examples of running the server and client components with loss and delay parameters:

```bash
# Start the receiver with simulated loss and delay
./receiver 8080 output.txt --loss 0.1 --delay 100

# Start the sender using the sliding window protocol
./sender 127.0.0.1 8080 input.txt sliding-window

```
