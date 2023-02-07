//Timothy Nicholson
// ECE4470 Lab3

// Client-side UDP Code 
// Written by Sarvesh Kulkarni <sarvesh.kulkarni@villanova.edu>
// Adapted for use from "Beej's Guide to Network Programming" (C) 2017


#include <stdio.h>		// Std i/o libraries - obviously
#include <stdlib.h>		// Std C library for utility fns & NULL defn
#include <unistd.h>		// System calls, also declares STDOUT_FILENO
#include <errno.h>	    // Pre-defined C error codes (in Linux)
#include <string.h>		// String operations - surely, you know this!
#include <sys/types.h>  // Defns of system data types
#include <sys/socket.h> // Unix Socket libraries for TCP, UDP, etc.
#include <netinet/in.h> // INET constants
#include <arpa/inet.h>  // Conversion of IP addresses, etc.
#include <netdb.h>		// Network database operations, incl. getaddrinfo
#include <math.h>       // Math functions (like ceil())
#include <stdint.h>		// Allows the use of uint_t data types

// Our constants ..
#define MAXBUF 10000      // Max buffer size for i/o over nwk
#define MAXBUF1 4096 	  // Maximun amount of bytes in buffer
#define SRVR_PORT "6666"  // the server's port# to which we send data
						  // NOTE: ports 0 -1023 are reserved for superuser!

// Function to convert a string number into an integer
int str2int_num(char s[], int start, int digits);
// Checksum algorithm
uint16_t cksum(uint16_t *buf, int count);
// Converts integer checksum into a char array
void chk_to_arr(char chk[], int check);

