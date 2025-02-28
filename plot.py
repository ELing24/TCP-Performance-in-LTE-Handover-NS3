import pandas as pd
import matplotlib.pyplot as plt
import argparse
import numpy as np

parser = argparse.ArgumentParser(description="label for sinr diagram")
parser.add_argument("SimPart", type=int)
parser.add_argument("SimNumber", type = int)
args = parser.parse_args()
#file reading
df = pd.read_csv("./DlRsrpSinrStats.txt", sep='\t', comment="%", names=["time", "cellId", "IMSI", "RNTI", "rsrp", "sinr", "ComponentCarrierId"])
handovers = pd.read_csv("./results/handoverSim.txt", names = ["values"])
df1 = pd.read_csv("./results/tcp_packets.csv", names = ["time", "sourceport", "destinationport", "sequencenumber", "ack", "payload"])
fig, ax = plt.subplots(3, 1, figsize=(10, 6))

#throughput plot
def Throughput(t,w):

    total = 0
    start = t - (w/2)
    end = t + (w/2)
    total = df1.loc[(df1["time"] >= start) & (df1["time"] < end), "payload"].sum()
    thuput = total / w
    return thuput

start = df1["time"].min()
end = df1["time"].max()

#create a list with step of .1 within time range
throughput_steps = np.arange(start, end, .1)
renderArr = []

for t in throughput_steps:
    renderArr.append(Throughput(t, .1))

ax[0].set_title(f"Simulation {args.SimPart}-{args.SimNumber}")
ax[0].plot(throughput_steps, renderArr, marker='.', label='Throughput')
ax[0].set_xlabel("Time (seconds)")
ax[0].set_ylabel("Throughput Bps")
ax[0].grid()
for i in handovers["values"]:
    ax[0].axvline(x=i, color="r")

#RTT
matches = []
times = []
for _, row in df1.iterrows():
    if row["sourceport"] == 49153:
        ackToFind = row["sequencenumber"] + row["payload"]
        
        matchingack = df1[(df1["sourceport"] == 10000) & (df1["ack"] == ackToFind)]

        if not matchingack.empty:
            minAck = matchingack["time"].min()
            timeDiff = minAck - row["time"]
            matches.append(timeDiff)
            times.append(row["time"])

ax[1].plot(times, matches, marker='.', label='RTT ', color="orange")
ax[1].set_xlabel("Time (seconds)")
ax[1].set_ylabel("RTT")
ax[1].grid()
for i in handovers["values"]:
    ax[1].axvline(x=i, color="r")

#sinr plot
ax[2].plot(df['time'], df['sinr'], marker='.', label='SINR', color="green")
ax[2].set_xlabel("Time (seconds)")
ax[2].set_ylabel("SINR (dB)")
ax[2].grid()
for i in handovers["values"]:
    ax[2].axvline(x=i, color="r")


plt.savefig(f"./results/plot{args.SimPart}-{args.SimNumber}.png")