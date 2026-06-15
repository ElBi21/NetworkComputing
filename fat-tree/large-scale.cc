#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <set>

// The CDF in TrafficGenerator
extern "C"
{
#include "cdf.h"
}

#define LINK_CAPACITY_BASE    1000000000          // 1Gbps
#define BUFFER_SIZE 250                           // 250 packets

// The flow port range, each flow will be assigned a random port number within this range
static uint16_t PORT = 1000;

#define PACKET_SIZE 1400

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LargeScale");

enum AQM {
  TCN,
  ECNSharp
};

// Acknowledged to https://github.com/HKUST-SING/TrafficGenerator/blob/master/src/common/common.c
double poission_gen_interval(double avg_rate)
{
  if (avg_rate > 0)
    return -logf(1.0 - (double)rand() / RAND_MAX) / avg_rate;
  else
    return 0;
}

template<typename T>
T rand_range (T min, T max)
{
  return min + ((double)max - min) * rand () / RAND_MAX;
}

static Ptr<QueueDisc>
CreateSwitchQueueDisc (AQM aqm)
{
  ObjectFactory switchSideQueueFactory;

  if (aqm == TCN)
    {
      switchSideQueueFactory.SetTypeId ("ns3::TCNQueueDisc");
    }
  else
    {
      switchSideQueueFactory.SetTypeId ("ns3::ECNSharpQueueDisc");
    }

  return switchSideQueueFactory.Create<QueueDisc> ();
}

static void
InstallQueueDiscOnDevice (Ptr<NetDevice> netDevice, Ptr<QueueDisc> queueDisc)
{
  Ptr<TrafficControlLayer> tcl = netDevice->GetNode ()->GetObject<TrafficControlLayer> ();
  queueDisc->SetNetDevice (netDevice);
  tcl->SetRootQueueDiscOnDevice (netDevice, queueDisc);
}

static void
InstallSwitchQueueDiscOnDevice (Ptr<NetDevice> netDevice, AQM aqm)
{
  InstallQueueDiscOnDevice (netDevice, CreateSwitchQueueDisc (aqm));
}

static void
InstallServerDelayQueueDiscOnDevice (Ptr<NetDevice> netDevice)
{
  Ptr<DelayQueueDisc> delayQueueDisc = CreateObject<DelayQueueDisc> ();
  Ptr<Ipv4SimplePacketFilter> filter = CreateObject<Ipv4SimplePacketFilter> ();

  delayQueueDisc->AddPacketFilter (filter);

  delayQueueDisc->AddDelayClass (0, MicroSeconds (1));
  delayQueueDisc->AddDelayClass (1, MicroSeconds (20));
  delayQueueDisc->AddDelayClass (2, MicroSeconds (50));
  delayQueueDisc->AddDelayClass (3, MicroSeconds (80));
  delayQueueDisc->AddDelayClass (4, MicroSeconds (160));

  InstallQueueDiscOnDevice (netDevice, delayQueueDisc);
}

void install_incast_applications (NodeContainer servers, long &flowCount, int SERVER_COUNT, int LEAF_COUNT, double START_TIME, double END_TIME, double FLOW_LAUNCH_END_TIME)
{
  NS_LOG_INFO ("Install incast applications:");
  for (int i = 0; i < SERVER_COUNT; i++)
    {
      Ptr<Node> destServer = servers.Get (i);
      Ptr<Ipv4> ipv4 = destServer->GetObject<Ipv4> ();
      Ipv4InterfaceAddress destInterface = ipv4->GetAddress (1,0);
      Ipv4Address destAddress = destInterface.GetLocal ();

      uint32_t fanout = rand () % 50 + 100;
      for (uint32_t j = 0; j < fanout; j++)
        {
          double startTime = START_TIME + static_cast<double> (rand () % 100) / 1000000;
          while (startTime < FLOW_LAUNCH_END_TIME)
            {
              flowCount ++;
              uint32_t fromServerIndex = rand () % SERVER_COUNT;
              uint16_t port = PORT++;

              BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (destAddress, port));
              uint32_t flowSize = rand () % 10000;
              uint32_t tos = rand() % 5;

              source.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
              source.SetAttribute ("MaxBytes", UintegerValue(flowSize));
              source.SetAttribute ("SimpleTOS", UintegerValue (tos));

              // Install apps
              ApplicationContainer sourceApp = source.Install (servers.Get (fromServerIndex));
              sourceApp.Start (Seconds (startTime));
              sourceApp.Stop (Seconds (END_TIME));

              // Install packet sinks
              PacketSinkHelper sink ("ns3::TcpSocketFactory",
                                     InetSocketAddress (Ipv4Address::GetAny (), port));
              ApplicationContainer sinkApp = sink.Install (servers. Get (i));
              sinkApp.Start (Seconds (START_TIME));
              sinkApp.Stop (Seconds (END_TIME));

              startTime += static_cast<double> (rand () % 1000) / 1000000;
            }

        }

    }
}

