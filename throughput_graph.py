import matplotlib.pyplot as plt

with open('events.log', 'r') as file:
    log_data = file.read()

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
            
            
            throughput = delta_bytes / delta_time if delta_time > 0 else 0
            
            
            
            if delta_time > 0.5: 
                times.append(current_time_sec - 0.01)
                throughputs.append(0)
                
            times.append(current_time_sec)
            throughputs.append(throughput)
            
        prev_time = current_time_sec
        prev_ack = current_ack


plt.figure(figsize=(10, 5))
plt.plot(times, throughputs, marker='o', linestyle='-', color='b')
plt.title('Throughput Over Time')
plt.xlabel('Time (seconds)')
plt.ylabel('Throughput (Bytes/sec)')
plt.grid(True, linestyle='--', alpha=0.6)
plt.show()