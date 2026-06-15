import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

# ---------- 1. Parse the gnuplot data file ----------
PLT_FILE = "Queue_Track_ECNSharp_test_Queue.plt"

times, queues = [], []
with open(PLT_FILE) as f:
    in_data = False
    for line in f:
        line = line.strip()
        if line.startswith('plot'):
            in_data = True
            continue
        if not in_data:
            continue
        if line in ('e', ''):
            break
        parts = line.split()
        if len(parts) == 2:
            times.append(float(parts[0]))
            queues.append(float(parts[1]))

times  = np.array(times)
queues = np.array(queues)

# ---------- 2. Extract the incast burst window ----------
# The incast period is endTime/10. With endTime=4.5 the last burst
# lands at t=4.05s. We slice a 5ms window and shift it to start at
# t=4.000 to match the paper's x-axis presentation.
T_START = 4.05
T_END   = 4.055

mask  = (times >= T_START) & (times <= T_END)
t_win = times[mask]
q_win = queues[mask]

# Shift so the window opens at t=4.000 (paper convention)
t_plot = t_win - T_START + 4.000

# ---------- 3. Locate "Query Start" (first non-zero queue sample) ----------
nonzero = np.where(q_win > 0)[0]
query_start_t = t_plot[nonzero[0]] if len(nonzero) else 4.000

# ---------- 4. Draw the plot ----------
fig, ax = plt.subplots(figsize=(5.5, 4))

ax.plot(t_plot, q_win, color='#1f77b4', linewidth=1.0)

# Buffer cap reference line
BUFFER_SIZE = 600
ax.axhline(y=BUFFER_SIZE, color='red', linewidth=1.5)
ax.text(4.0002, BUFFER_SIZE + 15, 'Queue Size', color='red', fontsize=9)

# Query Start annotation with downward arrow
ax.annotate(
    'Query Start',
    xy=(query_start_t, 2),
    xytext=(query_start_t + 0.00025, 55),
    arrowprops=dict(arrowstyle='->', color='black', lw=1.1),
    fontsize=8.5,
)

# ---------- 5. Axes formatting ----------
ax.set_xlim(4.000, 4.005)
ax.set_ylim(0, 700)
ax.set_xlabel('Time(s)', fontsize=10)
ax.set_ylabel('Queue length(# of packets)', fontsize=10)
ax.set_title(r'(c) ECN$^\sharp$', fontsize=11, fontweight='bold')

ax.xaxis.set_major_locator(ticker.MultipleLocator(0.001))
ax.xaxis.set_major_formatter(ticker.FormatStrFormatter('%.3f'))
ax.tick_params(axis='both', labelsize=8.5)
ax.spines['top'].set_visible(False)
ax.spines['right'].set_visible(False)

plt.tight_layout()
plt.savefig('plot_c_ecnsharp_queue.png', dpi=150, bbox_inches='tight')
print("Saved: plot_c_ecnsharp_queue.png")


# Quick diagnostic — paste before the plot section
burst_times = [0.45 * i for i in range(1, 11)]
for bt in burst_times:
    mask = (times >= bt) & (times <= bt + 0.005)
    if mask.sum() > 0 and queues[mask].max() > 0:
        print(f"Burst at {bt:.2f}s: max_q={queues[mask].max():.0f}")