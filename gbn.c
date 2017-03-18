#include "gbn.h"

state_t s_machine;

uint16_t checksum(uint16_t *buf, int nwords)
{
	uint32_t sum;

	for (sum = 0; nwords > 0; nwords--)
		sum += *buf++;
	sum = (sum >> 8) + (sum & 0xffff);
	sum += (sum >> 8);
	return ~sum;
}

gbnhdr make_header(int type_command, uint8_t sequence_number){
	gbnhdr header;
	header.type = type_command;
	header.seqnum = sequence_number;
	header.checksum = 0; // TODO: fill this in later
	// strcpy(header.data, ""); //TODO: fill this in later

	return header;
}

gbnhdr make_header_with_data(int type_command, uint8_t sequence_number, char *buffer, int data_length){
	gbnhdr header;
	header.type = type_command;
	header.seqnum = sequence_number;
	header.checksum = 0; // Default checksum value
	memcpy(header.data, buffer, DATALEN); //data capped at 1024 because that is the max packet size
	header.lenData = data_length;

	return header;
}

void timeout_handler() {
	printf("TIMEOUT HAPPENED.");
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
int send_packet(int sockfd,  char buf[], int data_length, uint8_t seqnum) {
	gbnhdr create_header_with_data = make_header_with_data(DATA, seqnum, buf, data_length);

	// Calculate checksum
	create_header_with_data.checksum = checksum(buf, create_header_with_data.lenData);
	printf("\nCHECKSUM: %d\n", create_header_with_data.checksum);

	printf ("data sent: %s\n", create_header_with_data.data);
	printf("DATA LENGTH %d\n", create_header_with_data.lenData);

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
		return -1;
	}
}

// checks if the data received is FIN packet
int check_if_fin_packet(char *buffer){
	if (buffer[0] == FIN){
		return 0;
	}
	else{
		return -1;
	}
}