void install_applications (int fromLeafId, NodeContainer servers, double requestRate, struct cdf_table *cdfTable,
                           long &flowCount, long &totalFlowSize, int SERVER_COUNT, int LEAF_COUNT, double START_TIME, double END_TIME, double FLOW_LAUNCH_END_TIME)
{
  NS_LOG_INFO ("Install applications:");
  for (int i = 0; i < SERVER_COUNT; i++)
    {
      int fromServerIndex = fromLeafId * SERVER_COUNT + i;

      double startTime = START_TIME + poission_gen_interval (requestRate);
      while (startTime < FLOW_LAUNCH_END_TIME)
        {
          flowCount ++;
          uint16_t port = PORT++;

          int destServerIndex = fromServerIndex;
          while (destServerIndex >= fromLeafId * SERVER_COUNT && destServerIndex < fromLeafId * SERVER_COUNT + SERVER_COUNT)
            {
              destServerIndex = rand_range (0, SERVER_COUNT * LEAF_COUNT);
            }

          Ptr<Node> destServer = servers.Get (destServerIndex);
          Ptr<Ipv4> ipv4 = destServer->GetObject<Ipv4> ();
          Ipv4InterfaceAddress destInterface = ipv4->GetAddress (1,0);
          Ipv4Address destAddress = destInterface.GetLocal ();

          BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (destAddress, port));
          uint32_t flowSize = gen_random_cdf (cdfTable);
          uint32_t tos = rand() % 5;

          totalFlowSize += flowSize;

          source.SetAttribute ("SendSize", UintegerValue (PACKET_SIZE));
          source.SetAttribute ("MaxBytes", UintegerValue(flowSize));
          source.SetAttribute ("SimpleTOS", UintegerValue (tos));

          // Install apps
          ApplicationContainer sourceApp = source.Install (servers.Get (fromServerIndex));
          sourceApp.Start (Seconds (startTime));
          sourceApp.Stop (Seconds (END_TIME));

          // Install packet sinks
          PacketSinkHelper sink ("ns3::TcpSocketFactory",
                                 InetSocketAddress (Ipv4Address::GetAny (), port));
          ApplicationContainer sinkApp = sink.Install (servers. Get (destServerIndex));
          sinkApp.Start (Seconds (START_TIME));
          sinkApp.Stop (Seconds (END_TIME));

          startTime += poission_gen_interval (requestRate);
        }
    }
}

