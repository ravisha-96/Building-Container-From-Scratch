import socket
s = socket.socket()

#host = '192.10.11.12'
host = raw_input("Enter server listen ip:")
port = 12345
s.connect((host,port))
print s.recv(1024)
s.close()