int check_if_synack(char * buffer) {
	if (buffer[0] == SYNACK) {
		return 0;
	}
	return -1;
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

	// Tracks state of slow vs. fast mode. 0 = slow mode, 1 = fast mode
	int cur_mode = 0;
	int seed_seq_num = rand();
	s_machine.window_size = 2;
	s_machine.seqnum = seed_seq_num;

	//  make packets
	int orig_len = len;
	int cur_size = 0;
	int track = 0;
	int errno; // track timoue status
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
			int attempts = 0;
			memcpy(new_buf, buf + track, cur_size);
			s_machine.state = SYN_RCVD;
			while(s_machine.state != ACK_RCVD && attempts < 5) {
				int send_data = send_packet(sockfd, new_buf, cur_size, s_machine.seqnum);
				alarm(TIMEOUT);
				s_machine.state = DATA_SENT;


				gbnhdr * rec_buf = malloc(sizeof(*rec_buf));
				int ack_bytes = recvfrom(sockfd, rec_buf, sizeof * rec_buf, 0, sender_global, sender_socklen_global);

				// If sending data fails
				if (send_data == -1){
					printf("\nLUIS SEND DATA FAILED\n");
					attempts++;
				}

				// If ack not received before timeout
				else if (errno == EINTR) {
					printf("\n LUIS TIMEOUT OCCURED\n");
					attempts++;
				}

				else if (rec_buf->type != DATAACK) {
					printf("\n LUIS NOT AN ACK\n");
					attempts++;
				}

				// Make sure seqnum of ACK makes sense
				else if (rec_buf->seqnum != s_machine.seqnum){
					printf("\nNOT MATCHING SEQNUM\n");
					attempts++;
				}

				// OTHERWISE, ACK was received SUCCESFULLY
				else {
					s_machine.state = ACK_RCVD;
					cur_mode = 1;
				}
			}

			// CLOSE CONNECTION IF NOT SENT AFTER 5 TIMEs
			if (s_machine.state == DATA_SENT) {
				return -1;
			}

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
			// printf ("CUR SIZE: %d", cur_size);

			// Slow MODE
			if (cur_mode == 0) {
				int attempts = 0;
				memcpy(new_buf, buf + track, cur_size);
				s_machine.state = ESTABLISHED;
				while(s_machine.state != ACK_RCVD && attempts < 5) {
					int send_data = send_packet(sockfd, new_buf, cur_size, s_machine.seqnum);
					alarm(TIMEOUT);
					s_machine.state = DATA_SENT;


					gbnhdr * rec_buf = malloc(sizeof(*rec_buf));
					int ack_bytes = recvfrom(sockfd, rec_buf, sizeof * rec_buf, 0, sender_global, sender_socklen_global);

					// If sending data fails
					if (send_data == -1){
						printf("\nLUIS SEND DATA FAILED\n");
						attempts++;
					}

					// If ack not received before timeout
					else if (errno == EINTR) {
						printf("\n LUIS TIMEOUT OCCURED\n");
						attempts++;
					}

					else if (rec_buf->type != DATAACK) {
						printf("\n LUIS NOT AN ACK\n");
						attempts++;
					}

					// Make sure seqnum of ACK makes sense
					else if (rec_buf->seqnum != s_machine.seqnum){
						printf("\nNOT MATCHING SEQNUM\n");
						attempts++;
					}

					// OTHERWISE, ACK was received SUCCESFULLY. GO TO FAST MODE
					else {
						s_machine.state = ACK_RCVD;
						cur_mode = 1;
						s_machine.seqnum += 1;
						track += cur_size;
						len = len - cur_size;
					}
				}
			}

			// FAST MODE
			else {
				printf("\nENTERING FAST MODE\n");

				int errno;

				memcpy(new_buf, buf + track, cur_size);
				s_machine.state = ESTABLISHED;

				int first_p_track = track;
				int first_p_len = len;
				int first_seq_num = s_machine.seqnum;

				// send first packet
				int send_data_first = send_packet(sockfd, new_buf, cur_size, s_machine.seqnum);
				s_machine.state = DATA_SENT;


				// Clear the buffer
				memset(new_buf, '\0', sizeof(new_buf));

				// MOVE TO THE NEXT PACKET

				// If there's no more packets to be sent in between the first and second
				if (track >= orig_len) {
					continue;
				}

				// Prepare second packet
				s_machine.seqnum += 1;
				track += cur_size;
				len = len - cur_size;

				int second_p_track = track;
				int second_p_len = len;
				int second_seq_num = s_machine.seqnum;

				// Check for size of the next packet
				if (len >= DATALEN) {
					cur_size = DATALEN;
				}
				else {
					cur_size = len;
				}

				// send second packet
				memcpy(new_buf, buf + track, cur_size);
				int send_data_second = send_packet(sockfd, new_buf, cur_size, s_machine.seqnum);
				s_machine.seqnum += 1;
				track += cur_size;
				len = len - cur_size;

				// CHECK FIRST PACKET DELIVERY SUCCESS
				alarm(TIMEOUT);
				gbnhdr * rec_buf_first = malloc(sizeof(*rec_buf_first));
				int ack_bytes_first = recvfrom(sockfd, rec_buf_first, sizeof * rec_buf_first, 0, sender_global, sender_socklen_global);

				// Go back to slow mode with first packet
				if (send_data_first == -1){
					track = first_p_track;
					len = first_p_len;
					cur_mode = 0;
					s_machine.seqnum = first_seq_num;
					continue;
				}

				// Check if first packet timed out before ACK recieved
				else if (errno == EINTR) {
					track = first_p_track;
					len = first_p_len;
					cur_mode = 0;
					s_machine.seqnum = first_seq_num;
					continue;
				}

				// Make sure response is a DATAACK for first packet
				else if (rec_buf_first->type != DATAACK) {
					track = first_p_track;
					len = first_p_len;
					cur_mode = 0;
					s_machine.seqnum = first_seq_num;
					continue;
				}

				// CHECK IF SEQ_NUMS OF ACK MATCH
				// else if (rec_buf_first->seqnum != s_machine.seqnum - 2){
				// 	// printf("\n%s\n", "IN CHECK FOR FIRST PACKET SEQNUM" );
				// 	printf("\nFIRST SEQ NUM %d\n", s_machine.seqnum);
				// 	printf("\nRESPONSE SEQ NUM %d\n", rec_buf_first->seqnum - 2);
				// 	track = first_p_track;
				// 	len = first_p_len;
				// 	cur_mode = 0;
				// 	s_machine.seqnum = first_seq_num;
				// 	continue;
				// }


				// CHECK SECOND PACKET
				alarm(TIMEOUT);
				gbnhdr * rec_buf_second = malloc(sizeof(*rec_buf_second));
				int ack_bytes_second = recvfrom(sockfd, rec_buf_second, sizeof * rec_buf_second, 0, sender_global, sender_socklen_global);

				// Go back to slow mode with second packet
				if (send_data_second == -1){
					track = second_p_track;
					len = second_p_len;
					cur_mode = 0;
					s_machine.seqnum = second_seq_num;
					continue;
				}

				// Check if second packet timed out before ACK recieved
				else if (errno == EINTR) {
					track = second_p_track;
					len = second_p_len;
					cur_mode = 0;
					s_machine.seqnum = second_seq_num;
					continue;
				}

				// Make sure response is a DATAACK for second packet
				else if (rec_buf_second->type != DATAACK) {
					track = second_p_track;
					len = second_p_len;
					cur_mode = 0;
					s_machine.seqnum = second_seq_num;
					continue;
				}

				// CHECK IF SEQ_NUMS OF SECOND ACK MATCH
				// else if (rec_buf_second->seqnum != s_machine.seqnum - 2){
				// 	// printf("\n%s\n", "IN CHECK FOR FIRST PACKET SEQNUM" );
				// 	printf("\nFIRST SEQ NUM %d\n", s_machine.seqnum);
				// 	printf("\nRESPONSE SEQ NUM %d\n", rec_buf_first->seqnum - 2);
				// 	track = second_p_track;
				// 	len = second_p_len;
				// 	cur_mode = 0;
				// 	s_machine.seqnum = second_seq_num;
				// 	continue;
				// }

			}


			// memcpy(new_buf, buf + track, cur_size);
			// send_packet(sockfd, new_buf, cur_size, s_machine.seqnum);

			// track += cur_size;
			// len = len - cur_size;
		}
		printf("%s\n","PACKET SENT");
		// printf("track: %d %s", track, new_buf);
	}
	s_machine.isFin = 1;
	return len;
}


