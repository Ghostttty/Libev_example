import socket
import argparse
import time

def test_connection(address, port, message, timeout=5):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(timeout)
            s.connect((address, port))
            
            s.sendall(message.encode())
            print(f"[→] Sent: {message}")
            
            data = s.recv(1024)
            response = data.decode()
            print(f"[←] Received: {response}")
            
            return response == message
            
    except Exception as e:
        print(f"[!] Error: {str(e)}")
        return False

def main():
    parser = argparse.ArgumentParser(description='TCP Client for testing non-blocking server')
    parser.add_argument('--address', type=str, default='127.0.0.1', help='Server IP address')
    parser.add_argument('--port', type=int, default=5000, help='Server port')
    parser.add_argument('--message', type=str, default='test123', help='Message to send')
    parser.add_argument('--repeat', type=int, default=1, help='Number of test iterations')
    parser.add_argument('--delay', type=float, default=0.5, help='Delay between requests')
    
    args = parser.parse_args()

    print(f"Testing server {args.address}:{args.port}")
    print(f"Message: '{args.message}', iterations: {args.repeat}\n")

    success_count = 0
    for i in range(1, args.repeat+1):
        print(f"Attempt {i}/{args.repeat}...")
        if test_connection(args.address, args.port, args.message):
            success_count += 1
            print("✓ Success\n")
        else:
            print("✗ Failed\n")
        time.sleep(args.delay)

    print(f"Results: {success_count}/{args.repeat} successful")
    if success_count == args.repeat:
        print("✅ All tests passed!")
    else:
        print("❌ Some tests failed")

if __name__ == "__main__":
    main()