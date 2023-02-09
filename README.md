# EITN30

TODO:
- Maximise throughput: 1.2Mbps
- System architecture
    - Main thread
    - Thread 1: IP -> radio (read IP packets, fragment and send over radio)
    - Thread 2: radio -> IP (listen and receive on radio, assemble and write to TUN device)
- Individual goals in Canvas: Friday Feb 10


INDIVIDUAL GOAL IDEAS:
1. Stream a video from the mobile unit
    - Link for download, many browsers offer to stream instead of download
    - Bandwidth
2. Minimize latency / maximize bandwidth
