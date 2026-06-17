import re
import numpy as np
import matplotlib.pyplot as plt

def plot_file(filename, label, color):
    with open(filename) as f:
        content = f.read()

    flow_blocks = re.split(r'(?=FlowID:)', content)
    small_fcts = []

    for block in flow_blocks:
        size_match = re.search(r'Flow size: (\d+) bytes', block)
        fct_match  = re.search(r'FCT: ([\d.]+)', block)
        if size_match and fct_match:
            if int(size_match.group(1)) == 29200:          # small flows only
                small_fcts.append(float(fct_match.group(1)) * 1e6)  # s → µs

    small_fcts = sorted(small_fcts)
    cdf = np.arange(1, len(small_fcts) + 1) / len(small_fcts)

    plt.plot(small_fcts, cdf, color=color, marker='s', markersize=2, label=label)

plot_file('mq_tcn_fct_parsed.txt', 'TCN', 'black')
plot_file('mq_ecnsharp_fct_parsed.txt', 'ESNSharp', 'red')

plt.xlabel(r'Time ($\mu$s)'); plt.ylabel('CDF')
plt.xlim(0, 5000); plt.ylim(0, 1)
plt.legend(); plt.tight_layout(); plt.savefig('fig20b.png', dpi=150)
