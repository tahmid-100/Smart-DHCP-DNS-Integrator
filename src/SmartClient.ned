package smartdhcpdns;

simple SmartClient
{
    parameters:
        double startTime @unit(s) = default(1s);
        int leaseTime @unit(s) = default(60s);
        string hostname = default("");  // If empty, MAC-based name will be used

        @display("i=device/pc");
        @signal[ipAssigned](type=long);
        @signal[dnsQuerySent](type=long);
        @statistic[ipAssignments](source=ipAssigned; record=count);
        @statistic[dnsQueries](source=dnsQuerySent; record=count);

    gates:
        inout port;
}