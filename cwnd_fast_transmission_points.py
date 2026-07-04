import pandas as pd
import matplotlib.pyplot as plt


df_cwnd = pd.read_csv('cwnd.csv')


df_events = pd.read_csv('events.log', names=['Time(ms)', 'Event', 'Details'])


df_fr = df_events[df_events['Event'] == 'FAST_RETRANSMIT'].copy()


df_cwnd = df_cwnd.sort_values('Time(ms)')
df_fr = df_fr.sort_values('Time(ms)')


if not df_fr.empty:
    df_fr_mapped = pd.merge_asof(df_fr, df_cwnd, on='Time(ms)', direction='nearest')
else:
    df_fr_mapped = pd.DataFrame(columns=['Time(ms)', 'Event', 'Details', 'cwnd', 'ssthresh'])


fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(11, 8), sharex=True, gridspec_kw={'height_ratios': [3, 1]})


ax1.plot(df_cwnd['Time(ms)'], df_cwnd['cwnd'], marker='o', linestyle='-', color='blue', alpha=0.7, label='cwnd')
ax1.step(df_cwnd['Time(ms)'], df_cwnd['ssthresh'], where='post', color='red', linestyle='--', label='ssthresh')


if not df_fr_mapped.empty:
    ax1.scatter(df_fr_mapped['Time(ms)'], df_fr_mapped['cwnd'], 
                color='darkorange', marker='X', s=150, edgecolor='black', label='Fast Retransmit Trigger', zorder=5)

ax1.set_ylabel('Window Size', fontsize=12)
ax1.set_title('TCP Congestion Window (cwnd) & Fast Retransmit Event Tracking', fontsize=14, fontweight='bold')
ax1.grid(True, linestyle=':', alpha=0.6)
ax1.legend(fontsize=11, loc='upper right')


if not df_fr.empty:
    ax2.scatter(df_fr['Time(ms)'], [1] * len(df_fr), 
                color='darkorange', marker='|', s=800, linewidths=3, label='Fast Retransmit Occurred')
else:
    ax2.text(0.5, 0.5, 'No Fast Retransmit Events Logged', 
             horizontalalignment='center', verticalalignment='center', transform=ax2.transAxes, color='gray')

ax2.set_xlabel('Time (ms)', fontsize=12)
ax2.set_ylabel('Event Logs', fontsize=12)
ax2.set_ylim(0.5, 1.5)
ax2.set_yticks([])  
ax2.grid(True, linestyle=':', alpha=0.6)
ax2.legend(fontsize=11, loc='upper right')


plt.tight_layout()
plt.show()