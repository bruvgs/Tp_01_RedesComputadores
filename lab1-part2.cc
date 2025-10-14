#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Lab1Part2");

int main(int argc, char* argv[])
{
    bool verbose = true;
    uint32_t nCsma = 3;     // número de nós extras CSMA
    uint32_t nPackets = 1;  // número de pacotes a serem enviados (padrão)

    CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Número de nós extras na rede CSMA", nCsma);
    cmd.AddValue("nPackets", "Número de pacotes enviados pelo cliente (máx. 20)", nPackets);
    cmd.AddValue("verbose", "Habilitar logs detalhados", verbose);
    cmd.Parse(argc, argv);

    if (nPackets > 20)
    {
        NS_ABORT_MSG("O número máximo de pacotes permitidos é 20.");
    }

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    nCsma = nCsma == 0 ? 1 : nCsma;

    // Criação dos nós principais
    NodeContainer p2pNodes; // cliente e nó intermediário
    p2pNodes.Create(2);

    // Criação dos nós da LAN CSMA (inclui o segundo nó p2p)
    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1)); // adiciona o nó intermediário
    csmaNodes.Create(nCsma);

    // Novo nó servidor conectado ao último nó CSMA
    NodeContainer p2p2Nodes;
    p2p2Nodes.Add(csmaNodes.Get(nCsma)); // último nó CSMA
    p2p2Nodes.Create(1); // novo nó servidor

    // Configuração dos links
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices = pointToPoint.Install(p2pNodes);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer csmaDevices = csma.Install(csmaNodes);

    // Segundo link ponto-a-ponto (mesmos parâmetros)
    PointToPointHelper pointToPoint2;
    pointToPoint2.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint2.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2p2Devices = pointToPoint2.Install(p2p2Nodes);

    // Instalação das pilhas de rede
    InternetStackHelper stack;
    stack.Install(p2pNodes.Get(0)); // cliente
    stack.Install(csmaNodes);
    stack.Install(p2p2Nodes.Get(1)); // servidor

    // Atribuição de endereços IP
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces = address.Assign(csmaDevices);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer p2p2Interfaces = address.Assign(p2p2Devices);

    // Aplicações UDP
    uint16_t port = 9;
    UdpEchoServerHelper echoServer(port);
    ApplicationContainer serverApps = echoServer.Install(p2p2Nodes.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(30.0));

    UdpEchoClientHelper echoClient(p2p2Interfaces.GetAddress(1), port);
    echoClient.SetAttribute("MaxPackets", UintegerValue(nPackets));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(p2pNodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(30.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    pointToPoint.EnablePcapAll("lab1-part2-p2p");
    csma.EnablePcap("lab1-part2-csma", csmaDevices.Get(1), true);
    pointToPoint2.EnablePcapAll("lab1-part2-p2p2");

    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}

