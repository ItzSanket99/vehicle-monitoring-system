from pathlib import Path
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import mm
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle,
    PageBreak, KeepTogether
)

OUT = Path("docs")
OUT.mkdir(exist_ok=True)

styles = getSampleStyleSheet()
styles.add(ParagraphStyle(
    name="CoverTitle", parent=styles["Title"],
    fontSize=23, leading=29, alignment=TA_CENTER,
    textColor=colors.HexColor("#163A5F"), spaceAfter=18
))
styles.add(ParagraphStyle(
    name="CoverSub", parent=styles["Normal"],
    fontSize=12, leading=18, alignment=TA_CENTER,
    spaceAfter=10
))
styles.add(ParagraphStyle(
    name="Section", parent=styles["Heading1"],
    textColor=colors.HexColor("#163A5F"),
    spaceBefore=12, spaceAfter=7
))
styles.add(ParagraphStyle(
    name="SubSection", parent=styles["Heading2"],
    textColor=colors.HexColor("#246B8E"),
    spaceBefore=8, spaceAfter=5
))
styles.add(ParagraphStyle(
    name="BodyCustom", parent=styles["BodyText"],
    fontSize=9.5, leading=14, spaceAfter=7
))
styles.add(ParagraphStyle(
    name="SmallCustom", parent=styles["BodyText"],
    fontSize=8, leading=11
))

def P(text, style="BodyCustom"):
    return Paragraph(text, styles[style])

def bullet(text):
    return P("&#8226;&nbsp; " + text)

def table(rows, widths=None):
    converted = []
    for row in rows:
        converted.append([
            cell if isinstance(cell, Paragraph) else P(str(cell), "SmallCustom")
            for cell in row
        ])
    t = Table(converted, colWidths=widths, repeatRows=1, hAlign="LEFT")
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#163A5F")),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
        ("GRID", (0, 0), (-1, -1), 0.45, colors.HexColor("#B8C7D1")),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1),
         [colors.white, colors.HexColor("#F1F5F8")]),
        ("LEFTPADDING", (0, 0), (-1, -1), 6),
        ("RIGHTPADDING", (0, 0), (-1, -1), 6),
        ("TOPPADDING", (0, 0), (-1, -1), 6),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 6),
    ]))
    return t

def footer(canvas, doc):
    canvas.saveState()
    canvas.setStrokeColor(colors.HexColor("#B8C7D1"))
    canvas.line(18*mm, 15*mm, 192*mm, 15*mm)
    canvas.setFont("Helvetica", 8)
    canvas.setFillColor(colors.HexColor("#526575"))
    canvas.drawString(18*mm, 10*mm, "Vehicle Monitoring System | Linux Socket Programming")
    canvas.drawRightString(192*mm, 10*mm, f"Page {doc.page}")
    canvas.restoreState()

def build(filename, title, subtitle, story):
    doc = SimpleDocTemplate(
        str(OUT / filename), pagesize=A4,
        rightMargin=18*mm, leftMargin=18*mm,
        topMargin=18*mm, bottomMargin=22*mm,
        title=title, author="Student Project"
    )
    cover = [
        Spacer(1, 35*mm),
        P("DEVELOPMENT OF AN", "CoverSub"),
        P("Ethernet-Based Vehicle<br/>Monitoring System", "CoverTitle"),
        P("Using Linux Socket Programming in C", "CoverSub"),
        Spacer(1, 12*mm),
        P(subtitle, "CoverSub"),
        Spacer(1, 18*mm),
        P("Project platform: Pop!_OS / Linux", "CoverSub"),
        P("Communication: TCP and UDP sockets", "CoverSub"),
        PageBreak()
    ]
    doc.build(cover + story, onFirstPage=footer, onLaterPages=footer)

