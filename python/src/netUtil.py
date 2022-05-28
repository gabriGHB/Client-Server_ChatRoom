"""Module containing the definition of the functions that will be in charge of sending data
to server socket, as well as connecting to the socket"""

# ******************** IMPORTS ***********************
import socket
from src import util


# ******************** FUNCTIONS *********************
def connect_socket(server_address: tuple):
    """Function in charge of connecting to the server socket"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(server_address)
        return sock
    except socket.error as ex:
        print(f"connect_socket fail: {ex}")


def send_header(sock, request):
    """Function in charge of sending the header to the server socket"""
    try:
        # send the op_code
        sock.sendall(request.header.op_code.encode('ascii'))
        sock.sendall(b'\0')

        # send the username
        sock.sendall(request.header.username.encode('ascii'))
        sock.sendall(b'\0')
    except socket.error as ex:
        print(f"send_header fail: {ex}")


def send_connection_request(sock, request):
    """Function in charge of sending the header and the destination port to the server socket, so that the connection
    can be performed """
    try:
        # first, send the header
        send_header(sock, request)
        # now, send also the port
        sock.sendall(request.item.listening_port.encode('ascii'))
        sock.sendall(b'\0')
    except socket.error as ex:
        print(f"send_connection_request fail: {ex}")


def send_message_request(sock, request):
    """Function in charge of sending the header, the recipient user and the message to the server socket"""
    try:
        # first, send the header
        send_header(sock, request)
        # we send the recipient user
        sock.sendall(request.item.recipient_username.encode('ascii'))
        sock.sendall(b'\0')
        # and also the message
        sock.sendall(request.item.message.encode('ascii'))
        sock.sendall(b'\0')
    except socket.error as ex:
        print(f"send_message_request fail: {ex}")


def receive_server_error_code(sock):
    """Function in charge of receiving a byte representing the error code from the server"""
    error_code = int.from_bytes(sock.recv(1), "big")
    return error_code


def receive_string(sock):
    """Function in charge of receiving a string from a given socket"""
    string = ''
    try:
        # we create a while loop in charge of receiving data from the socket
        while True:
            msg = sock.recv(1)
            # when the \0 character is reached, the string has ended, so it stops receiving characters
            if msg == b'\0':
                break
            # the received data is decoded and returned
            string += msg.decode()
        return string
    except socket.error as ex:
        print(f"receive_string fail: {ex}")


def listen_and_accept(sock):
    """Function in charge of listening at a free port and accepting the connection, receiving server replies"""
    sock.listen(1)
    # first, create the reply
    reply = util.Reply()
    connected = True
    while connected:
        # then, accept the connection
        connection, client_address = sock.accept()
        try:
            # receive the operation code of the server response
            reply.header.op_code = receive_string(connection)
            # in the case of a message:
            if reply.header.op_code == util.SEND_MESSAGE:
                reply.header.username = receive_string(connection)
                reply.item.message_id = receive_string(connection)
                reply.item.message = receive_string(connection)
                print(f"c> MESSAGE {reply.item.message_id} FROM {reply.header.username}:\n {reply.item.message}\nEND")
            # in case of a message acknowledgement:
            elif reply.header.op_code == util.SEND_MESS_ACK:
                reply.item.message_id = receive_string(connection)
                print(f"c> SEND MESSAGE {reply.item.message_id} OK")
            elif reply.header.op_code == util.END_LISTEN_THREAD:
                # end thread
                connected = False
            else:
                print("ERROR, INVALID OPERATION")
                break

        except socket.error as ex:
            print(f"listen_and_accept fail: {ex}")
            connection.close()
            sock.close()
            return None

        finally:
            connection.close()
            if not connected:
                sock.close()
