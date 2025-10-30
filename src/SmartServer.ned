package smartdhcpdns;

simple SmartServer
{
    parameters:
        string ipPool = default("192.168.1.10-192.168.1.50");
        string subnetMask = default("255.255.255.0");
        string gateway = default("192.168.1.1");
        string dnsServer = default("192.168.1.1");
        int leaseTime @unit(s) = default(60s);
        string friendlyNames = default("");

        // Security parameters
        bool enableSecurity = default(true);
        string macWhitelist = default("AA:BB:CC:DD:EE:01,AA:BB:CC:DD:EE:02,AA:BB:CC:DD:EE:03");
        int maxRequestsPerMinute = default(10);

        @display("i=device/server");
        @signal[dhcpAssigned](type=long);
        @signal[dnsRegistered](type=long);
        @statistic[dhcpAssignments](source=dhcpAssigned; record=count);
        @statistic[dnsRegistrations](source=dnsRegistered; record=count);

    gates:
        inout port[];
}