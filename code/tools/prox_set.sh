#!/bin/bash

ACT=$1

if [ x"${ACT}" = x"" ] ; then
    ACT=on
fi

echo "action : ${ACT}"

WGETRC=~/.wgetrc
WGETRC_TMP=wgetrc.tmp

cat ${WGETRC} | sed -e '/proxy/d' | sed -e '/continue/d' | sed -e '/check_certificate/d' > ${WGETRC_TMP}

PROXY_IP=192.168.11.207
PROXY_PORT=1080

WGET_PROXY_ADDR=http://${PROXY_IP}:${PROXY_PORT}

if [ x"${ACT}" = x"on" ] ; then
    echo "https_proxy = ${WGET_PROXY_ADDR}" >> ${WGETRC_TMP}
    echo "http_proxy = ${WGET_PROXY_ADDR}" >> ${WGETRC_TMP}
    echo "ftp_proxy = ${WGET_PROXY_ADDR}" >> ${WGETRC_TMP}
    echo "use_proxy = on" >> ${WGETRC_TMP}
    echo "continue = on" >> ${WGETRC_TMP}
    echo "check_certificate = off" >> ${WGETRC_TMP}
else
    echo "proxy of wget will clean"
fi

cp -rvf ${WGETRC_TMP} ${WGETRC}
rm -rfv ${WGETRC_TMP}

CURLRC=~/.curlrc
CURLRC_TMP=curlrc.tmp

cat ${CURLRC} | sed -e '/proxy/d' > ${CURLRC_TMP}

CURL_PROXY_ADDR=${PROXY_IP}:${PROXY_PORT}

if [ x"${ACT}" = x"on" ] ; then
    echo "proxy = ${CURL_PROXY_ADDR}" >> ${CURLRC_TMP}
else
    echo "proxy of curl will clean"
fi

cp -rvf ${CURLRC_TMP} ${CURLRC}
rm -rfv ${CURLRC_TMP}


GIT_PROXY_ADDR=http://${PROXY_IP}:${PROXY_PORT}

if [ x"${ACT}" = x"on" ] ; then
    git config --global http.proxy "${GIT_PROXY_ADDR}"
    git config --global https.proxy "${GIT_PROXY_ADDR}"
else
    echo "proxy of gitconfig will clean"
    git config --global --unset  https.proxy 
    git config --global --unset  http.proxy 

fi

