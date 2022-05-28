"""Module containing the abstraction of both requests and replies messages of the client.
 It also includes the definition of the operation codes"""
from enum import Enum

# ******************** OPERATION CODES *********************
# services called by client, served by server
REGISTER = 'REGISTER'
UNREGISTER = 'UNREGISTER'
CONNECT = 'CONNECT'
DISCONNECT = 'DISCONNECT'
SEND = 'SEND'
QUIT = 'QUIT'
TEST = "TEST"

# services called by server, served by client
SEND_MESSAGE = 'SEND_MESSAGE'
SEND_MESS_ACK = 'SEND_MESS_ACK'

# op code used to end the client receiving thread
END_LISTEN_THREAD = "END_LISTEN_THREAD"


# ******************** TYPES *********************
class EC(Enum):
    """
    Class used to represent service error codes.
    ...
    Attributes
    ----------
    SUCCESS: int
        general success code for any operation
    REGISTER: enum
        error codes for REGISTER service
    UNREGISTER: enum
        error codes for UNREGISTER service
    CONNECT: enum
        error codes for CONNECT service
    DISCONNECT: enum
        error codes for DISCONNECT service
    SEND: enum
        error codes for SEND service
 """

    SUCCESS = 0

    REGISTER_USR_ALREADY_REG = 1
    REGISTER_ANY = 2

    UNREGISTER_USR_NOT_EXISTS = 1
    UNREGISTER_ANY = 2

    CONNECT_USR_NOT_EXISTS = 1
    CONNECT_USR_ALREADY_CN = 2
    CONNECT_ANY = 3
    CONNECT_DIFF_USR_CN = 4     # extra

    DISCONNECT_USR_NOT_EXISTS = 1
    DISCONNECT_USR_NOT_CN = 2
    DISCONNECT_ANY = 3
    DISCONNECT_DIFF_USR_CN = 4  # extra

    SEND_USR_NOT_EXISTS = 1
    SEND_ANY = 2


class Header:
    """
        Class used to represent a request header.
        ...
        Attributes
        ----------
        op_code: str
            operation code used to represent the desired operation to be performed
        username: str
            name of the client
     """
    _op_code = str
    _username = str

    @property
    def op_code(self):
        return self._op_code

    @op_code.setter
    def op_code(self, value):
        self._op_code = value

    @property
    def username(self):
        return self._username

    @username.setter
    def username(self, value):
        self._username = value


class Item:
    """
        Class used to represent a request item.
        ...
        Attributes
        ----------
        listening_port: str
            port in which a thread will be listening to receive server replies
        recipient_username: str
            username of the client that should receive the message
        message: str
            message to be sent to a client
    """
    _listening_port = str
    _recipient_username = str
    _message = str
    _message_id = str

    @property
    def listening_port(self):
        return self._listening_port

    @listening_port.setter
    def listening_port(self, value):
        self._listening_port = value

    @property
    def recipient_username(self):
        return self._recipient_username

    @recipient_username.setter
    def recipient_username(self, value):
        self._recipient_username = value

    @property
    def message(self):
        return self._message

    @message.setter
    def message(self, value):
        self._message = value

    @property
    def message_id(self):
        return self._message_id

    @message_id.setter
    def message_id(self, value):
        self._message_id = value


class Request(object):
    """
        Class used to represent a request.
        ...
        Attributes
        ----------
        header: class
            header containing the username and operation code
        item: class
            item containing the listening port of the client and the message to be sent
    """

    _header = Header()
    _item = Item()

    @property
    def header(self):
        return self._header

    @property
    def item(self):
        return self._item


class Reply:
    """
        Class used to represent a server reply.
        ...
        Attributes
        ----------
        server_error_code: str
            error code sent by the server
         header: class
            header containing the username and operation code
        item: class
            item containing the listening port of the client and the message to be sent
    """

    _header = Header()
    _server_error_code = str
    _item = Item()

    @property
    def header(self):
        return self._header

    @property
    def item(self):
        return self._item

    @property
    def server_error_code(self):
        return self._server_error_code

    @server_error_code.setter
    def server_error_code(self, value):
        self._server_error_code = value
