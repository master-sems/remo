/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * A simulation based on the third.cc where wifi nodes have ranges and broadcast
 * dummy data to nodes within range. Broadcasting is just for the 1-hop node and not
 * extended to the rest of the network. 
 *
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
 * \author Abdallah Sobehy
 * \date 30/10/2018
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

#include "ns3/netanim-module.h"

// boost include
#include <boost/algorithm/string.hpp>


#define DEFAULT_PORT_NUM 80
#define NULL_TERMINATOR '\0'
#define SEPARATOR ','

// Default Network Topology
//   Wifi 10.1.3.0
//                 AP
//       *    *    *
//       |    |    | 
//      n2   n1   n0 

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdWifiOnly");

/**
 * Creates a packet from the input vector
 * \param payload the input vector to feed into the packet
 * \return the created packet
 */
Ptr<Packet> createPacket(std::vector<double> payload)
{
      NS_LOG_INFO (Simulator::Now().GetSeconds() << "s Creating one packet! at node ");

	std::ostringstream msg;
	for(uint i = 0 ; i < payload.size(); i ++)
	{
		msg << payload.at(i) ;
		if(i == payload.size() -1)
			msg << NULL_TERMINATOR;
		else
			msg << SEPARATOR;
	}
	return  Create<Packet> ((uint8_t *)msg.str().c_str(), msg.str().length());
}
/**
 * extracts data of the packet containg a double vector created by createPacket function
 * \param p packet to extract
 * \return a vector of received data
 */
std::vector<double> extractPayload(Ptr<Packet> p)
{
	std::vector<double> output;
	uint8_t *buffer = new uint8_t[p->GetSize ()];
	p->CopyData(buffer, p->GetSize ());
	std::string s = std::string((char*)buffer);
	std::vector<std::string> strs;
	boost::split(strs, s, boost::is_any_of(",\0 "));
	for ( uint i = 0 ; i < strs.size() ; i++)
	{
		output.push_back(std::atof(strs.at(i).c_str()));
	}
	return output;
}

/*
 * A function to be called when a packet is received
 * \param socket of the node receiving the packet
 */
void ReceivePacket (Ptr<Socket> socket)
{

	std::vector<double> payload = extractPayload(socket->Recv());
	int recnodeId = socket->GetNode()->GetId();
	double sendTime = payload.at(0);
	NS_LOG_UNCOND (Simulator::Now().GetSeconds() << "s Received one packet! at node " << recnodeId
			<<" sent at time: " << sendTime);
}

/**
 * Sending a packet through the input socket
 * \param socket 
 */
void sendPacket(Ptr<Socket> socket)
{
	std::vector<double> data;
	data.push_back(double(Simulator::Now().GetSeconds()));
	Ptr<Packet> p = createPacket(data);
	socket->Send (p);

    NS_LOG_INFO (Simulator::Now().GetSeconds() << "s sending one packet! at node " <<  socket->GetNode()->GetId());
    return;
}

int main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nWifi = 3;
  bool tracing = false;
  double maxRange = 30;

  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue ("maxRange", "Wifi maximum range of communication", maxRange);

  cmd.Parse (argc,argv);

  // Check for valid number of wifi nodes
  // 250 should be enough, otherwise IP addresses 
  // soon become an issue
  if (nWifi > 250)
    {
      std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("ThirdWifiOnly", LOG_LEVEL_INFO);
    }

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  	// Propagation Loss
  	channel.AddPropagationLoss ("ns3::RangePropagationLossModel","MaxRange",DoubleValue (maxRange));

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();

  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));
  mac.SetType ("ns3::AdhocWifiMac");

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (0, 5, 0, 10)));
  mobility.Install (wifiStaNodes);

  InternetStackHelper stack;
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces, interfaceAp;
  interfaces = address.Assign (staDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  ///////////////
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  
  InetSocketAddress broadCastAddr = InetSocketAddress (Ipv4Address ("255.255.255.255"), DEFAULT_PORT_NUM);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny(), DEFAULT_PORT_NUM);

  Ptr<Socket> source;
  Ptr<Socket> recvSink;
  for (uint32_t i = 0 ; i < nWifi ; i++)
	{
		// receiving bind
		recvSink = Socket::CreateSocket (wifiStaNodes.Get(i), tid);
		local = InetSocketAddress (interfaces.GetAddress(i), DEFAULT_PORT_NUM);
		recvSink->Bind (local);
		recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

		// sending bind
		source = Socket::CreateSocket (wifiStaNodes.Get (i), tid);
		source->SetAllowBroadcast (true);
		source->Connect (broadCastAddr);

		// Schedule events
		Simulator::ScheduleWithContext (source->GetNode()->GetId (),
		 		Seconds (i), &sendPacket, source);
	}

  Simulator::Stop (Seconds (10.0));

  if (tracing == true)
    {
      phy.EnablePcap ("third", staDevices.Get (0));
      AsciiTraceHelper ascii;
      phy.EnableAsciiAll(ascii.CreateFileStream("third_wifi.tr"));
    }

    AnimationInterface anime ("sobehy-third-wifi-only.xml");

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
