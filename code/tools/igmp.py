from scapy.all import *
import scapy.contrib.igmp
from scapy.contrib.igmpv3 import IGMPv3,IGMPv3mq,IGMP,IGMPv3gr
from scapy.contrib.igmpv3 import IGMPv3mr


# x = Ether(src="00:01:02:03:04:05")/IP(dst="192.168.0.1")/scapy.contrib.igmp(gaddr="www.google.fr", type=0x11)
# x = Ether(raw(x))
# p = IP(dst="224.0.0.1")/scapy.contrib.igmp.IGMP(type=0x11, version)
p = IP(dst="224.0.0.1")/IGMPv3()/IGMPv3mq()


send(p) 

#r = sr1(p)

#print(r.summary())

