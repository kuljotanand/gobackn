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
	// strcpy(header.data, ""); //TODO: fill this in later

	return header;
}

gbnhdr make_header_with_data(int type_command, int sequence_number, char *buffer, int data_length){
	gbnhdr header;
	header.type = type_command;
	header.seqnum = sequence_number;
	header.checksum = 0; // TODO: fill this in later
	memcpy(header.data, buffer, DATALEN); //data capped at 1024 because that is the max packet size
	header.lenData = data_length;

	return header;
}


int check_header(char *buffer, int length){
	if (length != 4){
		return (-1);
	}
	else if (buffer[0] == SYN){
		return 0;
	}
}


// int check_data_packet(char *buffer, int length){
// 	if (buffer[0] == DATA){
// 		return 0;
// 	}
// 	else {
// 		return(-1);
// 	}
// }


// Sends a packet with the appropriate 4 byte header
int send_packet(int sockfd,  char buf[], int data_length) {
	gbnhdr create_header_with_data = make_header_with_data(DATA, 0, buf, data_length);
	// printf("VENMO");
	printf ("data sent: %s\n", create_header_with_data.data);
	printf("DATA LENGTH %d\n", create_header_with_data.lenData);
	// printf(buf);
	int senddata = sendto(sockfd, &create_header_with_data, sizeof create_header_with_data, 0, receiver_global, receiver_socklen_global); //hardcoded 4 since that's always the length of the packet header
	// 4 from: DATA =1 byte, seqnum is 1 byte, checksum is 16 byte, and data is always empty for the SYN packet
	if (senddata == -1) return(-1);
}


// checks if the packet received is DATA packet
int check_if_data_packet(char *buffer){
	if (buffer[0] == DATA){
		return 0;
	}
	else{
		return (-1);
	}
}

// checks if the data received is FIN packet
int check_if_fin_packet(char *buffer){
	if (buffer[0] == FIN){
		return 0;
	}
	else{
		return (-1);
	}
}
//
//

ssize_t gbn_send(int sockfd, const void *buf, size_t len, int flags){
	/* TODO: Your code here. */
	/* Hint: Check the data length field 'len'.
	*       If it is > DATALEN, you will have to split the data
	*       up into multiple packets - you don't have to worry
	*       about getting more than N * DATALEN.
	*/
	//  make packets
	int orig_len = len;
	int cur_size = 0;
	int track = 0;
	char new_buf[DATALEN];

	while (track != orig_len) {

		// Clear the packet buffer
		memset(new_buf, '\0', sizeof(new_buf));

		// Send the first packet
		if (track == 0) {
			if (len >= DATALEN) {
				cur_size = DATALEN;
			}
			else {
				cur_size = len;
				printf ("CUR SIZE: %d", cur_size);
			}
			memcpy(new_buf, buf + track, cur_size);
			send_packet(sockfd, new_buf, cur_size);
			track += cur_size;
			len = len - cur_size;

			// Send the rest of the packets
		} else {
			if (len >= DATALEN) {
				cur_size = DATALEN;
			}
			else {
				cur_size = len;
			}
			printf ("CUR SIZE: %d", cur_size);
			memcpy(new_buf, buf + track, cur_size);
			send_packet(sockfd, new_buf, cur_size);
			track += cur_size;
			len = len - cur_size;
		}
		printf("%s\n","CLOSE");
		// printf("track: %d %s", track, new_buf);
	}
	return len;
}


ssize_t gbn_recv(int sockfd, void *buf, size_t len, int flags){
	/* TODO: Your code here. */
	//char data_buffer[1028];
	gbnhdr * data_buffer = malloc(sizeof(*data_buffer));
	int bytes_recd_in_data = recvfrom(sockfd, data_buffer, sizeof *data_buffer, 0, receiver_global, &receiver_socklen_global);
	int packet_type_recd;
	packet_type_recd = check_if_data_packet(data_buffer);
	if (packet_type_recd != 0){
		gbnhdr create_finack_header = make_header(FINACK, 0);
		int sendfinack = sendto(sockfd, &create_finack_header, 4, 0, sender_global, sender_socklen_global);
		if (sendfinack == -1){
			return(-1);
		}
		else{
			printf ("%s\n", "SENT THE FINACK");
			return 0;
		}
	}
	else{
		printf("BYTES RECEIVED %d\n", bytes_recd_in_data );
		printf("LENDATA %d\n", data_buffer->lenData);
		printf ("%s\n", "DATA packet was received");

		// printf ("Data: %s\n", &data_buffer->data);
		//here we will check if our SYN packet is correct
		// then we will move on to 'gbn_accept' to send a SYNACK back
		// int check_packet_val = check_data_packet(data_buffer, bytes_recd_in_data);
		// if (check_packet_val == -1){
		// 	return (-1);
		// }
		memcpy(buf, data_buffer->data, data_buffer->lenData);
		// printf("BUF:%s\n", buf);
		// return bytes_recd_in_data;
		return data_buffer->lenData;
	}
}

// if fin, isFin = 1, else isFin = 0 for Finack
int gbn_close(int sockfd, int isFin){
	/* TODO: Your code here. */

	if (sockfd < 0) {
		return(-1);
	}

	else {
		if (isFin == 1) {
			gbnhdr create_fin_header = make_header(FIN, 0);
			int sendfin = sendto(sockfd, &create_fin_header, 4, 0, receiver_global, receiver_socklen_global); //hardcoded 4 since that's always the length of the packet header
			// 4 from: SYN =1 byte, seqnum is 1 byte, checksum is 16 byte, and data is always empty for the SYN packet
			if (sendfin == -1)
				return-1;
			printf ("%s\n", "SENT THE FIN");
		}
		// FINACK
		else {
			gbnhdr create_finack_header = make_header(FINACK, 0);
			int sendfinack = sendto(sockfd, &create_finack_header, 4, 0, sender_global, sender_socklen_global);
			if (sendfinack = -1)
				return -1;
			printf ("%s\n", "SENT THE FINACK");
		}
	}

	return 0;
}

// int gbn_close(int sockfd){
//
// 	/* TODO: Your code here. */
//
//
// 	return(-1);
// }


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