int main(int argc, char *argv[]) {
	
    int sockfd;			 // Socket file descriptor; much like a file descriptor
    struct addrinfo hints, *servinfo, *p; // Address structure and ptrs to them
    int rv, nbytes, nread, attempts=0;
	char buf[MAXBUF], filechksum[4];
while(attempts<2)
{
    if (argc != 3) {
        fprintf(stderr,"ERROR! Correct Usage is: ./program_name server userid\n"
		        "Where,\n    server = server_name or ip_address, and\n"
		        "    userid = your LDAP (VU) userid\n");
        exit(1);
    }
//
	// First, we need to fill out some fields of the 'hints' struct
    memset(&hints, 0, sizeof hints); // fill zeroes in the hints struc
    hints.ai_family = AF_UNSPEC;     // AF_UNSPEC means IPv4 or IPv6; don't care
    hints.ai_socktype = SOCK_DGRAM;  // SOCK_DGRAM means UDP

	// Then, we call getaddrinfo() to fill out other fields of the struct 'hints
	// automagically for us; servinfo will now point to the addrinfo structure
	// of course, if getaddrinfo() fails to execute correctly, it will report an
	// error in the return value (rv). rv=0 implies no error. If we do get an
	// error, then the function gai_strerror() will print it out for us
    if ((rv = getaddrinfo(argv[1], SRVR_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
//
    // We start by pointing p to wherever servinfo is pointing; this could be
	// the very start of a linked list of addrinfo structs. So, try every one of
	// them, and open a socket with the very first one that allows us to
	// Note that if a socket() call fails (i.e. if it returns -1), we continue
	// to try opening a socket by advancing to the next node of the list
	// by means of the stmt: p = p->ai_next (ai_next is the next ptr, defined in
	// struct addrinfo).
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("CLIENT: socket");
            continue;
        }

        break;
    }
//
	// OK, keep calm - if p==NULL, then it means that we cycled through whole
	// linked list, but did not manage to open a socket! We have failed, and
	// with a deep hearted sigh, accept defeat; with our tail between our legs,
	// we terminate our program here with error code 2 (from main).
    if (p == NULL) {
        fprintf(stderr, "CLIENT: failed to create socket\n");
        return 2;
    }

	// If p!=NULL, then things are looking up; the OS has opened a socket for
	// us and given us a socket descriptor. We are cleared to send! Hurray!
	// The sendto() function will report how many bytes (nbytes) it sent; but a
	// negative value (-1) means failure to send. Sighhh. If nbytes is +ve,
	// don't rejoice yet; it is still possible for the packet to be lost/damaged
	// in transit - we won't know anything about this. This is UDP after all!
    if ((nbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
             p->ai_addr, p->ai_addrlen)) == -1) {
        perror("CLIENT: sendto");
        exit(1);
    }

    printf("CLIENT: sent '%s' (%d bytes) to %s\n", argv[2], nbytes, argv[1]);
//
	// Recv packet from server. You should modify this part of the program so that
	// you can receive more than 1 packet from the server. In fact, you should
	// call recvfrom() repeatedly till all parts of the file have been received.
	// Each time you call recvfrom(), buf will be loaded with new incoming data;
	// since the old contents of buf will be overwritten, you must save those
	// contents elsewhere before calling recvfrom().
	nread = recvfrom(sockfd,buf,MAXBUF,0,NULL,NULL);
	if (nread<0) {
		perror("CLIENT: Problem in recvfrom");
		exit(1);
	}
	// we recvd our very first packet from sender: <filesize,filename>
	printf("Received %d bytes\n\n",nread);
	// Output recvd data to stdout; 
	 if (write(STDOUT_FILENO,buf,nread) < 0) {
	  perror("CLIENT: Problem writing to stdout");
		exit(1);
	}
	printf("\n\n");
//	
	// Store the name of incoming file into an array
	char* dir = "./data_files/"; // ensures file is stored in data_files folder
	char name[strlen(dir)+strlen(buf+8)+1]; // "dynamically" allocates size 
											// based on length of file name
	// Combines dir and the name of the incoming file
	snprintf (name, sizeof(name), "%s%.*s", dir, (int)strlen(buf+8), buf+8);
//
	// Copy the chksum into an array
		memcpy(filechksum, buf+4, 4);
		printf("File chksum: %.4s\n", filechksum);
	
	// Isolate file size and convert it from string to integer
	int file_size = str2int_num (buf, 0, 4);
	printf("File size: %d bytes\n", file_size);

	// Creates array to fit total size of all packets
	int packets = file_size/100;
	if((file_size%100) > 0)
		packets++;
	printf("# of packets: %d\n\n", packets);

	char order[file_size];
//
	//Receive the other packages and display their contents on terminal
	for (int remain_data = file_size; remain_data != 0;)
	{
		// Read received packet
		nread = recvfrom(sockfd,buf,MAXBUF,0,NULL,NULL);
		if (nread<0) {
			perror("CLIENT: Problem in recvfrom");
			exit(1);
		}
		
		// Convert payload size to integer
		int data_size = str2int_num (buf, 3, 3);
		
		// Convert the sequence number into an integer
		int seq_num = str2int_num (buf, 0, 2);
		
		// Prints the packet number and size of the received packet
		printf("Received packet #%d with a size of %d bytes\n", seq_num, nread);
		printf ("Payload size: %d\n\n", data_size);
			
		// Insert the data payload in the correct index in order[]
		memcpy((order+(seq_num*100)), buf+6, data_size); 
		
		remain_data = remain_data - data_size;
	}
//
	char bufx[MAXBUF1], chksum[5];
	int count = 0;
	
	memcpy(bufx, order, file_size);
	count = file_size;
	
	printf("CHECKSUM CHECK:\n\n");
	printf("Original string length: %d\n\n", count);
	// Pads string with '\0' if string has odd number of bytes
	if(count%2)
	{
		bufx[count] = '\0';
		count++;
	}
	printf("Adjusted string length (to ensure it's even): %d\n\n", count);
	
	// Copy bufx into array of uint16_t data type
	uint16_t newbuf[MAXBUF1/2];
	memcpy(newbuf, bufx, count);
	
	uint16_t check = cksum(newbuf, count);
	// Store checksum value in char array
	chk_to_arr(chksum, check);
	printf("Computed Checksum: %.4s\n", chksum);
//
	// Check if checksums match
	printf("\nComparing checksums...\n");
	int same = 1;
	for(int k=0; k<4; k++)
	{
		if(chksum[k] != filechksum[k])
			same = 0;
	}

	if(same)
	{
		printf("Checksums match!\n");
		printf("Writing packets to a file...\n");
		
		// Creates the file
		FILE *fp;
		fp = fopen(name, "w");
		if(fp == NULL) // If file didn't open, end the program
		{
			printf("Error: File did not open\n\n");
			freeaddrinfo(servinfo);
    		close(sockfd);
			return -1;
		}
		// Write the data payloads to fp in the correct order
		fprintf(fp, "%.*s", file_size, order);
		printf("Complete!\n\n");
		fclose(fp);
		break;
	}
	else
	{
		attempts++;
		if(attempts == 2)
			printf("Too many failed attempts. Try again later\n\n");
		else
		{
				char ans[1]; int correct = 1;
				printf("Checksums didn't match and file was dropped. Would you like to try again? [y/n]: ");
				
				while(correct)
				{
					int fill = scanf("%1s", ans);
					if (fill != 1)
					{
						printf("\nThere was an issue reading the input. TERMINATING PROGRAM\n\n");
						freeaddrinfo(servinfo);
    					close(sockfd);
    					return 0;
					}

					switch(*ans)
					{
						case 'y':
						case 'Y':
							printf("Request resent\n");
							correct = 0;
							break;
						case 'n':
						case 'N':
							printf("OK. Maybe try again later.\n\n");
							correct = 0;
							attempts = 3;
							break;
						default:
							printf("Unknown input. Please enter 'y' or 'n'.\n");
							break;
					}
				}
		}
	}
}
	// AFTER all packets have been received ....
	// free up the linked-list memory that was allocated for us so graciously by
	// getaddrinfo() above; and close the socket as well - otherwise, bad things
	// could happen
    freeaddrinfo(servinfo);
    close(sockfd);
    return 0;
}
//
int str2int_num(char s[], int start, int digits)
{
	//start = index of the most significant digit of the number
	//digits = number of digits in the number
	int num = 0;
	for (int k=start; k < (digits + start); k++)
		num = num*10 + (s[k] - 48);

	return num;
}
uint16_t cksum(uint16_t *buf, int count)
{
	register uint32_t sum = 0;
	while(count)
	{
		sum += *buf++;
		if(sum & 0xffff0000)
		{
			// carry occurred, so wrap around
			sum &= 0xffff;
			sum++;
		}
		count -= 2;
	}
	return ~(sum & 0xffff);
}
void chk_to_arr(char chk[], int check)
{
	int offset = 0, place;
	char hex = '0';
	while(offset < 4)
	{
		int power = 1;
		for(int i = 0; i < (3-offset); i++) // Hard code for pow()
			power *= 16;
		place = check/power;
		switch(place)
		{
			case 15:
				hex = 'F';
				break;
			case 14:
				hex = 'E';
				break;
			case 13:
				hex = 'D';
				break;
			case 12:
				hex = 'C';
				break;
			case 11:
				hex = 'B';
				break;
			case 10:
				hex = 'A';
				break;
			case 9:
				hex = '9';
				break;
			case 8:
				hex = '8';
				break;
			case 7:
				hex = '7';
				break;
			case 6:
				hex = '6';
				break;
			case 5:
				hex = '5';
				break;
			case 4:
				hex = '4';
				break;
			case 3:
				hex = '3';
				break;
			case 2:
				hex = '2';
				break;
			case 1:
				hex = '1';
				break;
			case 0:
				hex = '0';
				break;
		}
		chk[offset] = hex;
		check = check % power;
		offset++;
	}
	return;
}