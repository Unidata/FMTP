/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpReceiver.cpp
 *
 * @history:
 *      Created on : Oct 2, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "vcmtpReceiver.h"

using namespace std;

VCMTPReceiver::VCMTPReceiver(
    string&                 tcpAddr,
    const unsigned short    tcpPort)
:
    tcpAddr(tcpAddr),
    tcpPort(tcpPort)
{
    init();
}

VCMTPReceiver::~VCMTPReceiver() {
    delete retrans_tcp_client;
    pthread_mutex_destroy(&retrans_list_mutex);
}

void VCMTPReceiver::init()
{
    retrans_tcp_client = 0;
    max_sock_fd = 0;
    //multicast_sock = ptr_multicast_comm->GetSocket();
    retrans_tcp_sock = 0;
    packet_loss_rate = 0;
    session_id = 0;
    //status_proxy = 0;
    time_diff_measured = false;
    time_diff = 0;
    read_ahead_header = (VcmtpHeader*)read_ahead_buffer;
    read_ahead_data = read_ahead_buffer + VCMTP_HLEN;
    recv_thread = 0;
    retrans_thread = 0;
    keep_retrans_alive = false;
    vcmtp_seq_num = 0;
    total_missing_bytes = 0;
    received_retrans_bytes = 0;
    is_multicast_finished = false;
    retrans_switch = true;
    read_ahead_header->session_id = -1;
}

void VCMTPReceiver::Start() {
    StartReceivingThread();
}

void VCMTPReceiver::StartReceivingThread() {
    pthread_create(&recv_thread, NULL, &VCMTPReceiver::StartReceivingThread, this);
}

void* VCMTPReceiver::StartReceivingThread(void* ptr) {
    ((VCMTPReceiver*)ptr)->RunReceivingThread();
    return NULL;
}

void VCMTPReceiver::RunReceivingThread() {
    std::cout << "RunReceivingThread() running" << std::endl;
    fd_set  read_set;
    while(true) {
        read_set = read_sock_set;
        if (select(max_sock_fd + 1, &read_set, NULL, NULL, NULL) == -1) {
            //SysError("TcpServer::SelectReceive::select() error");
            break;
        }

        // tests to see if multicast_sock is part of the set
        if (FD_ISSET(multicast_sock, &read_set)) {
            HandleMulticastPacket();
        }

        // tests to see if retrans_tcp_sock is part of the set
        if (FD_ISSET(retrans_tcp_sock, &read_set)) {
            //HandleUnicastPacket();
        }
    }
    pthread_exit(0);
}

