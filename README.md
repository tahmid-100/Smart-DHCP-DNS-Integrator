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

SmartDhcpDns/
├── src/ # Source code files
│ ├── SmartServer.ned # Integrated DHCP-DNS server
│ ├── SmartClient.ned # Network client implementation
│ ├── SmartNetwork.ned # Network topology definition
│ └── message types # Protocol message definitions
├── simulations/ # Simulation configuration
│ └── omnetpp.ini # Simulation parameters
├── results/ # Simulation output and logs
├── images/ # Network diagrams and screenshots
└── documentation/ # Project report and documentation
