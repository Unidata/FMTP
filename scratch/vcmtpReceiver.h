/*
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 * @licence: Published under GPLv3
 *
 * @filename: vcmtpReceiver.h
 *
 * @history:
 *      Created on : Oct 2, 2014
 *      Author     : Shawn <sc7cq@virginia.edu>
 */

#ifndef VCMTPRECEIVER_H_
#define VCMTPRECEIVER_H_

#include "vcmtp.h"
#include "MulticastComm.h"
#include "RawSocketComm.h"
#include "InetComm.h"
#include "VCMTPComm.h"
#include "TcpClient.h"
#include "CommUtil/StatusProxy.h"
#include <pthread.h>

// receiver side status (not statistic info)
struct MessageReceiveStatus {
    uint                 msg_id;
    string               msg_name;
    union {
        void*            mem_buffer;
        int              file_descriptor;
    };
    int                  retx_file_descriptor;
    bool                 is_multicast_done;
    long long            msg_length;
    uint                 current_offset;
    long long            multicast_packets;
    long long            multicast_bytes;
    long long            retx_packets;
    long long            retx_bytes;
    bool                 recv_failed;
    CpuCycleCounter      start_time_counter;
    double               send_time_adjust;
    double               multicast_time;
};

//class VCMTPReceiver : public VCMTPComm {
class VCMTPReceiver {
public:
    VCMTPReceiver(string& tcpAddr, const unsigned short tcpPort);
    ~VCMTPReceiver();

//    int     JoinGroup(string addr, u_short port);
//    int     ConnectSenderOnTCP();
    void    init();
    void    Start();
    void    StartReceivingThread();
    void    RunReceivingThread();
    static void*  StartReceivingThread(void* ptr);
//    void    stop();

protected:
    RawSocketComm*   ptr_raw_sock_comm;
    MulticastComm*   ptr_multicast_comm;

private:
    std::string      tcpAddr;    /* Address of TCP server for missed data     */
    unsigned short   tcpPort;    /* Port number of TCP server for missed data */
    TcpClient*       retrans_tcp_client;
    pthread_mutex_t  retrans_list_mutex;
    int              max_sock_fd;
    int              multicast_sock;
    int              retrans_tcp_sock;
    int              packet_loss_rate;
    uint             session_id;
    StatusProxy*     status_proxy;
    bool             time_diff_measured;
    double           time_diff;
    VcmtpHeader*     read_ahead_header;
    char*            read_ahead_data;
    pthread_t        recv_thread;
    pthread_t        retrans_thread;
    bool             keep_retrans_alive;
    int              vcmtp_seq_num;
    size_t           total_missing_bytes;
    size_t           received_retrans_bytes;
    bool             is_multicast_finished;
    bool             retrans_switch;        // a switch that allows/disallows on-the-fly retransmission
    char             read_ahead_buffer[VCMTP_PACKET_LEN];
    fd_set           read_sock_set;
    ofstream         retrans_info;

    // Receive status map for all active files
    map<uint, MessageReceiveStatus>     recv_status_map;
    // File descriptor map for the main RECEIVING thread. Format: <msg_id, file_descriptor>
    map<uint, int>                      recv_file_map;
    list<VcmtpRetransRequest>           retrans_list;


//    static void*  StartReceivingThread(void* ptr);
//    void          HandleMulticastPacket();
//    void          HandleUnicastPacket();
//    void          HandleBofMessage(VcmtpSenderMessage& sender_msg);
//    void          HandleEofMessage(uint msg_id);
//    void          PrepareForFileTransfer(VcmtpSenderMessage& sender_msg);
//    void          HandleSenderMessage(VcmtpSenderMessage& sender_msg);
//    void          AddRetxRequest(uint msg_id, uint current_offset, uint received_seq);

};

#endif /* VCMTPRECEIVER_H_ */
