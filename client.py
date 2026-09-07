import socket
import struct
import time

SERVER_IP = "127.0.0.1"
SERVER_PORT = 8080
TOTAL_ORDERS = 10000

# raw udp socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"Sending {TOTAL_ORDERS} UDP datagrams to {SERVER_IP}:{SERVER_PORT}...")
start_time = time.time()

for i in range(1, TOTAL_ORDERS + 1):
    # deterministic mock data
    is_buy = 1 if (i % 2 == 0) else 0
    price = float(100.0 + (i % 8))
    qty = int(10 + (i % 50))

    # Pack into 13 bytes to exactly match the C++ #pragma pack(1) struct
    # <Bdi = Little-Endian, uint8_t (1), double (8), int32_t (4)
    packet = struct.pack('<Bdi', is_buy, price, qty)

    # fire and forget
    sock.sendto(packet, (SERVER_IP, SERVER_PORT))

end_time = time.time()
elapsed = end_time - start_time

print(f"Done. Sent {TOTAL_ORDERS} packets in {elapsed:.4f} sec.")
sock.close()