int VCMTPReceiver::udpBindIPSock(string senderAddr, ushort port)
{
    struct sockaddr_in sender;
    bzero(&sender, sizeof(sender));
    sender.sin_family = AF_INET;
    sender.sin_addr.s_addr = inet_addr(senderAddr.c_str());
    sender.sin_port = htons(port);
    if( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        std::cout << "udpBindIPSock() creating socket failed." << std::endl;
    socklen_t sendAddrSize = sizeof(sender);
}

void VCMTPReceiver::HandleMulticastPacket() {
    static char packet_buffer[VCMTP_PACKET_LEN];
    static VcmtpHeader* header = (VcmtpHeader*) packet_buffer;
    static char* packet_data = packet_buffer + VCMTP_HLEN;

    static VcmtpSenderMessage *ptr_sender_msg = (VcmtpSenderMessage *) packet_data;
    static map<uint, MessageReceiveStatus>::iterator it;

    if (ptr_multicast_comm->RecvData(packet_buffer, VCMTP_PACKET_LEN, 0, NULL, NULL) < 0)
        SysError("VCMTPReceiver::RunReceivingThread() multicast recv error");

    // Check whether the header flag is BOF or Data or EOF. And call
    // corresponding handler.
    if (header->flags & VCMTP_BOF) {
        HandleBofMessage(*ptr_sender_msg);
    } else if (header->flags & VCMTP_EOF) {
        HandleEofMessage(header->session_id);
    } else if (header->flags == VCMTP_DATA) {
        if ((it = recv_status_map.find(header->session_id)) == recv_status_map.end()) {
        //cout << "[VCMTP_DATA] Could not find the message ID in recv_status_map: " << header->session_id << endl;
        return;
        }

        MessageReceiveStatus& recv_status = it->second;
        if (recv_status.recv_failed)
            return;

        // Write the packet into the file. Otherwise, just drop the packet (emulates errored packet)
        if (rand() % 1000 >= packet_loss_rate) {
            if (header->seq_number > recv_status.current_offset) {
                AddRetxRequest(header->session_id, recv_status.current_offset, header->seq_number);
                if (lseek(recv_status.file_descriptor, header->seq_number, SEEK_SET) < 0) {
                    cout << "Error in file " << header->session_id << ":  " << endl;
                    SysError("VCMTPReceiver::RunReceivingThread()::lseek() error on multicast data");
                }
            }

        if (recv_status.file_descriptor > 0 && \
            write(recv_status.file_descriptor, packet_data, header->data_len) < 0)
            SysError("VCMTPReceiver::RunReceivingThread()::write() error on multicast data");

        recv_status.current_offset = header->seq_number + header->data_len;
        }
    }
}

/*
// Handle the receive of a single TCP packet
void VCMTPReceiver::HandleUnicastPacket() {
    static char packet_buffer[VCMTP_PACKET_LEN];
    static VcmtpHeader* header = (VcmtpHeader*) packet_buffer;
    static char* packet_data = packet_buffer + VCMTP_HLEN;

    static VcmtpSenderMessage sender_msg;
    static map<uint, MessageReceiveStatus>::iterator it;

    if (retrans_tcp_client->Receive(header, VCMTP_HLEN) < 0) {
        SysError("VCMTPReceiver::RunReceivingThread()::recv() error");
    }

    if (header->flags & VCMTP_SENDER_MSG_EXP) {
        if (retrans_tcp_client->Receive(&sender_msg, header->data_len) < 0) {
            ReconnectSender();
            return;
        }
        HandleSenderMessage(sender_msg);
    } else if (header->flags & VCMTP_RETRANS_DATA) {
        if (retrans_tcp_client->Receive(packet_data, header->data_len) < 0)
            SysError("VCMTPReceiver::RunningReceivingThread()::receive error on TCP");

        if ((it = recv_status_map.find(header->session_id)) == recv_status_map.end()) {
            return;
        }

        MessageReceiveStatus& recv_status = it->second;
        if (recv_status.retx_file_descriptor == -1) {
            recv_status.retx_file_descriptor = dup(recv_status.file_descriptor);
            if (recv_status.retx_file_descriptor < 0)
                SysError("VCMTPReceiver::RunReceivingThread() open file error");
        }

        if (lseek(recv_status.retx_file_descriptor, header->seq_number, SEEK_SET) == -1) {
            SysError("VCMTPReceiver::RunReceivingThread()::lseek() error on retx data");
        }

        if (write(recv_status.retx_file_descriptor, packet_data, header->data_len) < 0) {
            //SysError("VCMTPReceiver::ReceiveFile()::write() error");
            cout << "VCMTPReceiver::RunReceivingThread()::write() error on retx data" << endl;
        }

        // Update statistics
        recv_status.retx_packets++;
        recv_status.retx_bytes += header->data_len;
        recv_stats.total_recv_packets++;
        recv_stats.total_recv_bytes += header->data_len;
        recv_stats.total_retrans_packets++;
        recv_stats.total_retrans_bytes += header->data_len;

    } else if (header->flags & VCMTP_RETRANS_END) {
        it = recv_status_map.find(header->session_id);
        if (it == recv_status_map.end()) {
            cout << "[VCMTP_RETRANS_END] Could not find the message ID in recv_status_map: " << header->session_id << endl;
            return;
        } else {
            MessageReceiveStatus& recv_status = it->second;
            close(recv_status.file_descriptor);
            recv_status.file_descriptor = -1;
            if (recv_status.retx_file_descriptor > 0) {
                close(recv_status.retx_file_descriptor);
                recv_status.retx_file_descriptor = -1;
            }

            recv_stats.last_file_recv_time = GetElapsedSeconds(recv_stats.reset_cpu_timer);
            AddSessionStatistics(header->session_id);
        }
    } else if (header->flags & VCMTP_RETRANS_TIMEOUT) {
        //cout << "I have received a timeout message for file " << header->session_id << endl;
        it = recv_status_map.find(header->session_id);
        if (it != recv_status_map.end()) {
            MessageReceiveStatus& recv_status = it->second;
            if (!recv_status.recv_failed) {
                recv_status.recv_failed = true;

                close(recv_status.file_descriptor);
                recv_status.file_descriptor = -1;
                if (recv_status.retx_file_descriptor > 0) {
                    close(recv_status.retx_file_descriptor);
                    recv_status.retx_file_descriptor = -1;
                }

                recv_stats.num_failed_files++;
                //AddSessionStatistics(header->session_id);
            }
        }
        else {
            cout << "[VCMTP_RETRANS_TIMEOUT] Could not find message in recv_status_map for file " << header->session_id << endl;
        }
    }
}
*/
