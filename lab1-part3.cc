#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Lab1Part3");

int main(int argc, char* argv[])
{
    bool verbose = true;
    uint32_t nWifi = 4;        // Default: 4 STA nodes per WiFi
    uint32_t nPackets = 10;    // Default number of packets
    bool tracing = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nWifi", "Number of WiFi STA devices per network (max 9)", nWifi);
    cmd.AddValue("nPackets", "Number of packets to send (max 20)", nPackets);
    cmd.AddValue("verbose", "Enable logging", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);
    cmd.Parse(argc, argv);

    if (nWifi > 9)
    {
        std::cout << "nWifi must be 9 or less." << std::endl;
        return 1;
    }
    if (nPackets > 20)
    {
        std::cout << "nPackets must be 20 or less." << std::endl;
        return 1;
    }

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    // --- Ponto a ponto entre os APs ---
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices = pointToPoint.Install(p2pNodes);

    // --- Rede WiFi 1 ---
    NodeContainer wifi1StaNodes;
    wifi1StaNodes.Create(nWifi);
    NodeContainer wifi1ApNode = p2pNodes.Get(0);

    YansWifiChannelHelper channel1 = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy1;
    phy1.SetChannel(channel1.Create());

    WifiHelper wifi1;
    WifiMacHelper mac1;
    Ssid ssid1 = Ssid("wifi-network-1");

    mac1.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid1), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices1 = wifi1.Install(phy1, mac1, wifi1StaNodes);

    mac1.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid1));
    NetDeviceContainer apDevices1 = wifi1.Install(phy1, mac1, wifi1ApNode);

    // --- Rede WiFi 2 ---
    NodeContainer wifi2StaNodes;
    wifi2StaNodes.Create(nWifi);
    NodeContainer wifi2ApNode = p2pNodes.Get(1);

    YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy2;
    phy2.SetChannel(channel2.Create());

    WifiHelper wifi2;
    WifiMacHelper mac2;
    Ssid ssid2 = Ssid("wifi-network-2");

    mac2.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid2), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices2 = wifi2.Install(phy2, mac2, wifi2StaNodes);

    mac2.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid2));
    NetDeviceContainer apDevices2 = wifi2.Install(phy2, mac2, wifi2ApNode);

    // --- Mobilidade ---
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(wifi1StaNodes);
    mobility.Install(wifi2StaNodes);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifi1ApNode);
    mobility.Install(wifi2ApNode);

    // --- Internet stack ---
    InternetStackHelper stack;
    stack.Install(wifi1StaNodes);
    stack.Install(wifi1ApNode);
    stack.Install(wifi2StaNodes);
    stack.Install(wifi2ApNode);

    // --- Endereçamento IP ---
    Ipv4AddressHelper address;
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer wifi1Interfaces = address.Assign(staDevices1);
    address.Assign(apDevices1);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer wifi2Interfaces = address.Assign(staDevices2);
    address.Assign(apDevices2);

    // --- Aplicações ---
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(wifi2StaNodes.Get(nWifi-1)); // último STA da rede 2
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(2.0 + nPackets)); // tempo suficiente

    UdpEchoClientHelper echoClient(wifi2Interfaces.GetAddress(nWifi-1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(nPackets));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(wifi1StaNodes.Get(nWifi-1)); // último STA da rede 1
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(2.0 + nPackets));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(3.0 + nPackets));

    if (tracing)
    {
        phy1.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy1.EnablePcap("wifi1", apDevices1.Get(0));
        phy2.EnablePcap("wifi2", apDevices2.Get(0));
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}

