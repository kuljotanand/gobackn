#include "gbn.h"

uint16_t checksum(uint16_t *buf, int nwords)
{
	uint32_t sum;

	for (sum = 0; nwords > 0; nwords--)
		sum += *buf++;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return ~sum;
}

gbnhdr make_header(int type_command, int sequence_number){
	gbnhdr header;
	header.type = type_command;
	header.seqnum = sequence_number;
	header.checksum = 0; // TODO: fill this in later
	strcpy(header.data, ""); //TODO: fill this in later

	return header;
}

gbnhdr make_header_with_data(int type_command, int sequence_number, char *buffer){
	gbnhdr header;
	header.type = type_command;
	header.seqnum = sequence_number;
	header.checksum = 0; // TODO: fill this in later
	memcpy(header.data, buffer, 1024); //data capped at 1024 because that is the max packet size

	return header;
}


int check_header(char *buffer, int length){
	if (length != 4){
		return (-1);
	}
	else if (buffer[0] = SYN){
		return 0;
	}
}

int check_data_packet(char *buffer, int length){
	if (buffer[0] = DATA){
		return 0;
	}
	else {
		return(-1);
	}
}


ssize_t gbn_send(int sockfd, const void *buf, size_t len, int flags){
	/* TODO: Your code here. */
	/* Hint: Check the data length field 'len'.
	*       If it is > DATALEN, you will have to split the data
	*       up into multiple packets - you don't have to worry
	*       about getting more than N * DATALEN.
	*/
	if (len > DATALEN){
		//  make packets
	}
	else{
		gbnhdr create_header_with_data = make_header_with_data(DATA, 0, buf); // +4 is to get rid of the header that's part of the buf that we get in
		printf ("data sent: %s", create_header_with_data.data);
		int senddata = sendto(sockfd, &create_header_with_data, sizeof create_header_with_data, 0, receiver_global, receiver_socklen_global); //hardcoded 4 since that's always the length of the packet header
		// 4 from: DATA =1 byte, seqnum is 1 byte, checksum is 16 byte, and data is always empty for the SYN packet
		if (senddata == -1) return(-1);
	}
	return len;
}


ssize_t gbn_recv(int sockfd, void *buf, size_t len, int flags){
/* TODO: Your code here. */

int errno;
char data_buffer[1028];
int bytes_recd_in_data = recvfrom(sockfd, data_buffer, sizeof data_buffer, 0, receiver_global, &receiver_socklen_global); //TODO: change last 2 var names
printf ("Data: %d\n", data_buffer[0]);
	//here we will check if our SYN packet is correct
	// then we will move on to 'gbn_accept' to send a SYNACK back
	int check_packet_val = check_data_packet(data_buffer, bytes_recd_in_data);
	if (check_packet_val == -1){
		return (-1);
	}
	memcpy(buf, data_buffer + 4, bytes_recd_in_data - 4);
	return bytes_recd_in_data;
}

int gbn_close(int sockfd){

	/* TODO: Your code here. */

	return(-1);
}


int gbn_connect(int sockfd, const struct sockaddr *server, socklen_t socklen){

	/* TODO: Your code here. */

	receiver_global = server;
	receiver_socklen_global = socklen;

	if (sockfd < 0) {
		return(-1);
	}
	else {
		gbnhdr create_syn_header = make_header(SYN, 0);
		int sendsyn = sendto(sockfd, &create_syn_header, 4, 0, server, socklen); //hardcoded 4 since that's always the length of the packet header
		// 4 from: SYN =1 byte, seqnum is 1 byte, checksum is 16 byte, and data is always empty for the SYN packet
		if (sendsyn == -1) return(-1);
	}
return 0;
}


int gbn_listen(int sockfd, int backlog){
/* TODO: Your code here. */

int errno;
char buffer[1028];
int ack_packet = recvfrom(sockfd, buffer, sizeof buffer, 0, sender_global, &sender_socklen_global); //TODO: change last 2 var names
printf ("ACK packet size is %d \n", ack_packet);
	//here we will check if our SYN packet is correct
	// then we will move on to 'gbn_accept' to send a SYNACK back
	int check_val = check_header(buffer, ack_packet);
	if (check_val == -1){
		return (-1);
	}
	return 0;
}


int gbn_bind(int sockfd, const struct sockaddr *server, socklen_t socklen){
	/* TODO: Your code here. */

sender_global = server;
sender_socklen_global = socklen;

	if (sockfd < 0) {
		return(-1);
	}
	else {
		if (bind(sockfd, server, socklen) != 0){
			return(-1);
		}

	}
	return 0;
}


int gbn_socket(int domain, int type, int protocol){

	/*----- Randomizing the seed. This is used by the rand() function -----*/
	srand((unsigned)time(0));

	/* TODO: Your code here. */
	int data = socket(domain, type, protocol);
	return data;

	return(-1);
}


int gbn_accept(int sockfd, struct sockaddr *client, socklen_t *socklen){

	/* TODO: Your code here. */
	if (sockfd < 0) {
		return(-1);
	}
	else {
		gbnhdr create_synack_header = make_header(SYNACK, 0);
		int sendsynack = sendto(sockfd, &create_synack_header, 4, 0, sender_global, sender_socklen_global); //hardcoded 4 since that's always the length of the packet header
		// 4 from: SYN =1 byte, seqnum is 1 byte, checksum is 16 byte, and data is always empty for the SYN packet
		if (sendsynack == -1) return(-1);
	}
return sockfd;
}

ssize_t maybe_sendto(int  s, const void *buf, size_t len, int flags, \
                     const struct sockaddr *to, socklen_t tolen){

	char *buffer = malloc(len);
	memcpy(buffer, buf, len);


	/*----- Packet not lost -----*/
	if (rand() > LOSS_PROB*RAND_MAX){
		/*----- Packet corrupted -----*/
		if (rand() < CORR_PROB*RAND_MAX){

			/*----- Selecting a random byte inside the packet -----*/
			int index = (int)((len-1)*rand()/(RAND_MAX + 1.0));

			/*----- Inverting a bit -----*/
			char c = buffer[index];
			if (c & 0x01)
				c &= 0xFE;
			else
				c |= 0x01;
			buffer[index] = c;
		}

		/*----- Sending the packet -----*/
		int retval = sendto(s, buffer, len, flags, to, tolen);
		free(buffer);
		return retval;
	}
	/*----- Packet lost -----*/
	else
		return(len);  /* Simulate a success */
}
