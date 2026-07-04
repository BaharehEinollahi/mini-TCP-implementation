import matplotlib.pyplot as plt

log_data = """
3.6481,SEND,Sent DATA Packet (Seq: 0)
104.414,ACK,Window advanced to ACK: 1024
104.525,SEND,Sent DATA Packet (Seq: 1024)
104.659,SEND,Sent DATA Packet (Seq: 2048)
205.485,ACK,Window advanced to ACK: 2048
205.619,SEND,Sent DATA Packet (Seq: 3072)
205.771,SEND,Sent DATA Packet (Seq: 4096)
1209.03,TIMEOUT,RTO expired. Adjusting ssthresh and cwnd...
1209.12,RETRANSMIT,Retransmitted DATA Packet (Seq: 2048)
1209.16,RETRANSMIT,Retransmitted DATA Packet (Seq: 3072)
1209.25,RETRANSMIT,Retransmitted DATA Packet (Seq: 4096)
1310.44,ACK,Window advanced to ACK: 5120
1310.47,SEND,Sent DATA Packet (Seq: 5120)
1310.52,SEND,Sent DATA Packet (Seq: 6144)
1310.79,SEND,Sent DATA Packet (Seq: 7168)
1612.5,ACK,Window advanced to ACK: 6144
1612.56,SEND,Sent DATA Packet (Seq: 8192)
1713.4,ACK,Window advanced to ACK: 7168
1713.56,SEND,Sent DATA Packet (Seq: 9216)
1814.25,ACK,Window advanced to ACK: 8192
1814.3,SEND,Sent DATA Packet (Seq: 10240)
1814.32,SEND,Sent DATA Packet (Seq: 11264)
1915.01,ACK,Window advanced to ACK: 9216
1915.06,SEND,Sent DATA Packet (Seq: 12288)
2015.89,ACK,Window advanced to ACK: 10240
2015.94,SEND,Sent DATA Packet (Seq: 13312)
2116.52,ACK,Window advanced to ACK: 11264
2116.57,SEND,Sent DATA Packet (Seq: 14336)
2217.31,ACK,Window advanced to ACK: 12288
2218.03,SEND,Sent DATA Packet (Seq: 15360)
2218.08,SEND,Sent DATA Packet (Seq: 16384)
2318.39,ACK,Window advanced to ACK: 13312
2318.5,SEND,Sent DATA Packet (Seq: 17408)
2419.79,ACK,Window advanced to ACK: 14336
2419.86,SEND,Sent DATA Packet (Seq: 18432)
3420.92,TIMEOUT,RTO expired. Adjusting ssthresh and cwnd...
3421.04,RETRANSMIT,Retransmitted DATA Packet (Seq: 14336)
3421.07,RETRANSMIT,Retransmitted DATA Packet (Seq: 15360)
3421.11,RETRANSMIT,Retransmitted DATA Packet (Seq: 16384)
3421.16,RETRANSMIT,Retransmitted DATA Packet (Seq: 17408)
3421.17,RETRANSMIT,Retransmitted DATA Packet (Seq: 18432)
3522.43,ACK,Window advanced to ACK: 17408
3522.49,SEND,Sent DATA Packet (Seq: 19456)
3824.92,ACK,Window advanced to ACK: 19456
3824.97,SEND,Sent DATA Packet (Seq: 20480)
3824.98,SEND,Sent DATA Packet (Seq: 21504)
4026.34,ACK,Window advanced to ACK: 20480
4026.38,SEND,Sent DATA Packet (Seq: 22528)
4026.77,SEND,Sent DATA Packet (Seq: 23552)
4127.1,ACK,Window advanced to ACK: 21504
4127.14,SEND,Sent DATA Packet (Seq: 24576)
4227.95,ACK,Window advanced to ACK: 22528
4228.06,SEND,Sent DATA Packet (Seq: 25600)
4329.31,ACK,Window advanced to ACK: 23552
4329.37,SEND,Sent DATA Packet (Seq: 26624)
4430.14,ACK,Window advanced to ACK: 24576
4430.19,SEND,Sent DATA Packet (Seq: 27648)
4430.2,SEND,Sent DATA Packet (Seq: 28672)
4530.89,ACK,Window advanced to ACK: 25600
4530.93,SEND,Sent DATA Packet (Seq: 29696)
4631.85,ACK,Window advanced to ACK: 26624
4632,SEND,Sent DATA Packet (Seq: 30720)
4733.58,ACK,Window advanced to ACK: 27648
4734.34,SEND,Sent DATA Packet (Seq: 31744)
4834.67,ACK,Window advanced to ACK: 28672
4834.73,SEND,Sent DATA Packet (Seq: 32768)
5836.92,TIMEOUT,RTO expired. Adjusting ssthresh and cwnd...
5837.08,RETRANSMIT,Retransmitted DATA Packet (Seq: 28672)
5837.16,RETRANSMIT,Retransmitted DATA Packet (Seq: 29696)
5837.22,RETRANSMIT,Retransmitted DATA Packet (Seq: 30720)
5837.25,RETRANSMIT,Retransmitted DATA Packet (Seq: 31744)
5837.32,RETRANSMIT,Retransmitted DATA Packet (Seq: 32768)
5940.19,ACK,Window advanced to ACK: 33792
5940.28,SEND,Sent DATA Packet (Seq: 33792)
5940.32,SEND,Sent DATA Packet (Seq: 34816)
5940.37,SEND,Sent DATA Packet (Seq: 35840)
6441.89,ACK,Window advanced to ACK: 34816
6441.96,SEND,Sent DATA Packet (Seq: 36864)
6441.98,SEND,Sent DATA Packet (Seq: 37888)
6542.62,ACK,Window advanced to ACK: 35840
6542.66,SEND,Sent DATA Packet (Seq: 38912)
6643.47,ACK,Window advanced to ACK: 36864
6644.81,SEND,Sent DATA Packet (Seq: 39936)
6744.55,ACK,Window advanced to ACK: 37888
6744.6,SEND,Sent DATA Packet (Seq: 40960)
6845.21,ACK,Window advanced to ACK: 38912
6845.25,SEND,Sent DATA Packet (Seq: 41984)
6845.27,SEND,Sent DATA Packet (Seq: 43008)
7849,TIMEOUT,RTO expired. Adjusting ssthresh and cwnd...
7849.13,RETRANSMIT,Retransmitted DATA Packet (Seq: 38912)
7849.17,RETRANSMIT,Retransmitted DATA Packet (Seq: 39936)
7849.21,RETRANSMIT,Retransmitted DATA Packet (Seq: 40960)
7849.23,RETRANSMIT,Retransmitted DATA Packet (Seq: 41984)
7849.24,RETRANSMIT,Retransmitted DATA Packet (Seq: 43008)
7951.26,ACK,Window advanced to ACK: 44032
7951.3,SEND,Sent DATA Packet (Seq: 44032)
7951.31,SEND,Sent DATA Packet (Seq: 45056)
7951.32,SEND,Sent DATA Packet (Seq: 46080)
8354.45,ACK,Window advanced to ACK: 45056
8354.5,SEND,Sent DATA Packet (Seq: 47104)
8354.89,SEND,Sent DATA Packet (Seq: 48128)
8455.19,ACK,Window advanced to ACK: 46080
8555.98,ACK,Window advanced to ACK: 47104
8656.76,ACK,Window advanced to ACK: 48128
8757.03,ACK,Window advanced to ACK: 48300
"""