int main (int argc, char *argv[])
{
#if 1
  LogComponentEnable ("LargeScale", LOG_LEVEL_INFO);
#endif

  // Command line parameters parsing
  std::string id = "undefined";
  unsigned randomSeed = 0;
  std::string cdfFileName = "examples/rtt-variations/DCTCP_CDF.txt";
  double load = 0.0;
  std::string transportProt = "DcTcp";

  std::string aqmStr = "ECNSharp";

  // The simulation starting and ending time
  double START_TIME = 0.0;
  double END_TIME = 0.5;

  double FLOW_LAUNCH_END_TIME = 0.2;

  uint32_t linkLatency = 10;

  // Fat-tree parameter.  K must be even.  K=4 gives:
  // 4 pods, 4 core switches, 8 aggregation switches, 8 edge switches, 16 servers.
  int K = 4;

  // Kept for command-line compatibility with the old leaf-spine script.
  // These are no longer used to size the topology; K determines the fat-tree size.
  int SERVER_COUNT = 8;
  int SPINE_COUNT = 4;
  int LEAF_COUNT = 4;
  int LINK_COUNT = 1;

  uint64_t spineLeafCapacity = 10;
  uint64_t leafServerCapacity = 10;

  uint32_t TCNThreshold = 80;

  uint32_t ECNSharpInterval = 150;
  uint32_t ECNSharpTarget = 10;
  uint32_t ECNSharpMarkingThreshold = 80;

  CommandLine cmd;
  cmd.AddValue ("ID", "Running ID", id);
  cmd.AddValue ("StartTime", "Start time of the simulation", START_TIME);
  cmd.AddValue ("EndTime", "End time of the simulation", END_TIME);
  cmd.AddValue ("FlowLaunchEndTime", "End time of the flow launch period", FLOW_LAUNCH_END_TIME);
  cmd.AddValue ("randomSeed", "Random seed, 0 for random generated", randomSeed);
  cmd.AddValue ("cdfFileName", "File name for flow distribution", cdfFileName);
  cmd.AddValue ("load", "Load of the network, 0.0 - 1.0", load);
  cmd.AddValue ("transportProt", "Transport protocol to use: Tcp, DcTcp", transportProt);
  cmd.AddValue ("linkLatency", "Link latency, should be in MicroSeconds", linkLatency);

  cmd.AddValue ("K", "Fat-tree k parameter. Must be even and at least 2", K);

  cmd.AddValue ("serverCount", "Unused in fat-tree mode; kept for compatibility", SERVER_COUNT);
  cmd.AddValue ("spineCount", "Unused in fat-tree mode; kept for compatibility", SPINE_COUNT);
  cmd.AddValue ("leafCount", "Unused in fat-tree mode; kept for compatibility", LEAF_COUNT);
  cmd.AddValue ("linkCount", "Unused in fat-tree mode; kept for compatibility", LINK_COUNT);

  cmd.AddValue ("spineLeafCapacity", "Aggregation/Core and Edge/Aggregation capacity in Gbps", spineLeafCapacity);
  cmd.AddValue ("leafServerCapacity", "Edge <-> Server capacity in Gbps", leafServerCapacity);

  cmd.AddValue ("AQM", "AQM to use: TCN or ECNSharp", aqmStr);

  cmd.AddValue ("TCNThreshold", "The threshold for TCN", TCNThreshold);

  cmd.AddValue ("ECNShaprInterval", "The persistent interval for ECNSharp", ECNSharpInterval);
  cmd.AddValue ("ECNSharpTarget", "The persistent target for ECNShapr", ECNSharpTarget);
  cmd.AddValue ("ECNShaprMarkingThreshold", "The instantaneous marking threshold for ECNSharp", ECNSharpMarkingThreshold);


  cmd.Parse (argc, argv);

  if (K < 2 || K % 2 != 0)
    {
      NS_LOG_ERROR ("Fat-tree K must be even and at least 2");
      return 0;
    }

  const int POD_COUNT = K;
  const int EDGE_PER_POD = K / 2;
  const int AGG_PER_POD = K / 2;
  const int CORE_GROUPS = K / 2;
  const int CORE_PER_GROUP = K / 2;
  const int CORE_COUNT = CORE_GROUPS * CORE_PER_GROUP;
  const int EDGE_COUNT = POD_COUNT * EDGE_PER_POD;
  const int AGG_COUNT = POD_COUNT * AGG_PER_POD;
  const int SERVER_PER_EDGE = K / 2;
  const int TOTAL_SERVER_COUNT = EDGE_COUNT * SERVER_PER_EDGE;

  uint64_t SPINE_LEAF_CAPACITY = spineLeafCapacity * LINK_CAPACITY_BASE;
  uint64_t LEAF_SERVER_CAPACITY = leafServerCapacity * LINK_CAPACITY_BASE;
  Time LINK_LATENCY = MicroSeconds (linkLatency);

  if (load <= 0.0 || load >= 1.0)
    {
      NS_LOG_ERROR ("The network load should within 0.0 and 1.0");
      return 0;
    }

  AQM aqm;
  if (aqmStr.compare ("TCN") == 0)
    {
      aqm = TCN;
    }
  else if (aqmStr.compare ("ECNSharp") == 0)
    {
      aqm = ECNSharp;
    }
  else
    {
      return 0;
    }

  if (transportProt.compare ("DcTcp") == 0)
    {
      NS_LOG_INFO ("Enabling DcTcp");
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDCTCP::GetTypeId ()));

      // TCN Configuration
      Config::SetDefault ("ns3::TCNQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
      Config::SetDefault ("ns3::TCNQueueDisc::MaxPackets", UintegerValue (BUFFER_SIZE));
      Config::SetDefault ("ns3::TCNQueueDisc::Threshold", TimeValue (MicroSeconds (TCNThreshold)));

      // ECN Sharp Configuration
      Config::SetDefault ("ns3::ECNSharpQueueDisc::Mode", StringValue ("QUEUE_MODE_PACKETS"));
      Config::SetDefault ("ns3::ECNSharpQueueDisc::MaxPackets", UintegerValue (BUFFER_SIZE));
      Config::SetDefault ("ns3::ECNSharpQueueDisc::InstantaneousMarkingThreshold", TimeValue (MicroSeconds (ECNSharpMarkingThreshold)));
      Config::SetDefault ("ns3::ECNSharpQueueDisc::PersistentMarkingTarget", TimeValue (MicroSeconds (ECNSharpTarget)));
      Config::SetDefault ("ns3::ECNSharpQueueDisc::PersistentMarkingInterval", TimeValue (MicroSeconds (ECNSharpInterval)));
    }

  NS_LOG_INFO ("Config parameters");
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(PACKET_SIZE));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));
  Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (MilliSeconds (5)));

  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
  Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (5)));
  Config::SetDefault ("ns3::TcpSocketBase::ClockGranularity", TimeValue (MicroSeconds (100)));
  Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue (MicroSeconds (80)));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (160000000));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (160000000));

  Config::SetDefault ("ns3::Ipv4GlobalRouting::PerflowEcmpRouting", BooleanValue(true));

  NodeContainer core;
  core.Create (CORE_COUNT);
  NodeContainer aggregation;
  aggregation.Create (AGG_COUNT);
  NodeContainer edge;
  edge.Create (EDGE_COUNT);
  NodeContainer servers;
  servers.Create (TOTAL_SERVER_COUNT);

  NS_LOG_INFO ("Fat-tree K: " << K);
  NS_LOG_INFO ("Pods: " << POD_COUNT
               << ", Core: " << CORE_COUNT
               << ", Aggregation: " << AGG_COUNT
               << ", Edge: " << EDGE_COUNT
               << ", Servers: " << TOTAL_SERVER_COUNT);

  NS_LOG_INFO ("Install Internet stacks");
  InternetStackHelper internet;
  Ipv4GlobalRoutingHelper globalRoutingHelper;

  internet.SetRoutingHelper (globalRoutingHelper);

  internet.Install (servers);
  internet.Install (core);
  internet.Install (aggregation);
  internet.Install (edge);

  NS_LOG_INFO ("Install channels and assign addresses");

  PointToPointHelper p2p;
  Ipv4AddressHelper ipv4;

  NS_LOG_INFO ("Configuring servers");
  // Setting servers
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (LEAF_SERVER_CAPACITY)));
  p2p.SetChannelAttribute ("Delay", TimeValue(LINK_LATENCY));
  p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));

  ipv4.SetBase ("10.1.0.0", "255.255.255.0");

  // Server <-> Edge links.
  for (int pod = 0; pod < POD_COUNT; pod++)
    {
      for (int e = 0; e < EDGE_PER_POD; e++)
        {
          int edgeIndex = pod * EDGE_PER_POD + e;

          for (int s = 0; s < SERVER_PER_EDGE; s++)
            {
              int serverIndex = edgeIndex * SERVER_PER_EDGE + s;

              ipv4.NewNetwork ();

              NodeContainer nodeContainer = NodeContainer (servers.Get (serverIndex), edge.Get (edgeIndex));
              NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);

              InstallServerDelayQueueDiscOnDevice (netDeviceContainer.Get (0));
              InstallSwitchQueueDiscOnDevice (netDeviceContainer.Get (1), aqm);

              Ipv4InterfaceContainer interfaceContainer = ipv4.Assign (netDeviceContainer);

              NS_LOG_INFO ("Edge - " << edgeIndex << " is connected to Server - " << serverIndex << " with address "
                           << interfaceContainer.GetAddress(0) << " <-> " << interfaceContainer.GetAddress (1)
                           << " with port " << netDeviceContainer.Get (0)->GetIfIndex () << " <-> " << netDeviceContainer.Get (1)->GetIfIndex ());
            }
        }
    }

  NS_LOG_INFO ("Configuring edge/aggregation/core switches");
  // Switch-to-switch links use the old spineLeafCapacity parameter.
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (SPINE_LEAF_CAPACITY)));

  // Edge <-> Aggregation links inside each pod.
  for (int pod = 0; pod < POD_COUNT; pod++)
    {
      for (int e = 0; e < EDGE_PER_POD; e++)
        {
          int edgeIndex = pod * EDGE_PER_POD + e;

          for (int a = 0; a < AGG_PER_POD; a++)
            {
              int aggIndex = pod * AGG_PER_POD + a;

              ipv4.NewNetwork ();

              NodeContainer nodeContainer = NodeContainer (edge.Get (edgeIndex), aggregation.Get (aggIndex));
              NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);

              InstallSwitchQueueDiscOnDevice (netDeviceContainer.Get (0), aqm);
              InstallSwitchQueueDiscOnDevice (netDeviceContainer.Get (1), aqm);

              Ipv4InterfaceContainer ipv4InterfaceContainer = ipv4.Assign (netDeviceContainer);
              NS_LOG_INFO ("Edge - " << edgeIndex << " is connected to Aggregation - " << aggIndex << " with address "
                           << ipv4InterfaceContainer.GetAddress(0) << " <-> " << ipv4InterfaceContainer.GetAddress (1)
                           << " with port " << netDeviceContainer.Get (0)->GetIfIndex () << " <-> " << netDeviceContainer.Get (1)->GetIfIndex ()
                           << " with data rate " << spineLeafCapacity);
            }
        }
    }

  // Aggregation <-> Core links.  Aggregation switch a in each pod connects to
  // the a-th group of K/2 core switches, which is the standard k-ary fat-tree wiring.
  for (int pod = 0; pod < POD_COUNT; pod++)
    {
      for (int a = 0; a < AGG_PER_POD; a++)
        {
          int aggIndex = pod * AGG_PER_POD + a;

          for (int c = 0; c < CORE_PER_GROUP; c++)
            {
              int coreIndex = a * CORE_PER_GROUP + c;

              ipv4.NewNetwork ();

              NodeContainer nodeContainer = NodeContainer (aggregation.Get (aggIndex), core.Get (coreIndex));
              NetDeviceContainer netDeviceContainer = p2p.Install (nodeContainer);

              InstallSwitchQueueDiscOnDevice (netDeviceContainer.Get (0), aqm);
              InstallSwitchQueueDiscOnDevice (netDeviceContainer.Get (1), aqm);

              Ipv4InterfaceContainer ipv4InterfaceContainer = ipv4.Assign (netDeviceContainer);
              NS_LOG_INFO ("Aggregation - " << aggIndex << " is connected to Core - " << coreIndex << " with address "
                           << ipv4InterfaceContainer.GetAddress(0) << " <-> " << ipv4InterfaceContainer.GetAddress (1)
                           << " with port " << netDeviceContainer.Get (0)->GetIfIndex () << " <-> " << netDeviceContainer.Get (1)->GetIfIndex ()
                           << " with data rate " << spineLeafCapacity);
            }
        }
    }

  NS_LOG_INFO ("Populate global routing tables");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  double oversubRatio = static_cast<double>(SERVER_PER_EDGE * LEAF_SERVER_CAPACITY) / (SPINE_LEAF_CAPACITY * AGG_PER_POD);
  NS_LOG_INFO ("Over-subscription ratio: " << oversubRatio);

  NS_LOG_INFO ("Initialize CDF table");
  struct cdf_table* cdfTable = new cdf_table ();
  init_cdf (cdfTable);
  load_cdf (cdfTable, cdfFileName.c_str ());

  NS_LOG_INFO ("Calculating request rate");
  double requestRate = load * LEAF_SERVER_CAPACITY * SERVER_PER_EDGE / oversubRatio / (8 * avg_cdf (cdfTable)) / SERVER_PER_EDGE;
  NS_LOG_INFO ("Average request rate: " << requestRate << " per second");

  NS_LOG_INFO ("Initialize random seed: " << randomSeed);
  if (randomSeed == 0)
    {
      srand ((unsigned)time (NULL));
    }
  else
    {
      srand (randomSeed);
    }

  NS_LOG_INFO ("Create applications");

  long flowCount = 0;
  long totalFlowSize = 0;

  for (int fromEdgeId = 0; fromEdgeId < EDGE_COUNT; fromEdgeId ++)
    {
      install_applications(fromEdgeId, servers, requestRate, cdfTable, flowCount, totalFlowSize, SERVER_PER_EDGE, EDGE_COUNT, START_TIME, END_TIME, FLOW_LAUNCH_END_TIME);
    }

  NS_LOG_INFO ("Total flow: " << flowCount);

  if (flowCount > 0)
    {
      NS_LOG_INFO ("Actual average flow size: " << static_cast<double> (totalFlowSize) / flowCount);
    }
  else
    {
      NS_LOG_INFO ("Actual average flow size: 0");
    }

  NS_LOG_INFO ("Enabling flow monitor");

  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();


  flowMonitor->CheckForLostPackets ();

  std::stringstream flowMonitorFilename;

  flowMonitorFilename << "Large_Scale_" << id << "_FatTreeK" << K << "_" << aqmStr << "_"  << transportProt << "_" << load << ".xml";


  NS_LOG_INFO ("Start simulation");
  Simulator::Stop (Seconds (END_TIME));
  Simulator::Run ();

  flowMonitor->SerializeToXmlFile(flowMonitorFilename.str (), true, true);

  Simulator::Destroy ();
  free_cdf (cdfTable);
  NS_LOG_INFO ("Stop simulation");
}
