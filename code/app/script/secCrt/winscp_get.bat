
set user_name=%1
set user_pwd=%2
set host=%3
set file_name=%4
set winscp_path=%5
set down_load_path=%6
set bat_path=%cd%


echo user nname : %user_name%

CD /d %winscp_path%
winscp.exe /console /command "option batch continue" "option confirm off" "open ftp://%user_name%:%user_pwd%@%host%" "option transfer binary" "get /%file_name% %down_load_path%\" "exit" /log=%bat_path%\winscp.log
