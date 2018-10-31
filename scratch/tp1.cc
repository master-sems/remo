/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/random-variable-stream.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int main (int argc, char *argv[])
{
  // Setting seed for reproducing experiments
	int seed = 12;
   srand( seed );
   RngSeedManager::SetSeed(seed);
   RngSeedManager::SetRun(seed);
  // Reading input from command-line for experiment configuration
  CommandLine cmd;
  bool verbose = false;
  cmd.AddValue("verbose", "allows showing of LOG messages", verbose);
  cmd.Parse (argc, argv);
  if (verbose)
    LogComponentEnable("FirstScriptExample", LOG_LEVEL_INFO);
   
  NS_LOG_INFO("LOG INFO");
  // Setting the smallest timestep
  Time::SetResolution (Time::NS);
  // Enabling Logging of the Udp application
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // Helper class that manages, accesses network nodes
  NodeContainer nodes;
  nodes.Create (2);

  // Helper class responsible for setting NetDevices on nodes and connecting them to channel
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  // Network devices like network cards to be installed on nodes
  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  // Installing internet protocols (TCP, UDP, IP etc.) 
  InternetStackHelper stack;
  stack.Install (nodes);

  // Assign addresses to devices
  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // Preparing server application giving the port number to constructor
  UdpEchoServerHelper echoServer (9);
  // install application on node 1
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  // Sets start and stopping time for the application
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  // Preparing client application giving the server address and port number to constructor
  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  // Setting atributes for the client
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  // Setting start and end times for the application
  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // Mobility helper to set the positions and mobility model for nodes
  MobilityHelper mobility;


  Ptr<RandomRectanglePositionAllocator> positionAlloc = CreateObject<RandomRectanglePositionAllocator> ();
  Ptr<UniformRandomVariable> posRadomVar = CreateObject<UniformRandomVariable>();
  double limits = 100;
  posRadomVar->SetAttribute("Min", DoubleValue(0.0));
  posRadomVar->SetAttribute("Max", DoubleValue(limits));
  positionAlloc->SetX(posRadomVar);
  positionAlloc->SetY(posRadomVar);
  mobility.SetPositionAllocator (positionAlloc);

  // Random variable for speed needed for the mobility model
  Ptr<UniformRandomVariable> speed = CreateObject<UniformRandomVariable>();
  speed->SetAttribute("Min", DoubleValue(2.0));
  speed->SetAttribute("Max", DoubleValue(4.0));

//  mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
//     "Bounds", RectangleValue(Rectangle(0, 100, 0, 100)),
//     "Speed", PointerValue(speed));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
				"Bounds", RectangleValue (Rectangle (0, 100, 0, 100)),
        "Speed", StringValue ("ns3::UniformRandomVariable[Min=2.0|Max=4.0]"));

  // Install the mobility model and positions on nodes
  mobility.Install(nodes);
  
  AnimationInterface anime ("nom-anim-1.xml");

      //Allow tracing 
    AsciiTraceHelper ascii;
    pointToPoint.EnableAsciiAll (ascii.CreateFileStream("myfirst.tr"));

  Simulator::Schedule(Seconds(11.0), &Simulator::Stop);
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
