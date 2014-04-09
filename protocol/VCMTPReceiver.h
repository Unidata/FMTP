/*
 * VCMTPReceiver.h
 *
 *  Created on: Jul 21, 2011
 *      Author: jie
 */

#ifndef VCMTPRECEIVER_H_
#define VCMTPRECEIVER_H_

#include "ReceivingApplicationNotifier.h"
#include "vcmtp.h"
#include "VCMTPComm.h"
#include "TcpClient.h"
#include "VcmtpSenderMetadata.h"
#include "../CommUtil/PerformanceCounter.h"
#include "../CommUtil/StatusProxy.h"

struct VcmtpReceiverStats {
	uint	current_msg_id;
	uint 	total_recv_packets;
	uint	total_recv_bytes;
	uint 	total_retrans_packets;
	uint	total_retrans_bytes;
	uint 	session_recv_packets;
	uint	session_recv_bytes;
	uint 	session_retrans_packets;
	uint	session_retrans_bytes;
	double	session_retrans_percentage;
	double	session_total_time;
	double	session_trans_time;
	double 	session_retrans_time;

	// new stat fields
	PerformanceCounter	cpu_monitor;
	CpuCycleCounter		reset_cpu_timer;
	int					num_recved_files;
	int					num_failed_files;
	double				last_file_recv_time;
	vector<string>		session_stats_vec;
};


struct MessageReceiveStatus {
	uint 		msg_id;
	string		msg_name;
	union {
			void* 	mem_buffer;
			int		file_descriptor;
		};
	int			retx_file_descriptor;
	bool		is_multicast_done;
	long long 	msg_length;
	uint		current_offset;
	long long	multicast_packets;
	long long	multicast_bytes;
	long long 	retx_packets;
	long long 	retx_bytes;
	bool		recv_failed;
	CpuCycleCounter 	start_time_counter;
	double		send_time_adjust;
	double		multicast_time;
};


struct VcmtpReceiverConfig {
	string 	multicast_addr;
	string  sender_ip_addr;
	int		sender_tcp_port;
	int		receive_mode;
};


// VCMTPReceiver is the main class that communicates with the VCMTP Sender
class VCMTPReceiver : public VCMTPComm {
public:
	VCMTPReceiver(int buf_size);
	VCMTPReceiver(const ReceivingApplicationNotifier& notifier);
	~VCMTPReceiver();

	int 	JoinGroup(string addr, u_short port);
	int		ConnectSenderOnTCP();
	void 	Start();
	void 	StartReceivingThread();
	void	SetSchedRR(bool is_rr);

	void 	SetPacketLossRate(int rate);
	int 	GetPacketLossRate();
	void	SetBufferSize(size_t size);
	void 	SendHistoryStats();
	void	ResetHistoryStats();
	void	SendHistoryStatsToSender();
	void 	SendSessionStatistics();
	void	ResetSessionStatistics();
	void 	AddSessionStatistics(uint msg_id);
	void	SendSessionStatisticsToSender();
	void	ExecuteCommand(char* command);
	void 	SetStatusProxy(StatusProxy* proxy);
	const   struct VcmtpReceiverStats GetBufferStats();
	bool    notify_of_bof(VcmtpSenderMessage& msg);
	void    notify_of_eof(VcmtpSenderMessage& msg);
	void    notify_of_missed_file(VcmtpSenderMessage& msg);
	void	RunReceivingThread();


private:
	TcpClient*	        retrans_tcp_client;
	// used in the select() system call
	int		        max_sock_fd;
	int 		        multicast_sock;
	int		        retrans_tcp_sock;
	fd_set		        read_sock_set;
	ofstream 	        retrans_info;

	int 		        packet_loss_rate;
	uint			session_id;
	VcmtpReceiverStats 	recv_stats;
	CpuCycleCounter		cpu_counter, global_timer;
	StatusProxy*		status_proxy;

	PerformanceCounter 	cpu_info;
	bool			time_diff_measured;
	double 			time_diff;

	/**
	 * Initializes this instance.
	 */
	void    init();

	void 	ReconnectSender();

	// Memory-to-memory data tranfer
	void 	ReceiveMemoryData(const VcmtpSenderMessage & msg, char* mem_data);
	void 	DoMemoryDataRetransmission(char* mem_data, const list<VcmtpNackMessage>& nack_list);
	// Disk-to-disk data transfer
	void 	ReceiveFileBufferedIO(const VcmtpSenderMessage & transfer_msg);
	void 	ReceiveFileMemoryMappedIO(const VcmtpSenderMessage & transfer_msg);
	void 	DoFileRetransmission(int fd, const list<VcmtpNackMessage>& nack_list);

	void 	DoAsynchronousWrite(int fd, size_t offset, char* data_buffer, size_t length);
	static void HandleAsyncWriteCompletion(sigval_t sigval);
	void	CheckReceivedFile(const char* file_name, size_t length);
	void	SendNackMessages(const list<VcmtpNackMessage>& nack_list);
	void 	HandleMissingPackets(list<VcmtpNackMessage>& nack_list, uint current_offset, uint received_seq);

	// Functions related to TCP data transfer
	void 	TcpReceiveMemoryData(const VcmtpSenderMessage & msg, char* mem_data);
	void 	TcpReceiveFile(const VcmtpSenderMessage & transfer_msg);


	// Receive status map for all active files
	map<uint, MessageReceiveStatus> recv_status_map;

	// File descriptor map for the main RECEIVING thread. Format: <msg_id, file_descriptor>
	map<uint, int> 	recv_file_map;

	char read_ahead_buffer[VCMTP_PACKET_LEN];
	VcmtpHeader* read_ahead_header;
	char* read_ahead_data;

	/**
	 * Notifies the receiving application about file events.
	 */
	const ReceivingApplicationNotifier      notifier;

	/**
	 * This class implements the default method for notifying the receiving
	 * application about file events by using the notification queue within
	 * the VCMTPReceiver.
	 */
	class BatchedNotifier : public ReceivingApplicationNotifier {
	public:
	    BatchedNotifier(VCMTPReceiver& receiver) : receiver(receiver) {};
            bool notify_of_bof(VcmtpSenderMessage& msg);
            void notify_of_eof(VcmtpSenderMessage& msg);
            void notify_of_missed_file(VcmtpSenderMessage& msg);
	private:
            VCMTPReceiver&      receiver;
	};


	//*********************** Main receiving thread functions ***********************
	pthread_t	recv_thread;
	static void* StartReceivingThread(void* ptr);
	void	HandleMulticastPacket();
	void	HandleUnicastPacket();
	void	HandleBofMessage(VcmtpSenderMessage& sender_msg);
	void  	HandleEofMessage(uint msg_id);
	void	PrepareForFileTransfer(VcmtpSenderMessage& sender_msg);
	void	HandleSenderMessage(VcmtpSenderMessage& sender_msg);
	void	AddRetxRequest(uint msg_id, uint current_offset, uint received_seq);

	//*********************** Retransmission thread functions ***********************
	void 	StartRetransmissionThread();
	static void* StartRetransmissionThread(void* ptr);
	void	RunRetransmissionThread();
	pthread_t			retrans_thread;
	pthread_mutex_t 	retrans_list_mutex;
	bool				keep_retrans_alive;
	list<VcmtpRetransRequest> 	retrans_list;

	int		vcmtp_seq_num;
	size_t	total_missing_bytes;
	size_t	received_retrans_bytes;
	bool	is_multicast_finished;
	bool	retrans_switch;		// a switch that allows/disallows on-the-fly retransmission
};

#endif /* VCMTPRECEIVER_H_ */
