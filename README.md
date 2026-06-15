<h1 align="center">Network Computing Project 🌐</h1>
<p align="center">Project for recreating the ECN# paper as part of the Network Computing course project</p>

---

## Figures 13 + 14

0. Start the container:

```bash
docker run -it snowzjx/ns3-ecn-sharp:optimized
```

> [!NOTE]
> If you disconnect from the container, just run `docker ps` to find the hash of the container and reconnect to it with `docker exec -it <container_hash> bash`

1. Inside the container, go to `/root/ns3-ecn-sharp`;
2. Simulations for the large scale are run with the command:

```bash
./waf --run "large-scale --randomSeed=233 --load=[0.2, 0.9] --ID=LeafSpine8x8 --serverCount=16 --spineCount=8 --leafCount=8 --AQM=[TCN|ECNSharp] --ECNShaprInterval=70 --ECNSharpTarget=10 --ECNShaprMarkingThreshold=70"
```

3. Visualization of the results is done through:

```bash
python examples/rtt-variations/fct_parser.py <result_xml_file>
```
