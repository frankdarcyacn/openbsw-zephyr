import socket
import struct

def send_hex_multicast(hex_data, multicast_group, multicast_port, interface=None):
    """
    Send hex data to a multicast address.
    
    Args:
        hex_data: String of hex data (e.g., "deadbeef" or "DE AD BE EF")
        multicast_group: Multicast IP address (e.g., "239.255.0.1")
        multicast_port: Port number (e.g., 5555)
        interface: Local interface IP to bind to (optional)
    """
    # Parse hex string to bytes
    hex_data = hex_data.replace(" ", "")
    try:
        data = bytes.fromhex(hex_data)
    except ValueError:
        print(f"Error: Invalid hex data: {hex_data}")
        return False
    
    try:
        # Create UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        
        # Allow reusing the address
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # Set multicast TTL (Time To Live)
        ttl = struct.pack('B', 32)  # TTL = 32 hops
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl)
        
        # Optionally bind to specific interface
        if interface:
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF,
                          socket.inet_aton(interface))
        
        # Send data to multicast group
        sock.sendto(data, (multicast_group, multicast_port))
        print(f"Sent {len(data)} bytes to {multicast_group}:{multicast_port}")
        print(f"Hex data: {hex_data}")
        
        sock.close()
        return True
        
    except Exception as e:
        print(f"Error: {e}")
        return False


# Example usage
if __name__ == "__main__":
    # Configuration
    MULTICAST_GROUP = "239.10.20.30"  # Multicast IP address
    MULTICAST_PORT = 30490             # Multicast port
    HEX_DATA = "DEADBEEF CAFEBABE 12345678"  # Example hex data
    
    # Optional: specify local interface IP (leave as None for default)
    LOCAL_INTERFACE =  "192.0.2.2"
    
    # Send the data
    send_hex_multicast(HEX_DATA, MULTICAST_GROUP, MULTICAST_PORT, LOCAL_INTERFACE)
