package smartdhcpdns;

network SmartNetwork
{
    parameters:
        int numClients = default(3);

    submodules:
        server: SmartServer {
            @display("p=288,56;i=device/server");
        }

        client[numClients]: SmartClient {
            @display("p=200,200,r,150;i=device/pc");
        }

    connections allowunconnected:
        // Direct connections between server and clients
        for i=0..numClients-1 {
            client[i].port <--> {  delay = 1ms; } <--> server.port++;
        }
}
