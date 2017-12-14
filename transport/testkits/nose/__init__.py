#from Config import config
import subprocess
import time
import os
import json
import sys

unittest = None


def setUp():
    print '__init__.setUp'
    global unittest
    unittest = subprocess.Popen(
        ['./wsf-transport-unittest'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE)
    print unittest
    while True:
        r = unittest.stdout.readline().strip()
        print r
        if r == "Enter 'q' or 'Q' to exit.":
            break
    print '---start test---'


def tearDown():
    print '__init__.tearDown'
    global unittest
    unittest.communicate("q")
    count = 0
    while None == unittest.poll():
        time.sleep(0.1)
        count += 1
        assert count < 10, "wsf-actor-unittest not exit in 1 second "
