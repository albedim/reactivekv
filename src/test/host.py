import socket

HOST = "127.0.0.1"
PORT = 6379

def main():
  try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))
  except ConnectionRefusedError:
    print(f"Error: Could not connect to server at {HOST}:{PORT}")
    print("Make sure the server is running first!")
    return

  print(f"Connected to KV server on {HOST}:{PORT}")

  while True:
    cmd = input("> ")

    if cmd.lower() in ["exit", "quit"]:
      break

    try:
      sock.sendall(cmd.encode("utf-8"))
      data = sock.recv(1024)
      print("SERVER:", data.decode("utf-8").strip())
    except Exception as e:
      print(f"Error: {e}")
      break

  sock.close()

if __name__ == "__main__":
    main()