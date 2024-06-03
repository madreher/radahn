import zmq

def main():
    addr = "tcp://localhost:50000"
    context = zmq.Context()
    sock = context.socket(zmq.SUB)
    sock.connect(addr)
    sock.setsockopt(zmq.SUBSCRIBE, b'')

    while(True):
        print("Waiting for message...")
        msg = sock.recv_json()
        print(msg)

if __name__ == "__main__":
    main()