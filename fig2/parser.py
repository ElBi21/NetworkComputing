import pandas as pd
import matplotlib.pyplot as plt

csv_file = 'fig2data5.csv'

df = pd.read_csv(csv_file)

plt.figure(figsize=(10, 6))
plt.plot(df['kh-threshold'], df['fct-avg'], label='fct-avg', marker='o')
plt.plot(df['kh-threshold'], df['fct-99'], label='fct-99', marker='o')
plt.xlabel('Threshold (KB)')
plt.ylabel('FCT (ms)')
plt.title('Instantaneous FCT Marking in ECN')
plt.legend()
plt.grid()

plt.show()