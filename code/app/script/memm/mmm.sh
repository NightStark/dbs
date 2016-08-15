#!/bin/sh

#download URL:wget http://code.taobao.org/svn/websever/trunk/code/app/script/memm/mmm.sh

if [ $# -lt 2 ] ; then
    echo use:
    echo "   mmm.sh \"3600 \\* 2\" \"3 \\* 60\""
    echo ""
    exit 1
fi

#monitor process list
mp_list="bmc dms"
#mp_list=$(ps | awk '{if ($2 == 1){print $7}}')
#miItList="MemTotal MemFree Buffers Cached Active Inactive"
miItList=SUnreclaim
#slabList="scsi_sense_cache scsi_cmd_cache bridge_fdb_cache"
#slabList=$(cat /proc/slabinfo | awk 'NR>2{print $1}')
slabList=skbuff_head_cache


MMM_FILE=/tmp/.mmmp
MMMI_FILE=/tmp/.mmmip
SLAB_FILE=/tmp/.mmmslab
rm -rfv $MMM_FILE
rm -rfv $MMMI_FILE
rm -rfv $SLAB_FILE

X_UPTIME=uptime
XMI_UPTIME=$X_UPTIME
SLAB_UPTIME=$X_UPTIME
#监控多久, in sec
timeGoStr=$1
timeGo=$(eval "expr ${timeGoStr}")
#隔多久获取一次数据。
timePStr=$2
timeP=$(eval "expr ${timePStr}")
echo memory monitor will run ${timeGo}s / ${timeP}s
echo monitor on : $mp_list

#miItList=$(cat /proc/meminfo  | awk -F ":" '{print $1 $2}' | awk '{printf("%s ", $1)}')

#add uptime to list
mp_list="${X_UPTIME} ${mp_list}" 
miItList="${XMI_UPTIME} ${miItList}"
slabList="${SLAB_UPTIME} ${slabList}"
#pid rss name
get_RSS_list() {
    #dev uptime in seconds (eg:235.34s)
    #uptime=$(cat /proc/uptime | awk '{print $1}')
    #uptime=$(cat /proc/uptime | awk '{print $1}' | awk -F "." '{print $1}')
	uptime=$(date +%m.%d-%H:%M:%S)

    PS_MMM_FILE=/tmp/.mmm
    rm -rvf $PS_MMM_FILE
    #ps_list=$(ps | awk '{printf("%s %s %s\n",$1, $5,$7)}')
    ps | awk '{printf("%s %s %s\n",$1, $5,$7)}' > $PS_MMM_FILE
    #awk '{printf("%s %s %s\n",$1, $5,$7);}' ps.f > $PS_MMM_FILE

    rss_list="${uptime}"
    for p in $mp_list;
    do
        #为了先后顺不乱，只好这样，sh又不支持数组!!
        rss_list="$rss_list $(cat $PS_MMM_FILE | awk '{if ($3 == "'$p'"){printf("%s",$2);exit}}')"
    done
    echo $rss_list >> $MMM_FILE
    
    return
}

get_MemInfo()
{
    #dev uptime in seconds (eg:235.34s)
    #uptime=$(cat /proc/uptime | awk '{print $1}')
    #uptime=$(cat /proc/uptime | awk '{print $1}' | awk -F "." '{print $1}')
	uptime=$(date +%m.%d-%H:%M:%S)

    #ps_list=$(ps | awk '{printf("%s %s %s\n",$1, $5,$7)}')
    #cat /proc/meminfo  | awk -F ":" '{print $1 $2}' | awk '{printf("%s ", $1)}'
    CAT_MI_FILE=/tmp/.cmi
    rm -rfv $CAT_MI_FILE
    cat /proc/meminfo  | awk -F ":" '{print $1 $2}' | awk   '{printf("%s %s\n",$1, $2)}' > $CAT_MI_FILE
    mi_lines="${uptime}"
    for p in $miItList;
    do
        #为了先后顺不乱，只好这样，sh又不支持数组!!
        mi_lines="$mi_lines $(cat $CAT_MI_FILE | awk '{if ($1 == "'$p'"){printf("%s",$2);exit}}')"
    done
    #mi_lines=$(cat /proc/meminfo  | awk 'BEGIN{printf("%s ", "'$uptime'");}{printf("%s ",$2)}END{printf("\n")}')
    echo $mi_lines >> $MMMI_FILE
    
    return
}

get_Slab_info()
{
    #uptime=$(cat /proc/uptime | awk '{print $1}' | awk -F "." '{print $1}')
	#date +%m.%d-%H:%M:%S
	uptime=$(date +%m.%d-%H:%M:%S)
    CAT_SLAB_FILE=/tmp/.cslab
    rm -rfv $CAT_SLAB_FILE
    cat /proc/slabinfo | awk '{printf("%s %d\n"),$1 ,$3*$4}' > $CAT_SLAB_FILE
    slabLines="${uptime}"
    for p in $slabList;
    do
        slabLines="${slabLines} $(cat $CAT_SLAB_FILE | awk '{if ($1 == "'$p'") {printf("%s", $2);exit};}')"
    done

    echo $slabLines >> $SLAB_FILE
}


#sh not support arrary
#array_name=("1" "2" "3")
#pname[0]="123"
#echo name = ${pname[0]}

#awk '{printf("%s %s %s\n",$1, $5,$7);}' ps.f > $PS_MMM_FILE

start_monit(){
    #monitor time in seconds
    mmm_time=$1
    p_time=$2
    echo  ${mp_list} > $MMM_FILE
    echo ${miItList} > $MMMI_FILE
    echo ${slabList} > $SLAB_FILE

    while true
    do
        get_RSS_list
        get_MemInfo
        get_Slab_info

        sleep $p_time
        mmm_time=$(expr $mmm_time - $p_time)
        echo mmm_time = $mmm_time
        if [ $mmm_time -lt 0 ] ; then
            break
        fi
    done
}

MMDTAT_JS=mmdata.js
rm -rfv $MMDTAT_JS
touch $MMDTAT_JS

make_a_data_js(){
M_VIEW=$1 #视图名称
M_LIST=$2 #监控项目
M_FILE=$3 #监控的数据文件
echo =========make $MMDTAT_JS with $M_VIEW -- $M_LIST -- $M_FILE
#***************mmdata.js************************
#var data_legend_xx = ['bmc','dms',] #xx:为视图名称
#var data_time = ['1','2','3','4','5','6','7',]; //X轴的时间
#var data_xx = [11, 11, 15, 13, 50, 13, 10,]; //xx为进程的名字
#var data_yy = [1, -2, 2, 5, 3, 2, 0,];       //yy和xx一样，也是个例子。
#var data_s_xx = [{name:'bmc',type:'line',data:data_xx,},{name:'dms',type:'line',data:data_yy,},]; #xx为视图名称
#**************************************
data_legend_line=$(cat $M_FILE | awk '{if (NR == 1){printf("var data_legend_%s = [", "'${M_VIEW}'");
for(pn=1;pn<=NF;pn++){
    if ($pn == "'$X_UPTIME'"){continue}
    printf("'\''""%s""'\''"",", $pn);
}
printf("];\n");}}')
#echo $data_legend_line 
echo $data_legend_line >> $MMDTAT_JS

#构建每个进程的js数组
pindex=1
for p in $M_LIST;
do
    #echo $p
    #为了先后顺不乱，只好这样，sh又不支持数组!!
    adata=$(cat $M_FILE | awk '
    BEGIN{istime = 0}
    {
        if (NR == 1){
            if( $'$pindex' != "'$p'"){
                printf("bad index\n"); exit;
            } else {
                if ($'$pindex' == "'$X_UPTIME'") {
                    istime = 1;
                    printf("var data_%s_%s = [", $'$pindex', "'$M_VIEW'");
                } else {
                    printf("var data_%s = [", $'$pindex');
                }
            }
        } else {
            if (istime == 1) {
                printf("'\''""%s""'\'', ", $'$pindex');
            } else {
                printf("%s, ", $'$pindex');
            }
        };
    }
    END{printf("];")}')
    pindex=$(expr $pindex + 1)
    #echo $adata
    echo $adata >> $MMDTAT_JS
done
#构建每个进程数据描述js数组
#var data_s = [{name:'bmc',type:'line',data:data_xx,},{name:'dms',type:'line',data:data_yy,},];
data_s_line="var data_s_${M_VIEW} = ["
for p in $M_LIST;
do
    #uptime is not a progress
    if [ x"$X_UPTIME" = x"$p" ]; then
        continue;
    fi
    data_s_line="${data_s_line}{name:"\'"$p"\'",type:"\'"line"\'",data:data_$p,},"
done
data_s_line="${data_s_line}];"
#echo $data_s_line
echo $data_s_line >> $MMDTAT_JS

#char transform
TMP_MMDTAT_JS=/tmp/$MMDTAT_JS
sed 's/\./_/g' $MMDTAT_JS > $TMP_MMDTAT_JS
mv $TMP_MMDTAT_JS $MMDTAT_JS
sed 's/-/_/g' $MMDTAT_JS > $TMP_MMDTAT_JS
mv $TMP_MMDTAT_JS $MMDTAT_JS

return
}

make_data(){                                                                                 
        make_a_data_js "mmm" "${mp_list}" "${MMM_FILE}" #字符串的参数要加个引号啊    
        make_a_data_js "mmmi" "${miItList}" "${MMMI_FILE}" #字符串的参数要加个引号啊 
        make_a_data_js "slabi" "${slabList}" "${SLAB_FILE}" #字符串的参数要加个引号啊
		
		rm -rvf $MMM_FILE   
		rm -rvf $MMMI_FILE 
		rm -rvf $SLAB_FILE 
		rm -rvf $PS_MMM_FILE
		
        exit 0            
}                         
                          
trap make_data 15         
start_monit $timeGo $timeP
make_data           
                    




