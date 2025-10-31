# Smart DHCP-DNS Integrator

An integrated network service that automates both IP address assignment and hostname registration, providing zero-configuration networking capabilities.

## Project Overview

This project implements a unified DHCP-DNS service that eliminates the traditional gap between IP address assignment and name resolution. When a device connects to the network and receives an IP address via DHCP, it automatically becomes accessible by hostname through DNS without any manual configuration.

## Features

- **Automatic IP Assignment**: Standard DHCP protocol for dynamic IP address allocation
- **Zero-Configuration DNS**: Automatic hostname registration upon successful DHCP lease
- **Synchronized Lifecycle Management**: Coordinated DHCP lease and DNS record expiration
- **Flexible Hostname Assignment**: Three-tier hostname determination system:
  - Administrator-defined friendly names
  - Client-provided hostnames
  - Auto-generated MAC-based names
- **Resource Efficiency**: Automatic cleanup of expired leases and DNS records

## Simulation Environment

- **Platform**: OMNeT++ Discrete Event Simulator
- **Network Topology**: 1 server + 3 clients in point-to-point configuration
- **IP Pool**: 192.168.1.10-192.168.1.50
- **Default Lease Time**: 60 seconds

## Project Structure

```
SmartDnsDhcp/
  src/
    SmartServer.ned # Integrated DHCP-DNS server
    SmartClient.ned # Network client implementation
    SmartNetwork.ned # Network topology definition
    Related .cc and .h files

simulations/
    omnetpp.ini
    
README.md                   # Project documentation
```

## Installation & Setup

### Prerequisites
- OMNeT++ 6.0 or later
- INET Framework (for basic network components)

### Building the Project
1. Clone or download the project files
2. Open OMNeT++ IDE
3. Import existing project into workspace
4. Build the project (Ctrl+B)

### Configuration
Edit `omnetpp.ini` to modify simulation parameters:
```ini
*.server.ipPool = "192.168.1.10-192.168.1.50"
*.server.leaseTime = 60s
*.server.friendlyNames = "AA:BB:CC:DD:EE:01=laptop, AA:BB:CC:DD:EE:02=desktop"