ssize_t gbn_recv(int sockfd, void *buf, size_t len, int flags){
	/* TODO: Your code here. */
	//char data_buffer[1028];
	gbnhdr * data_buffer = malloc(sizeof(*data_buffer));
	int bytes_recd_in_data = recvfrom(sockfd, data_buffer, sizeof *data_buffer, 0, receiver_global, &receiver_socklen_global);

	// Check the packet
	int packet_type_recd;
	packet_type_recd = check_if_data_packet(data_buffer);
	if (packet_type_recd == 0) {

		// Checksum logic
		// (data_buffer), bytes_recd_in_data)
		char cp_buf[data_buffer->lenData - 2];
		memcpy(cp_buf, data_buffer->data, data_buffer->lenData);
		printf("\nDATA ON RECIEVER %s\n", cp_buf);
		int cSum = checksum(buf, data_buffer->lenData);
		printf("\nCHECKSUM ON RECIEVER: %d\n", cSum);

		// Make sure checksums match
		// if (data_buffer->checksum != cSum) {
		// 	printf("DATA Packet failed Checksum");
		// 	return - 1;
		// }

		gbnhdr create_ack_header = make_header(DATAACK, data_buffer->seqnum);
		int sendack = sendto(sockfd, &create_ack_header, 4, 0, sender_global, sender_socklen_global);
		if (sendack == -1){
			return -1;
		}
	}
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
int gbn_close(int sockfd){
	/* TODO: Your code here. */

	if (sockfd < 0) {
		return(-1);
	}

	else {
		if (s_machine.isFin == 1) {
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
			if (sendfinack == -1)
				return -1;
			printf ("%s\n", "SENT THE FINACK");
			close(sockfd);
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

	// If connection is broken
	if (sockfd < 0)
		return(-1);

	int attempts = 0;
	gbnhdr create_syn_header = make_header(SYN, 0);
	s_machine.state = SYN_SENT;
	s_machine.window_size = 1; // only sending 1 SYN packet
	s_machine.isFin = 0;

	// Attempt to send the SYN packet up to 5 times
	while (s_machine.state == SYN_SENT && attempts < 5) {

		int errno; // Track error state for timeouts
		int sendsyn = sendto(sockfd, &create_syn_header, 4, 0, server, socklen); //hardcoded 4 since that's always the length of the packet header

		alarm(TIMEOUT); // Start a one second timer
		if (sendsyn == -1){
			continue;
		}

		char buf[1030]; // buffer for incoming ACK packet
		int ack_bytes = recvfrom(sockfd, buf, sizeof buf, 0, sender_global, sender_socklen_global);
		// Flow is paused until bytes are recieved

		// If nothing was recieved, aka a timeout occurred
		if (errno == EINTR) {
			printf("SYNACK NEVER CAME, TIMED OUT\n");
			attempts++;
		}
		else { // a packet was sent back from the reciever
			// If a synack was received
			if (check_if_synack(buf) == 0) {
				s_machine.state = ESTABLISHED;
				printf("MESSAGE SUCCESFULLY ACKED\n");
				return 0;
			}

			else {
				printf("SYNACK NOT RECEIVED, TRY AGAIN\n");
				attempts++;
			}

			receiver_global = server;
			receiver_socklen_global = socklen;
		}
	}

	if (attempts >= 5) {
		s_machine.state = CLOSED;
	}

	if (s_machine.state == CLOSED) {
		return -1;
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

	// Prepare timeout handling
	struct sigaction sact = {
    .sa_handler = timeout_handler,
    .sa_flags = 0,
  };

	sigaction(SIGALRM, &sact, NULL);

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
