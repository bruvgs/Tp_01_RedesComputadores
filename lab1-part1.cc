
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Lab1Part1");

int main(int argc, char *argv[])
{
    // -----------------------------
    // Parâmetros configuráveis
    // -----------------------------
    uint32_t nClients = 1;  // número de clientes (padrão = 1)
    uint32_t nPackets = 1;  // número de pacotes por cliente (padrão = 1)

    // Leitura dos parâmetros de linha de comando
    CommandLine cmd(__FILE__);
    cmd.AddValue("nClients", "Número de clientes (máximo 5)", nClients);
    cmd.AddValue("nPackets", "Número de pacotes por cliente (máximo 5)", nPackets);
    cmd.Parse(argc, argv);

    if (nClients > 5)
    {
        NS_ABORT_MSG("O número máximo de clientes é 5.");
    }

    Time::SetResolution(Time::NS);

    // Habilita logs do cliente e servidor UDP
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // -----------------------------
    // Criação dos nós
    // -----------------------------
    // 1 nó servidor + nClients clientes
    NodeContainer serverNode;
    serverNode.Create(1);

    NodeContainer clientNodes;
    clientNodes.Create(nClients);

    // -----------------------------
    // Configuração dos links P2P
    // -----------------------------
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    // Containers para armazenar os dispositivos e interfaces
    std::vector<NetDeviceContainer> devices;
    std::vector<Ipv4InterfaceContainer> interfaces;

    InternetStackHelper stack;
    stack.Install(serverNode);
    stack.Install(clientNodes);

    // -----------------------------
    // Atribuição de endereços IP
    // -----------------------------
    Ipv4AddressHelper address;

    // Base inicial 10.1.1.0
    uint32_t networkBase = 1;

    for (uint32_t i = 0; i < nClients; ++i)
    {
        // Cria um par cliente-servidor
        NodeContainer pair(clientNodes.Get(i), serverNode.Get(0));

        // Instala o link ponto-a-ponto entre eles
        NetDeviceContainer linkDevices = pointToPoint.Install(pair);
        devices.push_back(linkDevices);

        // Gera uma sub-rede diferente para cada link: 10.1.X.0/24
        std::ostringstream subnet;
        subnet << "10.1." << networkBase++ << ".0";
        address.SetBase(Ipv4Address(subnet.str().c_str()), "255.255.255.0");

        // Atribui endereços IP
        Ipv4InterfaceContainer iface = address.Assign(linkDevices);
        interfaces.push_back(iface);
    }

    // -----------------------------
    // Configuração do servidor UDP
    // -----------------------------
    uint16_t port = 15; // Porta modificada conforme instruções
    UdpEchoServerHelper echoServer(port);

    ApplicationContainer serverApps = echoServer.Install(serverNode.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    // -----------------------------
    // Configuração dos clientes UDP
    // -----------------------------
    Ptr<UniformRandomVariable> randomStart = CreateObject<UniformRandomVariable>();
    randomStart->SetAttribute("Min", DoubleValue(2.0));
    randomStart->SetAttribute("Max", DoubleValue(7.0));

    for (uint32_t i = 0; i < nClients; ++i)
    {
        // O servidor está sempre no endereço .2 de cada sub-rede
        Ipv4Address serverAddress = interfaces[i].GetAddress(1);

        UdpEchoClientHelper echoClient(serverAddress, port);
        echoClient.SetAttribute("MaxPackets", UintegerValue(nPackets));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        echoClient.SetAttribute("PacketSize", UintegerValue(1024));

        ApplicationContainer clientApp = echoClient.Install(clientNodes.Get(i));

        // Cada cliente inicia em um tempo aleatório entre 2 e 7 s
        double startTime = randomStart->GetValue();
        clientApp.Start(Seconds(startTime));
        clientApp.Stop(Seconds(20.0));
    }

    // -----------------------------
    // Execução da simulação
    // -----------------------------
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}

