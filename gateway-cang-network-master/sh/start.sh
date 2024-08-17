#!/bin/bash

mkdir /mnt/config -p
mkdir /tmp/ipc/shmem -p
mkdir /tmp/ipc/msgqueue/peer -p


# 创建 /tmp/ipc/shmem 目录，如果不存在
if [ ! -d "/tmp/ipc/shmem" ]; then
    mkdir -p "/tmp/ipc/shmem"
else
    echo "/tmp/ipc/shmem 目录已存在"
fi

# 创建 /tmp/ipc/msgqueue/peer 目录，如果不存在
if [ ! -d "/tmp/ipc/msgqueue/peer" ]; then
    mkdir -p "/tmp/ipc/msgqueue/peer"
else
    echo "/tmp/ipc/msgqueue/peer 目录已存在"
fi
#设备响应
gnome-terminal -- sh -c 'bash -c "cd /home/hq/24022/Bigger/gateway-cang-network/search_c;  sudo ./dev; exec bash"'
sleep 1

# 启动mqtt进程
gnome-terminal -- sh -c 'bash -c "sudo -u hq mosquitto; exec bash"'
sleep 1

# 启动视频流
xterm -T "Process stream" -e "cd /home/hq/24022/Bigger/gateway-cang-network/Mjpj; sudo ./mjpg_streamer -i \"./input_uvc.so -r 320x240\" -o \"./output_udp.so -p 8887\" -o \"./output_http.so -w ./www\""&


#上报进程
gnome-terminal -- sh -c 'bash -c "cd /home/hq/24022/Bigger/gateway-cang-network/ShangBao;  sudo ./a.out; exec bash"'
sleep 1

#32进程
gnome-terminal -- sh -c 'bash -c "cd /home/hq/24022/Bigger/gateway-cang-network/stm32; sudo ./test; exec bash"'
sleep 1

#modbus进程
gnome-terminal -- sh -c 'bash -c "cd /home/hq/24022/Bigger/gateway-cang-network/project_modbus;  sudo ./mb_program; exec bash"'
sleep 1


# #阿里云进程
# gnome-terminal -- sh -c 'bash -c "cd /home/hq/24022/Bigger/gateway-cang-network/aliyun/xiangmu/LinkSDK;  sudo ./output/data-model-basic-demo; exec bash"'
# sleep 1

#web进程
gnome-terminal -- sh -c 'bash -c "cd /home/hq/24022/Bigger/gateway-cang-network/Web;  sudo ./test; exec bash"'
sleep 1