# -------------------------------------------------------------------
# 1. DESIGN DOCUMENT
# -------------------------------------------------------------------
s = [
    P("1. Introduction", "Section"),
    P("This project implements a software-based vehicle monitoring system "
      "using Linux socket programming in C. A Vehicle ECU application generates "
      "vehicle signals and sends them to a Dashboard ECU application, which "
      "receives, validates, and displays the values."),
    P("2. Objectives", "Section"),
    bullet("Generate vehicle speed, engine RPM, and steering angle periodically."),
    bullet("Implement UDP datagram communication and TCP stream communication."),
    bullet("Inspect application traffic using Wireshark."),
    bullet("Evaluate normal operation and selected failure scenarios."),
    P("3. System Architecture", "Section"),
    table([
        ["Component", "Responsibility"],
        ["Vehicle ECU", "Generates vehicle data and transmits it to the dashboard."],
        ["Dashboard ECU", "Receives data, validates message fields, and displays readings."],
        ["Linux sockets", "Provide the application interface to UDP and TCP networking."],
        ["Wireshark", "Captures and decodes packets for analysis."]
    ], [38*mm, 136*mm]),
    Spacer(1, 5*mm),
    P("4. Communication Design", "Section"),
    P("<b>UDP mode:</b> The dashboard binds to 127.0.0.1:5005. The vehicle application "
      "sends datagrams once per second. Each datagram contains a comma-separated "
      "speed, RPM, and steering angle."),
    P("<b>TCP mode:</b> The dashboard listens on 127.0.0.1:5006. The vehicle "
      "establishes a connection and sends newline-terminated records over a "
      "TCP byte stream. The dashboard accumulates received bytes and processes "
      "complete lines."),
    P("5. Vehicle Data Format", "Section"),
    table([
        ["Signal", "Unit", "Example"],
        ["Vehicle speed", "km/h", "42"],
        ["Engine RPM", "rpm", "1800"],
        ["Steering angle", "degrees", "-20"],
        ["Message", "CSV-style text", "42,1800,-20"]
    ], [45*mm, 45*mm, 84*mm]),
    P("The current application uses simulated values for demonstration; it is "
      "not connected to real vehicle sensors or an actual automotive ECU."),
    P("6. Socket Operation", "Section"),
    table([
        ["Vehicle ECU", "Dashboard ECU"],
        ["Create socket", "Create socket"],
        ["Configure destination", "Bind local address and port"],
        ["TCP: connect / UDP: sendto", "TCP: listen and accept / UDP: recvfrom"],
        ["Transmit periodic messages", "Receive and validate messages"],
        ["Close socket", "Close client and listening socket"]
    ], [87*mm, 87*mm]),
    P("7. Error Handling and Limitations", "Section"),
    P("The programs check socket creation, address parsing, binding, and key "
      "send/receive operations. The dashboard performs basic range and format "
      "validation. UDP does not guarantee delivery. TCP provides a reliable, "
      "ordered byte stream, but the application must frame its messages."),
    P("The demonstration uses loopback networking on one Linux host. A deployment "
      "across separate ECUs would require appropriate interface addresses, routing, "
      "firewall rules, and network security."),
    P("8. Build and Execution", "Section"),
    P("Build using <font name='Courier'>make</font>. Run "
      "<font name='Courier'>./build/dashboard_ecu udp</font> in one terminal and "
      "<font name='Courier'>./build/vehicle_ecu udp</font> in another. For TCP, "
      "run <font name='Courier'>./build/dashboard_ecu tcp</font> and "
      "<font name='Courier'>./build/vehicle_ecu tcp</font>."),
    P("9. Conclusion", "Section"),
    P("The design demonstrates application-layer vehicle data exchange using "
      "Linux UDP and TCP sockets, with packet inspection performed separately "
      "using Wireshark.")
]
build("Design_Document.pdf", "Design Document",
      "System Architecture and Implementation Design", s)

# -------------------------------------------------------------------
# 2. VALIDATION REPORT
# -------------------------------------------------------------------
s = [
    P("1. Purpose", "Section"),
    P("This report defines the validation scenarios for the Vehicle Monitoring "
      "System. Actual outputs, packet captures, and screenshots should be retained "
      "as evidence. A scenario must not be marked as passed until its result has "
      "been observed and recorded."),
    P("2. Test Environment", "Section"),
    table([
        ["Item", "Configuration"],
        ["Operating system", "Pop!_OS 24.04 LTS"],
        ["Language", "C"],
        ["Build tools", "GCC and Make"],
        ["Capture tool", "Wireshark / dumpcap"],
        ["UDP endpoint", "127.0.0.1:5005"],
        ["TCP endpoint", "127.0.0.1:5006"]
    ], [50*mm, 124*mm]),
    P("3. Validation Scenarios", "Section"),
    table([
        ["Scenario", "Procedure", "Evidence to record"],
        ["1. Normal communication",
         "Start dashboard and vehicle in the same protocol mode.",
         "Sender and receiver output; packet capture."],
        ["2. Dashboard unavailable",
         "Stop dashboard and attempt UDP sending and TCP connection.",
         "Actual sender output and TCP connection error."],
        ["3. Extended transmission",
         "Run both applications for a defined period, e.g. 60 seconds.",
         "Duration, messages sent, messages received, observed issues."],
        ["4. Invalid configuration",
         "Provide an invalid IPv4 address and invalid port.",
         "Actual exit status and error messages."],
        ["5. TCP/UDP comparison",
         "Capture both modes and compare packet behaviour.",
         "Handshake, data, teardown, ports and payload evidence."]
    ], [35*mm, 73*mm, 66*mm]),
    P("4. Results Record", "Section"),
    P("Complete the following table using the actual test output. Replace each "
      "pending entry with Pass or Fail and reference the associated evidence."),
    table([
        ["Test", "Status", "Observed result / evidence"],
        ["Normal UDP", "Pending", "Record receiver output and UDP capture."],
        ["Normal TCP", "Pending", "Record receiver output and TCP capture."],
        ["Dashboard unavailable", "Pending", "Record UDP and TCP behaviour."],
        ["Extended duration", "Pending", "Record duration and actual message counts."],
        ["Invalid IP", "Pending", "Record actual program output."],
        ["Invalid port", "Pending", "Record actual program output."],
        ["Wireshark analysis", "Pending", "Reference saved captures and screenshots."]
    ], [42*mm, 27*mm, 105*mm]),
    P("5. Packet Analysis Checklist", "Section"),
    bullet("UDP: source IP, destination IP, source port, destination port, payload."),
    bullet("TCP: SYN, SYN-ACK, ACK, data transfer, and connection termination."),
    bullet("Check that captured packets correspond to the application ports."),
    bullet("Save genuine screenshots and packet captures in the repository."),
    P("6. Limitations", "Section"),
    P("Loopback testing validates local socket communication, not physical Ethernet "
      "hardware, external network performance, or communication between separate "
      "ECUs. Test outcomes depend on the actual execution environment."),
    P("7. Conclusion", "Section"),
    P("Finalize this report after all tests have been run and evidence has been "
      "reviewed. Pending scenarios must remain clearly identified until verified.")
]
build("Validation_Report.pdf", "Validation Report",
      "Test Procedures, Observations and Evidence", s)

