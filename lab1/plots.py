import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import io

csv_data = """x,t1,t2
0, 0.000001, 0.000001
1, 0.000001, 0.000001
2, 0.000001, 0.000001
3, 0.000001, 0.000001
4, 0.000001, 0.000001
5, 0.000001, 0.000001
6, 0.000001, 0.000001
7, 0.000001, 0.000001
8, 0.000001, 0.000001
9, 0.000001, 0.000001
10, 0.000002, 0.000001
11, 0.000002, 0.000001
12, 0.000003, 0.000002
13, 0.000005, 0.000003
14, 0.000007, 0.000004
15, 0.000012, 0.000008
16, 0.000020, 0.000015
17, 0.000023, 0.000028
18, 0.000045, 0.000050
19, 0.000097, 0.000098
20, 0.000215, 0.000222
21, 0.000472, 0.000465
22, 0.000948, 0.000920
23, 0.001912, 0.002501
24, 0.004859, 0.005932
25, 0.011441, 0.011788
26, 0.024189, 0.023446
27, 0.048276, 0.046793
28, 0.096626, 0.093190
29, 0.191536, 0.184888
30, 0.380715, 0.368563
"""

data = pd.read_csv(io.StringIO(csv_data))

# create plot x-axis - as 2^x data and y-axis - delay (just t1 and t2) in seconds  [convert required fields]
data["x"] = 2 ** data["x"]

plt.plot(data["x"], data["t1"], label="Non-blocking")
plt.plot(data["x"], data["t2"], label="Blocking")
plt.xlabel("Data [bytes]")
plt.ylabel("Delay [s]")

plt.legend()
plt.show()


# map t1 and t2 as throughput in Mbit/s (data is 2^x bytes and t1, t2 are time in seconds to transfer that amount of data)
# data["t1"] = 2 ** data["x"] / data["t1"] / 10**6
