/*
** selectserver.c -- a cheezy multiperson chat server
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<ctype.h>
#include <netdb.h>
#include "../include/global.h"
#include "../include/logger.h"
//#define PORT "9035" // port we're listening on
//#define DPORT "4950"
#define STDIN 0
#define MAXDATASIZE 4000

// struct for storing LIST values
struct clientlist {
	//int list_id;
	char hostname[100];
	char ip_addr[100];
	int port_num;
	int sockfd;
	char block[500];
	char status[30];
	int m_sent;
	int m_recv;
	char *bmsg;
};

char list[MAXDATASIZE];


struct clientlist clist[4];
int no = 0;

struct slist {
	//int list_id;
	char hostname[200];
	char ip_addr[100];
	int port_num;
	char block[500];
	
};

struct slist sl[4];
int no1 = 0;

struct blockedlist
{
	char hostname[100];
	char ip_addr[100];
	int port_num;
};

struct blockedlist blist[3];
int no2 = 0;


int validip(char *ipaddr)
{
	struct sockaddr_in sa;
	return inet_pton(AF_INET, ipaddr, &(sa.sin_addr)) != 0;
}
/*
int validport(char *port)
{
	while(*port)
	{
		if(isdigit(*port++) == 0)
			return 0;
	}
	return 1;
}
*/
void sort_client(struct clientlist *p, int no)
{
	int i,j;
	struct clientlist q;
	for(i=1; i<no; i++)
	{
		for(j=0; j<no-i; j++)
		{
			if(p[j].port_num > p[j+1].port_num)
			{
				q = p[j];
				p[j] = p[j+1];
				p[j+1] = q;
			}
		}
	}
}

void sort_blockclient(struct blockedlist *p, int no)
{
	int i,j;
	struct blockedlist q;
	for(i=1; i<no; i++)
	{
		for(j=0; j<no-i; j++)
		{
			if(p[j].port_num > p[j+1].port_num)
			{
				q = p[j];
				p[j] = p[j+1];
				p[j+1] = q;
			}
		}
	}
}

void add_client(struct clientlist *p, struct clientlist a, int *no)
{
	int i, flag=0;
	for(i=0; i<*no; i++)
	{
		if(strcmp(p[i].ip_addr, a.ip_addr) == 0 && strcmp(p[i].status, "logged-out") == 0)
		{
			flag = 1;		
			memset(p[i].status,0,sizeof(p[i].status));
			strcpy(p[i].status,"logged-in");
			p[i].sockfd = a.sockfd;
		}
		else if(strcmp(p[i].ip_addr, a.ip_addr) == 0 && strcmp(p[i].status, "logged-in") == 0)
		{
			flag = 2;
		}
	}
	if(flag == 0) {
		if(*no < 4)
		{
			p[*no] = a;
			*no += 1;
		}
	}
	//sort_client(p,no);
}

void print_statistics(struct clientlist *p, int no)
{
	char *command_str = "STATISTICS";
	int i, j=0;
	cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
	for(i=0; i<no; i++)
	{
		cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", i+1, p[i].hostname, p[i].m_sent, p[i].m_recv, p[i].status);
	}
	cse4589_print_and_log("[%s:END]\n", command_str);
}

void print_client(struct clientlist *p, int no)
{
	char *command_str = "LIST";
	
	int i, j=0;
	//printf("No: %d", no);
	cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
	for(i=0; i<no; i++)
	{
		
		if(strcmp(p[i].status,"logged-in") == 0)
		{
		//printf("It came till here!");
			
			cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", j+1,p[i].hostname,p[i].ip_addr,p[i].port_num);
			printf("Msgs sent: %d, Msgs recvd: %d\n", p[i].m_sent, p[i].m_recv);
			j++;
			//printf("%-5d%-35s%-20s%-8d\n",i+1,p[i].hostname,p[i].ip_addr,p[i].port_num);
			//printf("Socket no:%d\tBlocked ip:%s, status:%s,\n",p[i].sockfd,p[i].block,p[i].status);
		}
		
	}
	cse4589_print_and_log("[%s:END]\n", command_str);
	
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int get_port(struct sockaddr *sa) {
	if(sa->sa_family == AF_INET ) {
		return ((struct sockaddr_in*)sa)->sin_port;	
	}
	return ((struct sockaddr_in6*)sa)->sin6_port;
}

void get_ip()
{
	char hostname[128];
	char *command_str = "IP";
	gethostname(hostname, sizeof hostname);
	//printf("My hostname: %s\n", hostname);

	 struct addrinfo hints, *res, *p;
	 int status;
	 char ipstr[INET6_ADDRSTRLEN];
	
	 memset(&hints, 0, sizeof hints);
	 hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	 hints.ai_socktype = SOCK_STREAM;
	 if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
	 fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
	 return;
	 }
	 for(p = res;p != NULL; p = p->ai_next) {
		 void *addr;
		 char *ipver;
		 // get the pointer to the address itself,
		 // different fields in IPv4 and IPv6:
		 if (p->ai_family == AF_INET) { // IPv4
			 struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			 addr = &(ipv4->sin_addr);
			 ipver = "IPv4";
		 } else { // IPv6
			 struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			 addr = &(ipv6->sin6_addr);
			 ipver = "IPv6";
		 }
		 // convert the IP to a string and print it:
		 inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		 //printf(" IP:%s\n", ipstr);
		 cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
		cse4589_print_and_log("IP:%s\n", ipstr);
		cse4589_print_and_log("[%s:END]\n", command_str);
	}
	 freeaddrinfo(res); // free the linked list
	 return;

	
}

