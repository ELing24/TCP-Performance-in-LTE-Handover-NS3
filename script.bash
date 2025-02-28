#!/bin/bash 
# before start - source env/bin/activate
#for simulation 1
# declare -a sport=(0 35 65 5)

# echo "Simulation 1"
# for i in "${sport[@]}"
# do
#     echo "Started Running"
#     ./ns3 run scratch/lte_project_template.cc -- --speed="$i" 


#     #PCAP Part B-3
#     tshark -r capture-n3-i1.pcap -T fields -e frame.time_epoch -e tcp.srcport -e tcp.dstport -e tcp.seq_raw -e tcp.ack_raw -e tcp.len -E separator=, > ./results/tcp_packets.csv


#     # Part B-1 
#     echo "Running Python Script"
#     python ./plot.py 1 "$i"
#     echo "Finished executing"
# done

declare -a numOfeNBs=(1 2 3 5)

echo "Simulation 2"
for i in "${numOfeNBs[@]}"
do
    echo "Started Running with $i eNBs"
    ./ns3 run scratch/lte_project_template.cc -- --numberOfEnbs="$i" --speed=25 --simPart=2


    tshark -r capture-n3-i1.pcap -T fields -e frame.time_epoch -e tcp.srcport -e tcp.dstport -e tcp.seq_raw -e tcp.ack_raw -e tcp.len -E separator=, > ./results/tcp_packets.csv



    echo "Running Python Script"
    python ./plot.py 2 "$i"
    echo "Finished executing"

done