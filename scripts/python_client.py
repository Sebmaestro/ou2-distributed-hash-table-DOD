import socket
import sys
import struct

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Wrong number of arguments')
        print('Correct usage:{} node-address node-port'.format(sys.argv[0]))
        exit()

    sock = socket.socket(type=socket.SOCK_DGRAM)
    sock.connect((sys.argv[1], int(sys.argv[2])))
    print("Created UDP socket to {}:{}".format(sys.argv[1], sys.argv[2]))
    listening_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    listening_socket.bind(('127.0.0.1', 0))
    listen_port = listening_socket.getsockname()[1]
    print('\nListening for response messages on port {}'.format(listen_port))

    while True:
        print('Type 1 for VAL_INSERT, 2 for VAL_LOOKUP, 3 for VAL_REMOVE')
        n = int(input())

        if n == 1:
            print('Insert value\n')
            print('SSN: ', end='')
            ssn = input()
            print('Name: ', end='')
            name = input()
            print('Email: ', end='')
            email = input()

            msg = b'\x64' + struct.pack('13sB1s{0}sB7s{1}s'
                                        .format(len(name) + 1, len(email) + 1),
                                        str.encode(ssn, 'ascii'),
                                        len(name) + 1,
                                        b'\x00', str.encode(name, 'ascii'),
                                        len(email) + 1,
                                        b'\x00'*7,
                                        str.encode(email, 'ascii'))
            sock.sendall(msg)
            print('Sent insert!')

        elif n == 2:

            print('Lookup value\n')
            print('SSN: ', end='')
            ssn = input()

            msg = b'\x66' + struct.pack('!13s16sH',
                                        str.encode(ssn, 'ascii'),
                                        str.encode('127.0.0.1', 'ascii'),
                                        listen_port)
            print(msg)
            sock.sendall(msg)
            print('Sent lookup!')
            data = listening_socket.recv(4096)
            _, ssn, namelen = struct.unpack("B13sB", data[:15])
            name, emaillen = struct.unpack("{}sB".format(namelen), data[16:17 + namelen])
            email = struct.unpack("{}s".format(emaillen), data[17+namelen+7:])
    
            print('Received response\nSSN: {}\nName: {}\nEmail: {}'.
                  format(ssn, name, email))

        elif n == 3:
            print('Remove value\n')
            print('SSN: ', end='')
            ssn = input()

            msg = b'\x65' + struct.pack('13s',
                                        str.encode(ssn, 'ascii'))
            print(msg)
            sock.sendall(msg)
            print('Sent remove!')
        else:
            print('Wrong value')
