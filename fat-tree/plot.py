import pandas as pd
import matplotlib.pyplot as plt

baseline_file = 'data/baseline.csv'
sharp_file = 'data/sharp.csv'

df1 = pd.read_csv(baseline_file)
df2 = pd.read_csv(sharp_file)



plt.figure(figsize=(10, 6))
plt.plot(df1['load'], df1['fct-avg'], label='DCTCP-RED/TCN Average', marker='o')
plt.plot(df1['load'], df1['fct-99'], label='DCTCP-RED/TCN 99th Percentile', marker='o')
plt.plot(df2['load'], df2['fct-avg'], label='ECN# Average', marker='x')
plt.plot(df2['load'], df2['fct-99'], label='ECN# 99th Percentile', marker='x')
plt.xlabel('Load (%)')
plt.ylabel('Flow Completion Time (ms)')
plt.title('Fat Tree (K=4) Comparison of ECN# and DCTCP-RED / TCN')
plt.legend()
plt.grid()

plt.show()