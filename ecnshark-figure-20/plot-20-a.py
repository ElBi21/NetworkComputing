import matplotlib.pyplot as plt
import numpy as np

with open('multiqueue-sim-res-ecnsharp.txt', 'r') as f:
    lines = f.readlines()

lines = [float(line[28:].strip()) for line in lines]

flows = [[], [], []]
for (i, line) in enumerate(lines):
    flows[i % 3].append(line)


print(flows[0])
print(flows[1])
print(flows[2])

t = np.arange(len(flows[0])) * 0.15


plt.figure(figsize=(5,4))

plt.plot(
    t, flows[0],
    marker='D',
    markersize=5,
    fillstyle='none',
    linewidth=2,
    label='Flow 1'
)

plt.plot(
    t, flows[1],
    marker='o',
    markersize=5,
    fillstyle='none',
    linewidth=2,
    label='Flow 2'
)

plt.plot(
    t, flows[2],
    marker='s',
    markersize=5,
    fillstyle='none',
    linewidth=2,
    label='Flow 3'
)

plt.xlabel('Time (s)')
plt.ylabel('Throughput (Gbps)')  # TODO should be goodput
plt.xlim(0, 3.0)
plt.ylim(0, 10)

plt.legend()
plt.grid(False)

plt.tight_layout()
# plt.show()
plt.savefig('plot.png')
