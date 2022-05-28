import unittest
import os
from client import Client
from src import util


class MyTestCase(unittest.TestCase):
    def test_all_services_except_send(self):
        # first, we create both clients
        client_a = Client()
        client_a.server = os.getenv("SERVER_IP")
        client_a.port = int(os.getenv("SERVER_PORT"))

        client_b = Client()
        client_b.server = os.getenv("SERVER_IP")
        client_b.port = int(os.getenv("SERVER_PORT"))

        # register tests
        # register both users, a and b
        self.assertEqual(client_a.register("a"), util.EC.SUCCESS.value)
        self.assertEqual(client_b.register("b"), util.EC.SUCCESS.value)
        # try to re-register user-b
        self.assertEqual(client_b.register("b"), util.EC.REGISTER_USR_ALREADY_REG.value)

        # connect tests
        # connect one user
        self.assertEqual(client_a.connect("a"), util.EC.SUCCESS.value)
        # try to reconnect
        self.assertEqual(client_a.connect("a"), util.EC.CONNECT_USR_ALREADY_CN.value)
        # try to connect a non-existent user
        self.assertEqual(client_b.connect("c"), util.EC.CONNECT_USR_NOT_EXISTS.value)
        self.assertEqual(client_b.connect("b"), util.EC.SUCCESS.value)
        # try to connect another user while there is a user already connected
        self.assertEqual(client_b.connect("c"), util.EC.CONNECT_DIFF_USR_CN.value)

        # disconnect tests
        # disconnects from user-b successfully
        self.assertEqual(client_b.disconnect("b"), util.EC.SUCCESS.value)
        # try to disconnect again from user-b, which was disconnected before
        self.assertEqual(client_b.disconnect("b"), util.EC.DISCONNECT_USR_NOT_CN.value)
        # try to disconnect a different user than the one that is actually connected
        self.assertEqual(client_a.disconnect("c"), util.EC.DISCONNECT_DIFF_USR_CN.value)
        # disconnects from user-a successfully
        self.assertEqual(client_a.disconnect("a"), util.EC.SUCCESS.value)
        # try to disconnect from a non-existing user
        self.assertEqual(client_a.disconnect("c"), util.EC.DISCONNECT_USR_NOT_EXISTS.value)

        # unregister tests
        # try to unregister non-existent user
        self.assertEqual(client_a.unregister("c"), util.EC.UNREGISTER_USR_NOT_EXISTS.value)
        self.assertEqual(client_a.unregister("a"), util.EC.SUCCESS.value)
        self.assertEqual(client_a.unregister("b"), util.EC.SUCCESS.value)

    def test_send_service(self):
        # first, we create both clients
        client_a = Client()
        client_a.server = os.getenv("SERVER_IP")
        client_a.port = int(os.getenv("SERVER_PORT"))

        client_b = Client()
        client_b.server = os.getenv("SERVER_IP")
        client_b.port = int(os.getenv("SERVER_PORT"))

        # case 1: message sent successfully between two connected users
        # first, register both users
        self.assertEqual(client_a.register("a"), util.EC.SUCCESS.value)
        self.assertEqual(client_b.register("b"), util.EC.SUCCESS.value)
        # connect both users
        self.assertEqual(client_a.connect("a"), util.EC.SUCCESS.value)
        self.assertEqual(client_b.connect("b"), util.EC.SUCCESS.value)

        # send message from user-a to user-b
        msg1 = "1"
        print(f"Message sent from a to b: {msg1}")
        self.assertEqual(client_a.send("b", msg1), util.EC.SUCCESS.value)
        # send message from user-b to user-a
        msg2 = "2"
        print(f"Message sent from b to a: {msg2}")
        self.assertEqual(client_b.send("a", msg2), util.EC.SUCCESS.value)

        # case 2: user-a tries to send message to user-c which is not registered
        self.assertEqual(client_a.send("c", msg1), util.EC.SEND_USR_NOT_EXISTS.value)

        # case 3: message containing additional blank spaces from user-a to user-b
        msg3 = "1 2  3   4    5"
        print(f"Message sent from a to b: {msg3}")
        self.assertEqual(client_a.send("b", msg3), util.EC.SUCCESS.value)

        # case 4: user-a tries to send message to user-b, which is disconnected
        # disconnect user-b
        self.assertEqual(client_b.disconnect("b"), util.EC.SUCCESS.value)
        # send message from user-a to user-b (disconnected)
        msg4 = "4"
        print(f"Message sent from a to b: {msg4}")
        self.assertEqual(client_a.send("b", msg4), util.EC.SUCCESS.value)
        # user-b connects and receives its pending message
        self.assertEqual(client_b.connect("b"), util.EC.SUCCESS.value)

        # unregister both users (cleanup)
        self.assertEqual(client_a.unregister("a"), util.EC.SUCCESS.value)
        self.assertEqual(client_b.unregister("b"), util.EC.SUCCESS.value)


if __name__ == '__main__':
    unittest.main()
