import socket
import struct
import time

# ========================================================
# CONNECTION CONFIGURATION
# ========================================================
SERVER_IP = "127.0.0.1"  # Localhost
SERVER_PORT = 8080
TOTAL_ORDERS = 10000

# Initialize standard UDP datagram socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"🚀 Dispatching {TOTAL_ORDERS} binary orders to Matching Engine ({SERVER_IP}:{SERVER_PORT})...")
start_time = time.time()

for i in range(1, TOTAL_ORDERS + 1):
    # Generate deterministic market order payload
    is_buy = 1 if (i % 2 == 0) else 0  # 1 = Buy, 0 = Sell
    price = float(100.0 + (i % 8))     # 8-byte floating point
    qty = int(10 + (i % 50))           # 4-byte integer

    # ========================================================
    # PACKING PAYLOAD INTO 13 BYTES (Strict Little-Endian)
    # Format specifiers:
    #   < : Standard Little-Endian byte ordering (x86 architecture)
    #   B : uint8_t  (1 Byte  - Buy/Sell flag)
    #   d : double   (8 Bytes - Order Price)
    #   i : int32_t  (4 Bytes - Order Quantity)
    # Total: 1 + 8 + 4 = 13 Bytes (matches C++ #pragma pack(1))
    # ========================================================
    packet = struct.pack('<Bdi', is_buy, price, qty)

    # Transmit raw binary packet to C++ UDP Socket
    sock.sendto(packet, (SERVER_IP, SERVER_PORT))

end_time = time.time()
elapsed = end_time - start_time

print(f"✅ Finished! Sent {TOTAL_ORDERS} packets in {elapsed:.4f} seconds.")
sock.close()