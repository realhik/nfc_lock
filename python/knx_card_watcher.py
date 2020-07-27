#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys,os

import datetime
import yaml
import zmq
from zmq.eventloop.zmqstream import ZMQStream

from EIBConnection import EIBConnection, EIBBuffer, EIBAddr

class card_watcher(object):
    mainloop = None
    config_file = None
    config = {}
    zmq_context = None
    relay_disable_timer = None
    fobblogdb = None
    led_timer = None
    
    def __init__(self, config_file, mainloop):
        self.config_file = config_file
        self.mainloop = mainloop
        self.reload()
        print("Initialized")

    @staticmethod
    def groupAddrToBytes(groupAddr):
        ga = groupAddr.strip().split('/')
        main = int(ga[0])
        mid  = int(ga[1])
        sub  = int(ga[2])
        ga = (main << 11) | (mid << 8) | (sub & 0xff)
        return ga

    def hook_signals(self):
        """Hooks POSIX signals to correct callbacks, call only from the main thread!"""
        import signal as posixsignal
        posixsignal.signal(posixsignal.SIGTERM, self.quit)
        posixsignal.signal(posixsignal.SIGQUIT, self.quit)
        posixsignal.signal(posixsignal.SIGHUP, self.reload)

    def log(self, card_uid, card_valid):
        return

    def reload(self, *args):
        with open(self.config_file) as f:
            self.config = yaml.load(f, Loader=yaml.SafeLoader)

        if self.zmq_context:
            self.zmq_context.destroy()
            self.zmq_context = None
        self.zmq_context = zmq.Context()
        self.zmq_socket = self.zmq_context.socket(zmq.SUB)
        self.zmq_socket.connect(self.config['gatekeeper_socket'])
        #subscribe all topics
        self.zmq_socket.setsockopt(zmq.SUBSCRIBE, b'')
        self.zmq_stream = ZMQStream(self.zmq_socket)
        self.zmq_stream.on_recv(self._on_recv)

        self.__knx = EIBConnection()
        self.__knx.EIBSocketURL(self.config['knx']['url'])
        self.__knx.EIBOpen_GroupSocket(write_only = 0)

        print("Config (re-)loaded")

    def _on_recv(self, packet):
        topic = packet[0].decode('ascii')
        data = packet[1:]

        print("_on_recv topic=%s, data=%s" % (topic, repr(data)))

        if topic == "OK":
            self.valid_card_seen(data[0].decode('ascii'))
            return

        # Other results are failures
        print("Card %s not valid (reason: %s)" % (data[0], topic))

        self.log(data[0].decode('ascii'), topic)

    def valid_card_seen(self, card_uid):
        print("Card %s valid, sending to group %s" % (card_uid, self.config['knx']['group']))

        packet = [ 0x00, 0x80 ]
        devAddr = self.groupAddrToBytes(self.config['knx']['group'])
        self.__knx.EIBSendGroup(devAddr, packet)

        self.log(card_uid, "OK")

    def quit(self, *args):
        # This will close the sockets too
        if self.zmq_context:
            self.zmq_context.destroy()
        self.mainloop.stop()

    def run(self):
        print("Starting mainloop")
        self.mainloop.start()



if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: card_watcher.py config.yml")
        sys.exit(1)
    #TODO: init GPIO and do seteuid() to non-root account after that
    from tornado import ioloop
    loop = ioloop.IOLoop.instance()
    instance = card_watcher(sys.argv[1], loop)
    instance.hook_signals()
    try:
        instance.run()
    except KeyboardInterrupt:
        instance.quit()
