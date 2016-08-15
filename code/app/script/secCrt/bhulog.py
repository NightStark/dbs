# $language = "Python"

# $interface = "1.0"
import time
import os
 
winscp_path = "E:\\tools\\wrok\\WinSCP\\"
download_path = "O:\\tftp\\"

errcode = 0

#open a net tab and connect
try:
  sTab = crt.Session.ConnectInTab("/s bhulog", True)
except ScriptError:
  errcode = crt.GetLastError()
if errcode != 0:
  crt.Dialog.MessageBox("Connection Failed")
#else:
  #crt.Dialog.MessageBox("Connection Successful")

#connect remote dev
#rtMAC = "84:82:f4:24:0c:ec"
rtMAC = crt.Dialog.Prompt("Enter MAC Address:", "MAC", "", False)
crt.Screen.Send("remote " + rtMAC + "\n")

#time.sleep(5)
#crt.Screen.Send("tar zcvf /tmp/tfcard/log.tar.gz /tmp/tfcard/log\n")
def run_cmd_log(cmd):
	crt.Screen.Send(cmd + "\n")
	if (crt.Screen.WaitForString("$", 300) == True):
  		crt.Dialog.MessageBox("successful")
		return 0
	else:
  		crt.Dialog.MessageBox("failed")
		return 1
def run_cmd_remote(cmd):
	crt.Screen.Send(cmd + " && echo __END=${?}__ " +"\n")
	if (crt.Screen.WaitForString("__END=0__", 300) == True):
		#crt.Dialog.MessageBox("get")
		return 0
	else:
		crt.Dialog.MessageBox("get failed")
		return 1

#package log file
time =  time.strftime("%Y_%m_%d_%H_%M_%S", time.localtime())
rtsMAC = rtMAC.replace(":", "", 5)
rtsMAC = rtsMAC.upper()
#crt.Dialog.MessageBox(rtsMAC)

log_file_name =  "upload_"+ rtsMAC + "_" + time +".tar.gz"
log_tmp_dir = "/tmp/tfcard/upload"

run_cmd_remote("mkdir -p " + log_tmp_dir)
collect_cmd = "cp -rfv /tmp/tfcard/log "+ log_tmp_dir +";" + \
	"cp -rfv /etc/config/user.conf "+ log_tmp_dir 
run_cmd_remote(collect_cmd)

run_cmd_remote("tar zcvf /tmp/tfcard/"+ log_file_name + " " + log_tmp_dir)
run_cmd_remote("ftpput -u ftpdlog -p ftpBhu8273 log.bhunetworks.com "+ log_file_name +" /tmp/tfcard/" + log_file_name)
#clear tmp file
run_cmd_remote("rm -rvf /tmp/tfcard/"+ log_file_name)
run_cmd_remote("rm -rvf /tmp/tfcard/"+ log_tmp_dir)

#os.system("winscp")
#os.system("open ftpdlog@log.bhunetworks.com")

#winscp_get_cmd = "CD /d E:\\tools\\wrok\\WinSCP\\" + \
#	"winscp.exe /console /command " + \
#	"\"option batch continue\" \"option confirm off\" " + \
#	"\"open ftp://ftpdlog:ftpBhu8273@log.bhunetworks.com\" " + \
#	"\"option transfer binary\" " + \
#	"\"get /log_8482F4240CEC_2016_07_04_16_11_37.tar.gz O:\\tftp\\\" " + \
#	"\"exit\" " + \
#	"/log=O:\\tftp\\winscp.logi"
#crt.Dialog.MessageBox(winscp_get_cmd)

#dowload to local
ssh_dir = os.getcwd()
winscp_get_cmd = ssh_dir + "\winscp_get.bat ftpdlog ftpBhu8273 log.bhunetworks.com " + \
	log_file_name + " " + winscp_path + " " + download_path
#crt.Dialog.MessageBox(winscp_get_cmd)
os.system(winscp_get_cmd)



#sTab.Close();

