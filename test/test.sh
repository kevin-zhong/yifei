#!/bin/bash
i=0
cnt=10000
while [ $i -lt $cnt ]; 
do
        rm -rf dir/*
        ##valgrind --tool=memcheck  --leak-check=full ./yf_bridge_testor --gtest_filter=BridgeTestor.EvtThread; 
        ./yf_bridge_testor --gtest_filter=BridgeTestor.EvtThread
        if [ $? -ne 0 ];then 
                exit 
        fi

        rm -rf dir/*
        ./yf_bridge_testor --gtest_filter=BridgeTestor.BlockedThread
        if [ $? -ne 0 ];then
                exit
        fi

        #rm -rf dir/*
        #./yf_bridge_testor --gtest_filter=BridgeTestor.EvtProc
        if [ $? -ne 0 ];then
                exit
        fi

        let i++ 
        echo "try cnt=$i"
done
