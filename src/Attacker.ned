package smartdhcpdns;

simple Attacker
{
    parameters:
        double startTime @unit(s) = default(10s);
        string attackType = default("dhcp_starvation");  // dhcp_starvation, dns_spoofing, mac_spoofing
        double attackRate = default(0.1);  // attacks per second

        @display("i=status/stop;is=l");
        @signal[attackBlocked](type=long);
        @statistic[attacksBlocked](source=attackBlocked; record=count);

    gates:
        inout port;
}