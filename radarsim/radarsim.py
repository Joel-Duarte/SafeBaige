import serial
import time
import random
import struct

SIM_PORT = '/tmp/radar_sim'

class LD2451_Simulator:
    def __init__(self):
        self.ser = serial.Serial(SIM_PORT, 115200, timeout=0.1)
        self.header = b'\xf4\xf3\xf2\xf1'
        self.trailer = b'\xf8\xf7\xf6\xf5'
        
    def generate_target_data(self, distance, speed, angle, approaching=True):
        angle_byte = int(angle + 128) & 0xFF # Angle: reported as (degree + 0x80) 
        dist_byte = int(distance) & 0xFF # Distance: 1 byte (m)
        dir_byte = 0x01 if approaching else 0x00 # Direction: 1 = approach, 0 = away
        speed_byte = int(speed) & 0xFF # Speed: 1 byte (km/h)
        snr_byte = random.randint(150, 255) if distance < 50 else random.randint(50, 150) # SNR: stronger signal for closer targets
        
        return struct.pack('BBBBB', angle_byte, dist_byte, dir_byte, speed_byte, snr_byte)

    def run(self):
        print(f"LD2451 Simulator Active on {SIM_PORT} (115200 baud)")
        while True:
            # Simulate idle state
            print("[STATUS] Road Clear")
            time.sleep(random.uniform(3, 6))

            # Simulate a car appearing
            dist = random.uniform(10, 100) # Car appears between 10m and 100m
            speed = random.uniform(30, 70)
            angle = random.uniform(-5, 5) # Car approaching mostly straight, small angle variation
            
            print(f"[DETECT] Car appearing at {dist}m, speed {speed:.1f} km/h")
            
            while dist > 2:
                # Simulate target data frame
                target_count = 1
                alarm_info = 1
                target_payload = self.generate_target_data(dist, speed, angle)
                
                # Payload length = target_count * 5 + 2 bytes (count + alarm)
                data_len = struct.pack('<H', len(target_payload) + 2)
                
                frame = (self.header + data_len + 
                         struct.pack('BB', target_count, alarm_info) + 
                         target_payload + self.trailer)
                
                self.ser.write(frame)
                print(f"[FRAME] Sent target at {dist:.1f}m, speed {speed:.1f} km/h")
                
                # Physics update (10Hz) - simple linear motion
                time.sleep(0.1)
                dist -= (speed / 3.6) * 0.1
                
            print("[STATUS] Car passed.")

if __name__ == "__main__":
    LD2451_Simulator().run()