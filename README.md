
# Ethernet-Based Vehicle Monitoring System

A Linux socket programming project implementing vehicle-to-dashboard
communication in C using TCP and UDP.

## Objectives

- Generate vehicle signals periodically.
- Transmit vehicle data using UDP sockets.
- Implement reliable TCP client-server communication.
- Display received vehicle information on the dashboard.
- Analyse TCP and UDP traffic using Wireshark.
- Validate normal operation and failure scenarios.

## Vehicle Signals

- Vehicle speed (km/h)
- Engine RPM (rpm)
- Steering angle (degrees)

## Repository Structure

```text
vehicle-monitoring-system/
├── README.md
├── Makefile
├── src/
│   ├── vehicle_ecu.c
│   └── dashboard_ecu.c
├── docs/
├── screenshots/
├── wireshark/
└── results/