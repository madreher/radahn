from flask import Flask, render_template 
import logging
import threading
from threading import Lock
import zmq
from flask_socketio import SocketIO

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
app.logger.setLevel(logging.INFO)
socketio = SocketIO(app)
#app = None
#socketio = None

thread = None
thread_lock = Lock()
threadAtoms = None
thread_lockAtoms = Lock()

@app.route('/')
def index():
    return render_template('index.html')

def listen_to_zmq_socket():
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    socket.connect("tcp://localhost:50000")
    socket.setsockopt(zmq.SUBSCRIBE, b"")
    app.logger.info("Listening for ZMQ messages on tcp://localhost:50000")
    while True:
        message = socket.recv()
        #app.logger.info("Received a message from ZMQ on python side: %s", message)
        socketio.emit('zmq_message', {'message': message.decode('utf-8')})
        #time.sleep(1)  # wait for 1 second before receiving the next message

def listen_to_zmq_socketAtoms():
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    socket.connect("tcp://localhost:50001")
    socket.setsockopt(zmq.SUBSCRIBE, b"")
    app.logger.info("Listening for ZMQ messages on tcp://localhost:50001")
    while True:
        message = socket.recv()
        #app.logger.info("Received a message from ZMQ on python side: %s", message)
        socketio.emit('zmq_message_atoms', {'message': message.decode('utf-8')})
        #time.sleep(1)  # wait for 1 second before receiving the next message

# Receive the test request from client and send back a test response
@socketio.on('start_listening')
def handle_start_listening():
    app.logger.info("Received a request to start listening to the simulation.")
    global thread
    with thread_lock:
        if thread is None:
            thread = socketio.start_background_task(listen_to_zmq_socket)
            app.logger.info("Background thread listening for KVS started.")
        else:
            app.logger.info("Background task listening to KVS already started.")

    global threadAtoms
    with thread_lockAtoms:
        if threadAtoms is None:
            threadAtoms = socketio.start_background_task(listen_to_zmq_socketAtoms)
            app.logger.info("Background thread listening for atoms started.")
        else:
            app.logger.info("Background task listening to atoms already started.")
    #emit('test_response', {'data': 'Test response sent'})

#def run_zmq_thread():
#    thread = threading.Thread(target=listen_to_zmq_socket)
#    thread.daemon = True
#    thread.start()



if __name__ == '__main__':
    #app = Flask(__name__)
    #app.config['SECRET_KEY'] = 'secret!'
    #socketio = SocketIO(app)
    #run_zmq_thread()
    socketio.run(app, host='localhost', port=5000)