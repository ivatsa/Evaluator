#!/usr/bin/env python

import sys, socket, math

magic_string = "cs5700spring2013"
hello_msg = "%s HELLO %s\n"
solution_string = "%s %i\n"

hostname = sys.argv[1]
port = int(sys.argv[2])
neu = sys.argv[3]

def send_string(sock, string):
    sent = 0
    while sent < len(string):
        sent = sock.send(string[sent:])

def solve_math_problems(sock):
    rounds = 0

    while True:
        msg = ''
        while len(msg) == 0 or msg[-1] != '\n':
            msg += sock.recv(255)

        if msg.find('BYE') != -1:
            print "Received BYE message after", rounds, "math problems"
            return msg
        elif msg.startswith('cs5700spring2013 STATUS '):
            elem = msg.split()
            num1 = int(elem[2])
            op = elem[3]
            num2 = int(elem[4])
            
            if op == '+': solution = num1 + num2
            elif op == '-': solution = num1 - num2
            elif op == '*': solution = num1 * num2
            elif op == '/': solution = num1 / num2
            else:
                print "Unknown math operation:", op
                sys.exit()

            send_string(sock, solution_string % (magic_string, solution))
        else:
            print "Unknown message from server:", msg
            sys.exit()

        rounds += 1

try:
    sock = socket.create_connection((hostname, port))
except:
    print "Unable to connect to %s:%i" % (hostname, port)
    sys.exit()

send_string(sock, hello_msg % (magic_string, neu))
final_msg = solve_math_problems(sock)
print "Final message from server:", final_msg,
print "Secret key:", final_msg.split()[1]