times = []
throughputs = []

prev_time = None
prev_ack = 0

for line in log_data.strip().split('\n'):
    parts = line.split(',')
    if len(parts) < 3:
        continue
    
    time_ms = float(parts[0])
    event = parts[1]
    msg = parts[2]
    
    if event == 'ACK':
        current_ack = int(msg.split(': ')[1])
        current_time_sec = time_ms / 1000.0
        
        if prev_time is not None:
            delta_time = current_time_sec - prev_time
            delta_bytes = current_ack - prev_ack
            
            # Calculate Bytes per second
            throughput = delta_bytes / delta_time if delta_time > 0 else 0
            
            # To show the 'dead zones' during timeouts accurately,
            # we inject a 0 throughput point just before this ACK recovery
            if delta_time > 0.5: 
                times.append(current_time_sec - 0.01)
                throughputs.append(0)
                
            times.append(current_time_sec)
            throughputs.append(throughput)
            
        prev_time = current_time_sec
        prev_ack = current_ack

# Plotting
plt.figure(figsize=(10, 5))
plt.plot(times, throughputs, marker='o', linestyle='-', color='b')
plt.title('Throughput Over Time')
plt.xlabel('Time (seconds)')
plt.ylabel('Throughput (Bytes/sec)')
plt.grid(True, linestyle='--', alpha=0.6)
plt.show()