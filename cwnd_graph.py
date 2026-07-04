import pandas as pd
import matplotlib.pyplot as plt



df = pd.read_csv('cwnd.csv')


fig, ax = plt.subplots(figsize=(10, 6))


ax.plot(df['Time(ms)'], df['cwnd'], marker='o', linestyle='-', color='blue', label='cwnd')


ax.step(df['Time(ms)'], df['ssthresh'], where='post', color='red', linestyle='--', label='ssthresh')


ax.set_xlabel('Time (ms)', fontsize=12)
ax.set_ylabel('Window Size', fontsize=12)
ax.set_title('TCP Congestion Window (cwnd) over Time', fontsize=14, fontweight='bold')
ax.grid(True, linestyle=':', alpha=0.6)
ax.legend(fontsize=11)


plt.tight_layout()

plt.show()
# plt.savefig('cwnd_over_time_graph.png', dpi=300)