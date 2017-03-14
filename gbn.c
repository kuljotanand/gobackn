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


int check_header(char *buffer, int length){
	if (length != 4){
		return (-1);
	}
	else if (buffer[0] = SYN){
		return 0;
	}
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
		//  send data
	}

	return(-1);
}


ssize_t gbn_recv(int sockfd, void *buf, size_t len, int flags){

	/* TODO: Your code here. */

	return(-1);
}

int gbn_close(int sockfd){

	/* TODO: Your code here. */

	return(-1);
}


int gbn_connect(int sockfd, const struct sockaddr *server, socklen_t socklen){

	/* TODO: Your code here. */
	if (sockfd < 0) {
		return(-1);
	}
	else {
		gbnhdr create_syn_header = make_header(SYN, 0);
		int senddata = sendto(sockfd, &create_syn_header, 4, 0, server, socklen); //hardcoded 4 since that's always the length of the packet header
		// 4 from: SYN =1 byte, seqnum is 1 byte, checksum is 16 byte, and data is always empty for the SYN packet
		if (senddata == -1) return(-1);
	}
return 0;
}


int gbn_listen(int sockfd, int backlog){
gbnhdr ack_packet = recvfrom(sockfd, &buffer, sizeof buffer, 0, sockaddr, sender_socklen_global);
	/* TODO: Your code here. */
	//here we will check if our SYN packet is correct
	// then we will move on to 'gbn_accept' to send a SYNACK back
	int check_val = check_header(&ack_packet, sizeof ack_packet);
	if (check_val == -1){
		return (-1);
	}
	fprint ("we are good to go")
	return 0;

	// return 0;
	// return (recvfrom(sockfd, backlog));
}


int gbn_bind(int sockfd, const struct sockaddr *server, socklen_t socklen){
	/* TODO: Your code here. */

sockaddr = server;
socklen_t = socklen;

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

	return(-1);
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