int main(int argc, char *argv[])
{
	
	// Init Logger
	cse4589_init_log(argv[2]);

	// Clear LOGFILE
	fclose(fopen(LOGFILE,"w"));	

	// Server program
	if(strcmp(argv[1],"s") == 0) 
	{
		
		fd_set master; // master file descriptor list
		fd_set read_fds; // temp file descriptor list for select()
		int fdmax; // maximum file descriptor number
		int listener; // listening socket descriptor
		int newfd; // newly accept()ed socket descriptor
		struct sockaddr_storage remoteaddr; // client address
		socklen_t addrlen;
		char *buf, buf1[256]; // buffer for client data
		char buffer[450];
		int nbytes;
		char remoteIP[INET6_ADDRSTRLEN];
		int yes=1; // for setsockopt() SO_REUSEADDR, below
		int i, i1, j, rv, sfd, i2, j1, bdcast = 0, k, k1;
		
		struct clientlist a;
		struct addrinfo hints, *res, *q;
		int status;
		// BLOCK declarations
		int flag_blocked = 0;
		// Logged out
		int flag_logged = 0;
		int clist_no;
		int cpt; // client listening port
		int blockip=0;
		char unblock[200], command_str[20];
		char ipstr[INET6_ADDRSTRLEN];
		
		
		char str[200], msg[256], *msg1, msg2[256], *cip, *bip, *sourceip, portn[20], *blmsg, *block[3];
		char ubit[50] = "suppu";
		char *t, *str1, *t1, *a1[3], *b, *t2;

		struct addrinfo *ai, *p;
		FD_ZERO(&master); // clear the master and temp sets
		FD_ZERO(&read_fds);
		
		// Hostname of clients
		char host[1024];
		char service[20];
		
		char *PORT = argv[2];
		
	// get us a socket and bind it
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
			fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
			exit(1);
		}


		//printf("IP address:");
		for(p = ai; p != NULL; p = p->ai_next) {
			listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			if (listener < 0) {
				continue;
			}
			// lose the pesky "address already in use" error message
			setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
			if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
				close(listener);
				continue;
			}
			break;
		}
		// if we got here, it means we didn't get bound
		if (p == NULL) {
			fprintf(stderr, "selectserver: failed to bind\n");
			exit(2);
		}
		freeaddrinfo(ai); // all done with this
		// listen
		if (listen(listener, 10) == -1) {
			perror("listen");
			exit(3);
		}
		// add the listener to the master set
		FD_SET(listener, &master);
		// keep track of the biggest file descriptor
		fdmax = listener; // so far, it's this one
		// main loop
		for(;;) {
			read_fds = master; // copy it
			FD_SET(STDIN, &read_fds);
			if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
				perror("select");
				exit(4);
			}
			// run through the existing connections looking for data to read
			if (FD_ISSET(STDIN, &read_fds)) {
				//printf("A key was pressed!\n");	
				fgets(str,200,stdin);
				str1 = strtok(str, "\n");
				//printf("Input command: %s333\n",t);
				str1 = strtok(str," ");
				//scanf("%s",str);
				//printf("str: %s",str);
				
				// AUTHOR command
				if(strcmp(str1,"AUTHOR") == 0) {
					//printf("I, %s, have read and understood the course academic integrity policy.\n",ubit);
					cse4589_print_and_log("[%s:SUCCESS]\n", str);
					cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubit);
					cse4589_print_and_log("[%s:END]\n", str);					
				}
				// IP command
				if(strcmp(str1, "IP") == 0) {
					get_ip();
				}
				// PORT command
				if(strcmp(str1, "PORT") == 0) {
					cse4589_print_and_log("[%s:SUCCESS]\n", str);
					cse4589_print_and_log("PORT:%d\n", atoi(PORT));
					cse4589_print_and_log("[%s:END]\n", str);
					//printf("PORT:%d\n",atoi(PORT));
				}

				// LIST command
				if(strcmp(str1,"LIST") == 0) {
					print_client(clist, no);	
				}
				
				// BLOCKED command
				if(strcmp(str1, "BLOCKED") == 0) {
					
					memset(&blist, 0, sizeof blist);
					
					i2 = 0;
					while(str1 != NULL) {
						if(i2 == 1)
						{
							printf("Blocked command: %s,\t",str1);
							bip = str1; // check if bip is causing an override of any other ip somewhere
							printf("Bip in loop: %s\t",bip);
						}
						str1 = strtok(NULL, " ");
						i2++;
					}
					printf("Client ip for blocked: %s,\n",bip);
					
					if(validip(bip) == 0)
					{
						cse4589_print_and_log("[BLOCKED:ERROR]\n");
						cse4589_print_and_log("[BLOCKED:END]\n");
					}
					
					else 
					{
						blockip = 0;
						// check for the client ip in struct list and load all the blocked ip's into *block array
						for(i2=0; i2<no; i2++)
						{
							printf("Clist ip: %s,\n",clist[i2].ip_addr);
							if(strcmp(clist[i2].ip_addr,bip) == 0)
							{
								blockip = 1;
								printf("Blocked ips: %s,\t",clist[i2].block);
								blmsg = clist[i2].block;
							}
						}
						
						if(blockip == 0)
						{
							cse4589_print_and_log("[BLOCKED:ERROR]\n");
							cse4589_print_and_log("[BLOCKED:END]\n");
						}
						
						else
						{
							t2 = strtok(blmsg, ",");
							i2 = 0;
							while(t2 != NULL)
							{
								block[i2] = t2;
								printf("i2: %d\tblock[i2]: %s\n",i2,block[i2]);
								t2 = strtok(NULL, ",");
								i2++;
							}
							no2 = i2;
							for(k = 0; k < i2; k++)
							{
								for(k1 = 0; k1 < no; k1++)
								{
									if(strcmp(block[k],clist[k1].ip_addr) == 0)
									{
										printf("Entered inner loop!");
										printf("block[k]:%s,clist[k1].ip_addr:%s,\n",block[k],clist[k1].ip_addr);
										strcpy(blist[k].hostname, clist[k1].hostname);
										strcpy(blist[k].ip_addr, clist[k1].ip_addr);
										blist[k].port_num = clist[k1].port_num;
									}
								}
							}
							
							sort_blockclient(blist,no2);
							cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
							for(i2=0; i2<no2; i2++)
							{
								
								cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i2+1,blist[i2].hostname,blist[i2].ip_addr,blist[i2].port_num);
							}
							cse4589_print_and_log("[BLOCKED:END]\n");
						}
					}
					
				}
				
				// STATISTICS command
				if(strcmp(str1, "STATISTICS") == 0)
				{
					sort_client(clist, no);
					print_statistics(clist, no);
				}
				
			}

			for(i = 0; i <= fdmax; i++) {
				if (FD_ISSET(i, &read_fds)) { // we got one!!
					//printf("Step checking i in read_fds");
					if (i == listener && i != STDIN) {
						// handle new connections
						addrlen = sizeof remoteaddr;
						newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
						if (newfd == -1) {
							perror("accept");
						} 
						else {
							memset(list,0,sizeof(list));
							strcpy(list, "LIST");
							FD_SET(newfd, &master); // add to master set
							if (newfd > fdmax) { // keep track of the max
								fdmax = newfd;
							}
							printf("selectserver: new connection from %s on "
							"socket %d and port %d\n", inet_ntop(remoteaddr.ss_family,	get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN), newfd, get_port((struct sockaddr*)&remoteaddr));

							//ip1 = inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr),remoteIP, 							INET6_ADDRSTRLEN);
							// gethostinfo
							memset(buf1,0,sizeof(buf1));
							if ((nbytes = recv(newfd, buf1, sizeof buf1, 0)) <= 0) {
							// got error or connection closed by client
								if (nbytes == 0) {
									// connection closed
									printf("selectserver: socket %d hung up\n", i);
									
								} 
								else {
									perror("recv");
								}
							}
							else if(nbytes > 0)
							{
								printf("Received port msg:%s\n",buf1);
								//memset(t,0,strlen(t));
								t = strtok(buf1," ");
								//printf("t in port:%s\t",t);
								j1 = 0;
								if(strcmp(t, "PORT") == 0)
								{
									while(t != NULL) {
									//	printf("t:%s, j1:%d\t",t,j1);
										if(j1 == 1)
										{
											cpt = atoi(t);
										}
										j1++;
										t = strtok(NULL," ");
									}
								}
								//printf("Received port no:%d\t",cpt);
								addrlen = sizeof remoteaddr;
								getnameinfo((struct sockaddr*)&remoteaddr, addrlen, host, 1024, service, 20, NI_NAMEREQD);
								
								strcpy(a.hostname, host);
								strcpy(a.ip_addr, inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN));
								a.port_num = cpt;
								a.sockfd = newfd;
								strcpy(a.status, "logged-in");
								add_client(clist, a, &no);
								sort_client(clist,no);
								
								for(i1=0; i1<no; i1++) {
									if(strcmp(clist[i1].status,"logged-in") == 0)
									{
										strcat(list, " ");
										strcat(list, clist[i1].hostname);
										strcat(list, " ");
										strcat(list, clist[i1].ip_addr);
										strcat(list, " ");
										sprintf(portn, "%d", clist[i1].port_num);
										strcat(list, portn);
									}
								}
								
								if(send(newfd, list, strlen(list), 0) == -1) {
									perror("send");	
								}
							}
						}
					} 
					else if(i != STDIN){
						//printf("Data incoming from a client");
						// handle data from a client
						memset(buf1,0,sizeof(buf1));
						if ((nbytes = recv(i, buf1, sizeof buf1, 0)) <= 0) {
							// got error or connection closed by client
							if (nbytes == 0) {
								// connection closed
								printf("selectserver: socket %d hung up\n", i);
								
							} 
							else {
								perror("recv");
							}
							
							
							close(i); // bye!
							FD_CLR(i, &master); // remove from master set
						}
						else {

							
							
							// Store source ip in sourceip char array so that it'll be easier to check if it is blocked or not by the dest ip.
							flag_blocked = 0;
							for(j=0; j<no; j++) {
								if(clist[j].sockfd == i)
								{
									sourceip = clist[j].ip_addr;
								}
							}
							printf("Msg received:%s,\n",buf1);
							buf = strtok(buf1, "\n");
						
							t1 = strtok(buf, " ");
							
							
							// REFRESH list for client
							if(strcmp(t1, "REFRESH") == 0) {
								memset(list, 0, sizeof(list));
								strcpy(list, "LIST");
								for(i1=0; i1<no; i1++) {
									if(strcmp(clist[i1].status,"logged-in") == 0)
									{
										strcat(list, " ");
										strcat(list, clist[i1].hostname);
										strcat(list, " ");
										strcat(list, clist[i1].ip_addr);
										strcat(list, " ");
										sprintf(portn, "%d", clist[i1].port_num);
										strcat(list, portn);
									}
								}
							
								if(send(i, list, strlen(list), 0) == -1) {
									perror("send");	
								}
								
							
							}
						
							// SEND to specific client
							else if(strcmp(t1, "SEND") == 0) {
								
								for(i2=0; i2<no; i2++)
								{
									if(clist[i2].sockfd == i) 
									{
										clist[i2].m_sent = clist[i2].m_sent + 1;
										break;
									}
								}
							
								i2 = 0;
								//printf("Msg t1: %s,\n",t1);
								memset(msg,0,sizeof(msg));
								strcpy(msg, sourceip);
								while (t1 != NULL)
								{
								
									if(i2 == 1)
									{
										printf("t in i=1:%s\n",t1);
										cip = t1;	
									}
									else if(i2 == 2) {
										msg1 = t1;
										//printf("t in i>2:%s\n",t1);
										strcpy(msg2,msg1);
										strcat(msg, " ");
										strcat(msg, msg1);
									}
									else if(i2 > 2) {
										strcat(msg2, " ");
										strcat(msg2, t1);
										strcat(msg, " ");
										strcat(msg, t1);
									}
									printf("t1: %s\t",t1);
							
									t1 = strtok (NULL, " ");
									i2++;
								}
								printf("Cip: %s,\t msg:%s,\n",cip,msg);
								
								for(i1=0; i1<no; i1++) {
									//printf("Clist ip:%s, Clist sockfd: %d\n", clist[i1].ip_addr, clist[i1].sockfd);							
									// Find the ip that matches with the dest ip in struct clientlist.	
									if(strcmp(clist[i1].ip_addr,cip) == 0) {
										// check if the dest client is logged out									
										if(strcmp(clist[i1].status,"logged-out") == 0)
										{
											printf("Logged out indeed!");
											flag_logged = 1;
											clist_no = i1;
										}
										// Check if the source ip is in blocked list of dest.
										//printf("Blocked ip for this client:%s,\n",clist[i1].block);
										if(strlen(clist[i1].block) != 0) {
											b = strtok(clist[i1].block, ",");
											while(b != NULL) {
												printf("b:%s,sourceip:%s,\n",b,sourceip);
												if(strcmp(b,sourceip) == 0) {
													flag_blocked = 1;
												}
												b = strtok(NULL, " ");	
											}
																			
										}
										sfd = clist[i1].sockfd;								
										//printf("Sockfd of the dest ip:%d\n",sfd);
										break;			
									}				
								}
								//printf("Sfd:%d,flag_blocked:%d,flag_logged:%d,struct no:%d\n",sfd, flag_blocked,flag_logged, clist_no);
								if(flag_blocked != 1) {
									if(flag_logged == 0) {
										if( send(sfd, msg, strlen(msg), 0) == -1) {
											//printf("Failing to send");
											perror("send");
										}
										else
										{
											for(i2=0; i2<no; i2++)
											{
												if(clist[i2].sockfd == sfd)
												{
													clist[i2].m_recv = clist[i2].m_recv + 1;
												}
											}
											strcpy(command_str,"RELAYED");
											cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
											cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", sourceip, cip, msg2);
											cse4589_print_and_log("[%s:END]\n", command_str);
										}
									}
									else
									{
										if(clist[clist_no].bmsg == NULL)
										{
											//printf("The dest ip is logged out and the buffered msgs are empty!");
											clist[clist_no].bmsg = sourceip;
											strcat(clist[clist_no].bmsg, " ");
											strcat(clist[clist_no].bmsg, msg);
											
											printf("Buffered msg for logged out client first time: %s,\n",clist[clist_no].bmsg);																				
										}
										else
										{
											strcat(clist[clist_no].bmsg, ",");
											strcat(clist[clist_no].bmsg, sourceip);
											strcat(clist[clist_no].bmsg, " ");
											strcat(clist[clist_no].bmsg, msg);
											printf("Buffered msg for logged out client: %s,\n",clist[clist_no].bmsg);
										}
									}
								} 
								//printf("Send to dest client status: %s,\t",send(sfd,a1[2],strlen(a1[2]),0));
								
							}

							// BROADCAST from client
							else if(strcmp(t1, "broadcast") == 0) {
								
								for(i2=0; i2<no; i2++)
								{
									if(clist[i2].sockfd == i) 
									{
										clist[i2].m_sent = clist[i2].m_sent + 1;
										break;
									}
								}
							
							
								i2 = 0;
								//printf("Msg t1: %s,\n",t1);
								memset(msg, 0, sizeof(msg));
								memset(msg2,0,sizeof(msg2));
								strcpy(msg, sourceip);
								
								while (t1 != NULL)
								{
									if(i2 == 1) {
										//msg1 = t1;
										strcpy(msg2,t1);
										strcat(msg, " ");
										strcat(msg,t1);
									}
									else if(i2 > 1)
									{
										strcat(msg2, " ");
										strcat(msg2, t1);
										strcat(msg, " ");
										strcat(msg, t1);
									}
									//b1[i1] = t1;
									//printf("%s\n",t);
									t1 = strtok (NULL, " ");
									i2++;
								}
								printf("Msg:%s,",msg);
								//printf("Clist ip:%s, no:%n\t",clist[1].ip_addr, no);							
								//printf("Comparing:%d\n",strcmp(clist[1].ip_addr,a1[1]));
								
								for(j = 1; j <= fdmax; j++) {
									// send to everyone!
									
									if (FD_ISSET(j, &master)) {
										for(i1 = 0; i1<no; i1++) {
											if(clist[i1].sockfd == j) {
												if(strlen(clist[i1].block) != 0) {
													b = strtok(clist[i1].block, ",");
													while(b != NULL) {
														//printf("b:%s,sourceip:%s,\n",b,sourceip);
														if(strcmp(b,sourceip) == 0) {
															flag_blocked = 1;
														}
														b = strtok(NULL, " ");	
													}
												}
											}
										}
										// except the listener and ourselves
										if (j != listener && j != i && flag_blocked != 1) {
											if (send(j, msg, strlen(msg), 0) == -1) {
												perror("send");
											}
											else
											{
												for(i2=0; i2<no; i2++)
												{
													if(clist[i2].sockfd == j)
													{
														clist[i2].m_recv = clist[i2].m_recv + 1;
														break;
													}
												}
												bdcast = 1;
												
											}
										}
									}
								}
								if(bdcast == 1)
								{
									cip = "255.255.255.255";
									memset(command_str,0,sizeof(command_str));
									strcpy(command_str,"RELAYED");
									cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
									cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", sourceip, cip, msg2);
									cse4589_print_and_log("[%s:END]\n", command_str);
								}
								
							}

							// BLOCK a specified client
							else if(strcmp(t1, "BLOCK") == 0) 
							{
								i1 = 0;
								while(t1 != NULL) {
									if(i1 == 1) {
										bip = t1;								
									}
									t1 = strtok (NULL, " ");
									i1++;							
								}
								//printf("Blocked ip:%s,\n",bip);
								for(j = 0; j<no; j++) {
									//printf("Entered the loop!");								
									if(clist[j].sockfd == i) {
										if(strlen(clist[j].block) == 0) {
											//printf("NULL man!");
											strcpy(clist[j].block, bip);
											//printf("Clist block:%s\n", clist[j].block);
										}
										else  {
											strcat(clist[j].block, ",");
											strcat(clist[j].block, bip);									
										}
										//printf("Blocked ip's for this client:%s,\n",clist[j].block);
									}				
									
									
								}
							}
							// UNBLOCK given ip for the source client
							else if(strcmp(t1, "UNBLOCK") == 0)
							{
								//printf("t1 in unblock:%s\n",t1);
								//memset(bip,0,strlen(bip));
								//memset(b,0,strlen(b));
								//memset(cip,0,strlen(cip));
								//printf("Came till here!");
								
								i1 = 0;
								while(t1 != NULL) {
									if(i1 == 1) {
										bip = t1;								
									}
									t1 = strtok (NULL, " ");
									i1++;							
								}
								//printf("Unblock ip:%s,\n",bip);
								
								for(i1 = 0; i1 < no; i1++) {
									if(clist[i1].sockfd == i) {
										//printf("Found the source ip in struct\n");
										cip = clist[i1].block;
										printf("cip:%s\n",cip);
										b = strtok(cip,",");
										printf("First b:%s\n",b);
										while(b != NULL)
										{
											printf("b: %s\n",b);
											if(strcmp(b,bip) != 0) {
												
												if(strlen(unblock) == 0)
												{
													strcpy(unblock, b);
												}
												else {
													strcat(unblock, ",");
													strcat(unblock, b);
												}
											}
											printf("Unblock:%s\n",unblock);
											b = strtok(NULL, " ");
										}
										printf("Final unblock list:%s,\n",unblock);
										memset(&clist[i1].block, 0, sizeof(clist[i1].block));
										strcpy(clist[i1].block, unblock);
										printf("Blocked ips of source after unblocking:%s\n",clist[i1].block);
																		
									}
								}
							}

							// Client LOGOUT 
							else if(strcmp(t1, "LOGOUT") == 0)	
							{
								for(i1=0; i1<no; i1++)
								{
									if(clist[i1].sockfd == i)
									{
										memset(clist[i1].status,0,sizeof(clist[i1].status));
										strcpy(clist[i1].status, "logged-out");
										break;
									}
								}
								close(i);
								FD_CLR(i,&master);
							}
							
							// client EXIT - change status of client to logged-out
							
							else if(strcmp(t1, "EXIT") == 0)	
							{
								for(i1=0; i1<no; i1++)
								{
									if(clist[i1].sockfd == i)
									{
										k = i1;
										break;
									}
								}
								close(i);
								FD_CLR(i,&master);
								
								for(i1=k; i1<no; i1++)
								{
									memset(&clist[i1],0,sizeof clist[i1]);
									if(i1+1 < no)
									{
										strcpy(clist[i1].hostname, clist[i1+1].hostname);
										strcpy(clist[i1].ip_addr, clist[i1+1].ip_addr);
										clist[i1].port_num = clist[i1+1].port_num;
										clist[i1].sockfd = clist[i1+1].sockfd;
										strcpy(clist[i1].block, clist[i1+1].block);
										strcpy(clist[i1].status, clist[i1+1].status);
										clist[i1].bmsg = clist[i1+1].bmsg;
										clist[i1].m_sent = clist[i1+1].m_sent;
										clist[i1].m_recv = clist[i1+1].m_recv;
										
									}
								}
								
							}
							

							/*printf("Already obtained data from a client");
							// we got some data from a client
							for(j = 0; j <= fdmax; j++) {
								// send to everyone!
								if (FD_ISSET(j, &master)) {
									// except the listener and ourselves
									if (j != listener && j != i) {
										if (send(j, buf, nbytes, 0) == -1) {
											perror("send");
										}
									}
								}
							} */
						}
					} // END handle data from client
				} // END got new incoming connection
			} // END looping through file descriptors
		} // END for(;;)--and you thought it would never end!
	}
	else if(strcmp(argv[1], "c") == 0)
	{
		
		int sockfd, numbytes;
		char buf[MAXDATASIZE];
		struct addrinfo hints, *servinfo, *p;
		int rv;
		char s[INET6_ADDRSTRLEN];

		char str[100];
		char ubit[50] = "suppu";
		char *t, *t2, *ip, *cip;
		int port, fdmax;
		char pt[20];
		char msg[256], msg2[300];
		char *msg1, buf1[MAXDATASIZE];
		char *a[3], *b[2];
		int i, log = 0, i1, j1, flag_block=0, flag_unblock=0, list_block=0, list_unblock=0;
		
		char *PORT = argv[2];
		char command[20], *sip;
		fd_set master;
		fd_set read_fds;
		
		FD_ZERO(&master);
		FD_ZERO(&read_fds);
		/*
		if (argc != 2) {
			fprintf(stderr,"usage: client hostname\n");
			exit(1);
		} */
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		fdmax = 0;
		
		for(;;) {
			read_fds = master;
			FD_SET(STDIN, &read_fds);
			if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
				perror("select");
				exit(4);
			}
			if (FD_ISSET(STDIN, &read_fds)) {
				fgets(str,200,stdin);
				t = strtok(str, "\n");
				//printf("Input command: %s333\n",t);
				t = strtok(str," ");
				

				// AUTHOR command
				if(strcmp(t,"AUTHOR") == 0) {
					//printf("I, %s, have read and understood the course academic integrity policy.\n",ubit);	
					cse4589_print_and_log("[%s:SUCCESS]\n", t);
					cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubit);
					cse4589_print_and_log("[%s:END]\n", t);
				}
				// IP command
				else if(strcmp(t, "IP") == 0) {
					//printf("Yoo!");
					get_ip();
				}
				// PORT command
				else if(strcmp(t, "PORT") == 0) {
					cse4589_print_and_log("[%s:SUCCESS]\n", t);
					cse4589_print_and_log("PORT:%d\n", atoi(PORT));
					cse4589_print_and_log("[%s:END]\n", t);
					//printf("PORT:%d\n",atoi(PORT));
				}
				
				// LIST command
				else if(strcmp(t, "LIST") == 0) {
					//command = "LIST";
					cse4589_print_and_log("[%s:SUCCESS]\n", t);
					for(j1=0; j1<no1; j1++)
					{
						
						cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", j1+1,sl[j1].hostname,sl[j1].ip_addr,sl[j1].port_num);
						
						//printf("%-5d%-35s%-20s%-8d\n",j1+1,sl[j1].hostname,sl[j1].ip_addr,sl[j1].port_num);
					} 
					cse4589_print_and_log("[%s:END]\n", t);
				}
				
				
				//t = strtok(str," ");
				//printf("%s:",t);
				//t = strtok(str," ");
				else if(strcmp(t, "LOGIN") == 0) {																			
					//printf("str: %s\n",str);
					//t = strtok(str, " ");
					i = 0;
					while (t != NULL && i < 3)
					{
						a[i] = t;
						//printf("%s\n",t);
						t = strtok (NULL, " ");
						i++;
					}
					ip = a[1];
					
					/*if(validip(ip) == 0)
					{
						cse4589_print_and_log("[LOGIN:ERROR]\n");
						cse4589_print_and_log("[LOGIN:END]\n");
					}
					//printf("LOG:%s\tIP:%s\n",a[0],ip);
					
					else
					{*/
						int nport=0;
						
						port = atoi(a[2]);
						sprintf(pt, "%d", port);
						
										
						/*if(validport(pt))
						{
							cse4589_print_and_log("[LOGIN:ERROR]\n");
							cse4589_print_and_log("[LOGIN:END]\n");
						}
						
						else
						{*/
							if ((rv = getaddrinfo(ip,pt, &hints, &servinfo)) != 0) {
							fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
							return 1;
							}
							// loop through all the results and connect to the first we can
							for(p = servinfo; p != NULL; p = p->ai_next) {
								if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
									perror("client: socket");
									continue;

								}
								if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
									close(sockfd);
									perror("client: connect");
									continue;
								}
								else
								{
									FD_SET(sockfd, &master);
									if (sockfd > fdmax) { // keep track of the max
										fdmax = sockfd;
									}
								}
								break;
							}
							if (p == NULL) {
								fprintf(stderr, "client: failed to connect\n");
								return 2;
							}
							log = 1;
							inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
							printf("client: connecting to %s\n", s);
							//memset(msg, 0, sizeof(msg));
							strcpy(msg,"PORT");
							strcat(msg," ");
							strcat(msg, PORT);
							printf("Porting msg after login:%s,\n",msg);
							if(send(sockfd,msg,strlen(msg),0) == -1)
							{
								perror("send");
							}
							//memset(msg,0,sizeof(msg));
							freeaddrinfo(servinfo); // all done with this structure
						//}
					//}
					//if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
					//	perror("recv");
					//	exit(1);
					//}
					//buf[numbytes] = '\0';
					//printf("client: received '%s'\n",buf);
				
				}
				
				else if(strcmp(t, "REFRESH") == 0 && log == 1)
				{
					if(send(sockfd, t, strlen(t), 0) == -1)
					{
						perror("send");
					}	
				}
				
				
				else if(strcmp(t, "SEND") == 0 && log == 1)
				{
					i = 0;	
					char *mip;
					msg1 = t;
					strcpy(msg, msg1);
					//printf("Initial msg check:%s\n",msg);
					while (t != NULL)
					{
						if(i == 1)
						{
							mip = t;
						}
						if( i >= 1 ) {
							msg1 = t;
							strcat(msg, " ");
							strcat(msg, msg1);
							//printf("t:%s\n",t);
						}
						t = strtok (NULL, " ");
						i++;
						
					}
					printf("mip:%s,\n",mip);
					if(validip(mip) == 0)
					{
						cse4589_print_and_log("[SEND:ERROR]\n");
						cse4589_print_and_log("[SEND:END]\n");
					}
					
					else
					{
						int flag_send = 0;
						
						for(i=0; i<no1; i++)
						{
							if(strcmp(sl[i].ip_addr, mip) == 0)
							{
								flag_send = 1;
							}
						}
						
						if(flag_send == 0)
						{
							cse4589_print_and_log("[SEND:ERROR]\n");
							cse4589_print_and_log("[SEND:END]\n");
						}
						
						else
						{
					
							printf("Client message: %s,",msg);
							
							if(send(sockfd, msg, strlen(msg),0) == -1)
							{
								perror("send");
							}	
						}
							
					}

				}
				else if(strcmp(t, "BROADCAST") == 0 && log == 1)
				{
					i = 0;
					//printf("Brd t:%s,\n",t);
					strcpy(msg, "broadcast");
					while (t != NULL)
					{
						if(i >= 1)
						{
							msg1 = t;
							strcat(msg," ");
							strcat(msg, msg1);
							
						}
						b[i] = t;
						//printf("%s\n",msg);
						t = strtok (NULL, " ");
						i++;
					}
					
					
					//printf("Client message: %s,",msg);
					if(send(sockfd, msg, strlen(msg),0) == -1)
					{
						perror("send");
					}	

				}
				else if(strcmp(t, "BLOCK") == 0 && log == 1)
				{
					i = 0;	
					msg1 = t;
					strcpy(msg, msg1);
					//printf("Initial msg check:%s\n",msg);
					while (t != NULL)
					{
						if( i >= 1 ) {
							msg1 = t;
							strcat(msg, " ");
							strcat(msg, msg1);
							//printf("t:%s\n",t);
						}
						t = strtok (NULL, " ");
						i++;
						
					}
					
					if(validip(msg1) == 0)
					{
						cse4589_print_and_log("[BLOCK:ERROR]\n");
						cse4589_print_and_log("[BLOCK:END]\n");
					}
					
					else {
					list_block = 0;
						for(i=0; i<no1; i++)
						{
							if(strcmp(msg1, sl[i].ip_addr) == 0)
							{
								list_block = 1;
							}
						}
						if(list_block == 0)
						{
							printf("IP not present in list");
							cse4589_print_and_log("[BLOCK:ERROR]\n");
							cse4589_print_and_log("[BLOCK:END]\n");	
						}
						
						else {
							flag_block = 0;
							for(i=0; i<no1; i++)
							{
								//atio(PORT);
								
								if(atoi(PORT) == sl[i].port_num)
								{
									if(strlen(sl[i].block) == 0)
									{
										strcpy(sl[i].block, msg1);
									}
									else
									{
										t2 = strtok(sl[i].block, ",");
										while(t2 != NULL)
										{
											if(strcmp(t2, msg1) == 0)
											{
												flag_block = 1;
											}
											t2 = strtok(NULL, ",");
										}
										if(flag_block == 1)
										{
											printf("Already blocked!");
											cse4589_print_and_log("[BLOCK:ERROR]\n");
											cse4589_print_and_log("[BLOCK:END]\n");
										}
										else {
											strcat(sl[i].block, ",");
											strcat(sl[i].block, msg1);
										}
									}
								}
							}
							//msg = strtok(msg, "\n");				
							//printf("Blocked msg: %s,",msg);
							
							if(flag_block == 0 && send(sockfd, msg, strlen(msg),0) == -1)
							{
								perror("send");
							}
						
						}
					}
				}
				else if(strcmp(t, "UNBLOCK") == 0 && log == 1)
				{
					i = 0;	
					msg1 = t;
					strcpy(msg, msg1);
					//printf("Initial msg check:%s\n",msg);
					while (t != NULL)
					{
						if( i >= 1 ) {
							msg1 = t;
							strcat(msg, " ");
							strcat(msg, msg1);
							printf("t:%s\n",t);
						}
						t = strtok (NULL, " ");
						i++;
						
					}
					printf("Msg1: %s,\t",msg1);
					if(validip(msg1) == 0)
					{
						cse4589_print_and_log("[UNBLOCK:ERROR]\n");
						cse4589_print_and_log("[UNBLOCK:END]\n");
					}
					
					else {
					list_unblock = 0;
						for(i=0; i<no1; i++)
						{
							if(strcmp(msg1, sl[i].ip_addr) == 0)
							{
								list_unblock = 1;
							}
						}
						if(list_unblock == 0)
						{
							printf("IP not present in list");
							cse4589_print_and_log("[UNBLOCK:ERROR]\n");
							cse4589_print_and_log("[UNBLOCK:END]\n");	
						}
						
						else {
							flag_unblock = 0;
							for(i=0; i<no1; i++)
							{
								//atio(PORT);
								
								if(atoi(PORT) == sl[i].port_num)
								{
									if(strlen(sl[i].block) == 0)
									{
										flag_unblock = 1;
										cse4589_print_and_log("[UNBLOCK:ERROR]\n");
										cse4589_print_and_log("[UNBLOCK:END]\n");
									}
									else
									{
										memset(msg2, 0, sizeof(msg2));
										printf("sl block: %s,\n",sl[i].block);
										t2 = strtok(sl[i].block, ",");
										while(t2 != NULL)
										{
											printf("t2: %s,\n",t2);
											if(strcmp(t2, msg1) == 0)
											{
												flag_unblock = 1;
											}
											else
											{
												
												if(strlen(msg2) == 0)
												{
													strcpy(msg2, t2);
												}
												else
												{
													strcat(msg2,",");
													strcat(msg2,t2);
												}
												printf("Msg2: %s,\n",msg2);
											}
											t2 = strtok(NULL, ",");
										}
										if(flag_unblock == 0)
										{
											printf("Not blocked yet!");
											cse4589_print_and_log("[UNBLOCK:ERROR]\n");
											cse4589_print_and_log("[UNBLOCK:END]\n");
										}
										else {
											printf("sl[i].block after strtok: %s,\n",sl[i].block);
											memset(&sl[i].block, 0, sizeof sl[i].block);
											strcpy(sl[i].block, msg2);
											printf("sl[i].block after copying msg2: %s,\n",sl[i].block);
											//strcat(sl[i].block, ",");
											//strcat(sl[i].block, msg1);
										}
									}
								}
							}
					
					//msg = strtok(msg, "\n");				
					//printf("Unblock msg: %s,",msg);
					
							if(flag_unblock == 1 && send(sockfd, msg, strlen(msg),0) == -1)
							{
								perror("send");
							}
						}
					}
				}
				
				else if(strcmp(t, "LOGOUT") == 0 && log == 1)
				{
					if(send(sockfd, t, strlen(t), 0) == -1)
					{
						perror("send");
					}
				}
				
				else if(strcmp(t, "EXIT") == 0 && log == 1)
				{
					if(send(sockfd, t, strlen(t), 0) == -1)
					{
						perror("send");
					}
					close(sockfd);
					FD_CLR(sockfd, &master);
					exit(0);
				}
				/*
				{
					if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
						perror("recv");
						exit(1);
					}
					buf[numbytes] = '\0';
					printf("client: received '%s'\n",buf);
				} */
			}

			for(i=0; i<=fdmax; i++) {
				if(i != STDIN && FD_ISSET(i, &read_fds)) {
					memset(buf, 0, sizeof(buf));
					if ((numbytes = recv(i, buf, MAXDATASIZE-1, 0)) == -1) {
						perror("recv");
						exit(1);
					}
					else if(numbytes > 0) {
						printf("Received: %s,\n",buf);
						memset(buf1,0,sizeof(buf1));
						strcpy(buf1,buf);
						t2 = strtok(buf1, " ");
						i1 = 0;
						//j1 = 0;
						if(strcmp(t2, "LIST") == 0)
						{
							memset(&sl,0,sizeof sl);
							no1 = 0;
							while(t2 != NULL)
							{
								if((i1%3) == 1)
								{
									strcpy(sl[no1].hostname,t2);
								}
								else if ((i1%3) == 2)
								{
									strcpy(sl[no1].ip_addr,t2);
								}
								else if(i1 != 0 && (i1%3) == 0)
								{
									sl[no1].port_num = atoi(t2);
									no1++;
								}
								t2 = strtok(NULL, " ");
								i1++;
							}

							/* printing LIST
							for(j1=0; j1<no1; j1++)
							{
								cse4589_print_and_log("[%s:SUCCESS]\n", command);
								cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", j1+1,sl[j1].hostname,sl[j1].ip_addr,sl[j1].port_num);
								cse4589_print_and_log("[%s:END]\n", command);
								//printf("%-5d%-35s%-20s%-8d\n",j1+1,sl[j1].hostname,sl[j1].ip_addr,sl[j1].port_num);
							} */
						}
						else {
							memset(msg,0,sizeof(msg));
							i1 = 0;
							sip = t2;
							while(t2 != NULL)
							{
								if(i1 == 1)
								{
									strcpy(msg, t2);
								}
								else if(i1 > 1)
								{
									strcat(msg, " ");
									strcat(msg, t2);
								}
								
								t2= strtok(NULL," ");
								i1++;
							}
							//buf[numbytes] = '\0';
							strcpy(command,"RECEIVED");
							cse4589_print_and_log("[%s:SUCCESS]\n", command);
							cse4589_print_and_log("msg from:%s\n[msg]:%s\n",sip,msg);
							cse4589_print_and_log("[%s:END]\n", command);
						}
					}
				}
			} 
		}
		close(sockfd);
	}
	return 0;
}