# -------------------------------------------------------------------
# 3. TCP / UDP COMPARISON
# -------------------------------------------------------------------
s = [
    P("1. Overview", "Section"),
    P("TCP and UDP are transport-layer protocols that applications can use to "
      "exchange vehicle monitoring data. The choice depends on delivery guarantees, "
      "latency requirements, and application design."),
    P("2. Comparative Table", "Section"),
    table([
        ["Parameter", "TCP", "UDP"],
        ["Reliability",
         "Acknowledgements, retransmission, and ordered byte-stream delivery.",
         "No built-in delivery or ordering guarantee."],
        ["Connection establishment",
         "Connection-oriented; normally uses a three-way handshake.",
         "Connectionless; datagrams can be sent without a handshake."],
        ["Overhead",
         "Larger transport header and connection state; may include ACKs and retransmissions.",
         "Smaller base header and no connection state in the transport protocol."],
        ["Error handling",
         "Detects segment corruption and retransmits lost data as needed.",
         "Checksum detects corruption where used; delivery recovery is application-specific."],
        ["Data model",
         "Byte stream; application must frame messages.",
         "Datagrams preserve message boundaries."],
        ["Latency characteristics",
         "Retransmission and congestion control can add delay.",
         "Lower protocol overhead, but delivery is not guaranteed."],
        ["Typical usage",
         "Web sessions, file transfer, commands that require reliable delivery.",
         "Telemetry, discovery, real-time updates where occasional loss is acceptable."]
    ], [32*mm, 71*mm, 71*mm]),
    P("3. Application to Vehicle Monitoring", "Section"),
    P("In this demonstration, UDP sends periodic speed, RPM, and steering readings "
      "to port 5005. It is simple and has low protocol overhead, but a lost datagram "
      "is not automatically retransmitted."),
    P("The TCP implementation sends the same readings over a connection to port "
      "5006. TCP provides ordered reliable delivery while the connection remains "
      "viable, but it does not preserve application message boundaries. The newline "
      "delimiter in the demonstration provides message framing."),
    P("4. Interpreting Wireshark Captures", "Section"),
    bullet("UDP: inspect datagram ports, length, and the application payload."),
    bullet("TCP: identify SYN, SYN-ACK, ACK, subsequent data, and FIN/ACK teardown."),
    bullet("A single application message may span multiple TCP segments, or several "
           "messages may be carried in a segment."),
    bullet("Loopback captures are suitable for demonstrating socket behaviour, but "
           "they do not measure physical Ethernet performance."),
    P("5. Conclusion", "Section"),
    P("TCP is appropriate when reliable ordered delivery is required. UDP is useful "
      "for compact, low-overhead datagrams when the application can tolerate loss or "
      "implements its own recovery. The correct choice depends on the vehicle system's "
      "functional and timing requirements.")
]
build("TCP_UDP_Comparison.pdf", "TCP and UDP Comparison",
      "Transport Protocol Behaviour for Vehicle Monitoring", s)

print("Generated:")
for name in (
    "Design_Document.pdf",
    "Validation_Report.pdf",
    "TCP_UDP_Comparison.pdf"
):
    print(f"  docs/{name}")
