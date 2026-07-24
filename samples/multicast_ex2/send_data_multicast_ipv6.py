import socket
import struct

def send_hex_multicast_ipv6(hex_data, multicast_group, multicast_port, interface_index=None):
    """
    Send hex data to an IPv6 multicast address.
    
    Args:
        hex_data: String of hex data (e.g., "deadbeef" or "DE AD BE EF")
        multicast_group: IPv6 multicast address (e.g., "ff02::1")
        multicast_port: Port number (e.g., 5555)
        interface_index: Interface index for IPv6 (optional, 0 = default)
    """
    # Parse hex string to bytes
    hex_data = hex_data.replace(" ", "")
    try:
        data = bytes.fromhex(hex_data)
    except ValueError:
        print(f"Error: Invalid hex data: {hex_data}")
        return False

    try:
        # Create IPv6 UDP socket
        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        
        # Allow reusing the address
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # Set multicast hop limit (IPv6 equivalent of TTL)
        hop_limit = 32
        sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_MULTICAST_HOPS, hop_limit)
    
        # Set the interface for multicast (optional)
        if interface_index is not None:
            sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_MULTICAST_IF, interface_index)

        print(f"Sending to [{multicast_group}]:{multicast_port} on interface index {interface_index if interface_index is not None else 'default'}\n")

        # Send data to multicast group
        sock.sendto(data, (multicast_group, multicast_port, 0, 0))
        print(f"Sent {len(data)} bytes to [{multicast_group}]:{multicast_port}")
        print(f"Hex data: {hex_data}")
        
        sock.close()
        return True
        
    except Exception as e:
        print(f"Error: {e}")
        return False


# Example usage
if __name__ == "__main__":
    # Configuration
    MULTICAST_GROUP_IPV6 = "ff14::0004:0000"      # IPv6 link-local multicast address
    MULTICAST_PORT = 22222                # Multicast port
    HEX_DATA = "DEADBEEF CAFEBABE 12345678"  # Example hex data
    
    # Optional: specify interface index (None = default interface)
    # On Linux, you can find interface indices with: ip link show
    INTERFACE_INDEX = 8
    
    # Send the data
    send_hex_multicast_ipv6(HEX_DATA, MULTICAST_GROUP_IPV6, MULTICAST_PORT, INTERFACE_INDEX)
