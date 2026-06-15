ECN SHARP
```
./waf --run "queue-track \
  --id=test \
  --AQM=ECNSharp \
  --transportProt=DcTcp \
  --numOfSenders=100 \
  --bufferSize=600 \
  --endTime=4.5 \
  --simEndTime=5.0 \
  --ECNSharpInterval=150 \
  --ECNSharpTarget=10 \
  --ECNSharpMarkingThreshold=20 \
  --load=0.9 \
  --flowNum=1000"
```


CoDel
```
./waf --run "queue-track \
  --id=test \
  --AQM=CODEL \
  --transportProt=DcTcp \
  --numOfSenders=100 \
  --bufferSize=600 \
  --CODELInterval=150 \
  --CODELTarget=10 \
  --endTime=4.5 \
  --simEndTime=5.0 \
  --load=0.9 \
  --flowNum=1000"
```


plot a (RED DC...):
```
./waf --run "queue-track \
  --id=test \
  --AQM=RED \
  --transportProt=DcTcp \
  --numOfSenders=100 \
  --bufferSize=600 \
  --REDMarkingThreshold=40 \
  --endTime=4.5 \
  --simEndTime=5.0 \
  --load=0.9 \
  --flowNum=1000"
```


gnuplot enable
```
cat > /etc/apt/sources.list <<'EOF'
deb http://archive.debian.org/debian jessie main
deb http://archive.debian.org/debian-security jessie/updates main
EOF

apt-get -o Acquire::Check-Valid-Until=false update
apt-get install -y --allow-unauthenticated gnuplot-nox



docker cp 2ff0c98fdd44:/root/ns3-ecn-sharp/Queue_Track_ECNSharp_test_Queue.png .
```