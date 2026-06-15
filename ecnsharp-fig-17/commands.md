run simulation:
```
#!/bin/bash
# run_fct_sweep.sh

SENDERS=(25 50 75 100 125 150 175 200)
AQMS=(RED CODEL ECNSharp)

for AQM in "${AQMS[@]}"; do
    for N in "${SENDERS[@]}"; do
        echo "Running AQM=$AQM numOfSenders=$N"
        ./waf --run "queue-track \
            --id=${AQM}_${N} \
            --AQM=$AQM \
            --transportProt=DcTcp \
            --numOfSenders=$N \
            --bufferSize=600 \
            --endTime=4.5 \
            --simEndTime=5.0 \
            --load=0.9 \
            --flowNum=1000 \
            --REDMarkingThreshold=40 \
            --ECNSharpInterval=150 \
            --ECNSharpTarget=10 \
            --ECNSharpMarkingThreshold=20 \
            --CODELInterval=240 \
            --CODELTarget=10" 2>/dev/null
    done
done
```


copy all
```
docker exec c5abe92072dc bash -c "ls /root/ns3-ecn-sharp/Queue_Track_*_Flow_Monitor.xml"   | xargs -I{} docker cp c5abe92072dc:{} .
```


```
python3 plot_17.py --xml-dir xml-results/ --out fig17.png
```