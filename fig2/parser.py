import pandas as pd
import matplotlib.pyplot as plt

csv_file = 'data/fig2data5.csv'

df = pd.read_csv(csv_file)

plt.figure(figsize=(10, 6))
plt.plot(df['kh-threshold'], df['fct-avg'], label='Averagt FCT', marker='o')
plt.plot(df['kh-threshold'], df['fct-99'], label='99th Percentile FCT', marker='o')
plt.xlabel('Threshold (KB)')
plt.ylabel('Flow Completion Time (ms)')
plt.title('Instantaneous FCT Marking in ECN')
plt.legend()
plt.grid()

plt.show()