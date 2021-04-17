import socket

s = socket.socket()
host = input("server ip:")
port = 12345
s.bind((host, port))

s.listen(5)
while True:
    print("listening")
    c, addr = s.accept()
    print('Got connection from %', addr)
    ans = 'connection established between parent and child network namespace'
    c.send(bytes(ans,'utf-8'))
    c.close()
