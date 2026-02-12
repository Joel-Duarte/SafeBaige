# Simple HLK-LD2451 radar simulator

Simulates serial output of the radar module and mocks vehicle approach behavior through real world hardware communication protocols. With mock data simulating car approaches againt a moving point of reference.

## Usage

Set up socat for the virual serial bridge (connects to /tmp/radar_app at 115200 baud):

```bash
socat -d -d pty,raw,echo=0,link=/tmp/radar_sim pty,raw,echo=0,link=/tmp/radar_app
```

then just run radarsim.py (needs pyserial)


## Data format

TODO