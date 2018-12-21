#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>


/** 
 * Linked list
 * Current balance- double
 * Account name- char*
 * In-session flag- int
 * next node
 *
 * if In-session flag is 1, then mutex lock that account, save that account so you can reopen it after printing is done, for print to stdout
 *
 * */

sem_t mutex;

//my thread that accpets connections from the server. join it in the sighandler
pthread_t acceptor;

struct _customer{
	char* account;
	double balance;
	int insession;	
	int sockfd;
	struct _customer* next; 
};
	int sockfd, newsockfd, portno, clilen;
     	 char buffer[256];
    	 struct sockaddr_in serv_addr, cli_addr;
     	
typedef struct _customer customer;

customer* Chead=NULL;


struct _PTnode{
	int sockfd;
	struct _PTnode* next;
	int insession;
	pthread_t* tid;
};
typedef struct _PTnode PTnode;

//global 
//
PTnode* PThead=NULL;

struct itimerval s_time;
void* clienthandler(void*);

int check=0;
int ctrlc=0;

void sigc(int var){
//close sockets of every pthread-->send shutdown message before every close to respective sockfd's

	ctrlc=1;
	s_time.it_value.tv_sec=0;
	s_time.it_value.tv_usec=0;
	s_time.it_interval=s_time.it_value;
	if(setitimer(ITIMER_REAL,&s_time,NULL)<0){
		printf("error");
	}
	printf("\nShutting down server\n");
	return;

//close server socket	


}



void sighandler(int dummy){
//	printf("\nalarm went off\n");	
	// think
	//
//lock here

sem_wait(&mutex);

	if(Chead==NULL){
		printf("there are no accounts yet\n");
	}else{
		customer* ptr=Chead;
		while(ptr!=NULL){
			printf("%s\t%f\t",ptr->account,ptr->balance);
			if(ptr->insession==1){
				printf("IN SESSION\n");
			}else{
				printf("\n");
			}
			
			ptr=ptr->next;
		}

	}
sem_post(&mutex);
//unlock here
	return;
	
}
void error(char *msg){
    perror(msg);
    exit(1);
}

void* connectionAcceptor(void* placeholder){
//printf("here\n");
	while(ctrlc==0){
		int* newsockfd =malloc(sizeof(int));
			*newsockfd= accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	    	if (*newsockfd >= 0) {
			char* accept="Connection Accepted!";	
			printf("%s\n",accept);
	//		printf("newsockfd: %d\n",*newsockfd);
			pthread_t* newConn=(pthread_t*)malloc(sizeof(pthread_t));
			pthread_create(newConn, NULL, clienthandler,(void*)newsockfd);
			//add this pthread to a global LL		
			if(PThead==NULL){
				PThead=(PTnode*)malloc(sizeof(PTnode));
				PThead->sockfd=*newsockfd;
				PThead->tid=newConn;
				PThead->insession=1;
				PThead->next=NULL;		
			}else{
				//add to head
				PTnode* ptr=(PTnode*)malloc(sizeof(PTnode));
				ptr->sockfd=*newsockfd;
				ptr->tid=newConn;
				ptr->insession=1;
				ptr->next=PThead;
				PThead=ptr;
			}
		}
		PTnode* ptr = PThead;
		PTnode* prev=NULL;
		while(ptr!=NULL){
			if(ptr->insession==0){
				pthread_join(*(ptr->tid),NULL);
				if(prev!=NULL){
					prev->next=ptr->next;
					
				}else{
					PThead=ptr->next;
				}
				PTnode* temp=ptr->next;
				free(&(ptr->sockfd));
				free(ptr);	
				ptr=temp;
			}else{
				prev=ptr;
				ptr=ptr->next;		
			}
		}	
	}	

	//close client in client thread
	//	close(newsockfd);


}

void* clienthandler(void* nsfd){
	int socketfd=*(int*)nsfd;
//printf("clienthandler's sockfd: %d\n",socketfd);
	//recv and send to this sock fd
	int messageLength = 266;
	char* message=(char*)malloc(sizeof(char)*266);
	customer* c=NULL;//this is the node we will be doing all our operations on
	while(ctrlc==0){
		if(recv(socketfd,message, messageLength,0)<=0){
	//		printf("error receiving from client");
			break;
		}else{
//			printf("The message: %s ",message);
			
			//checking what the instruction is and doing respective operations

			if(message[0]=='c'&&message[1]=='r'){
				//create
				char* accname=(char*)malloc(sizeof(char)*strlen(&message[7]));    //&message[7];//i should memcpy this
				memcpy(accname,&message[7],strlen(&message[7]));

				//lock on Chead and check if this acc already exists
//-----------------------lock here
sem_wait(&mutex);
				customer* ptr=Chead;
				int accexists=0;
				while(ptr!=NULL){
					if(strcmp(ptr->account,accname)==0){
						accexists=1;
						send(socketfd,"This account already exists, try serve\n",40,0);
						break;
					}
					ptr=ptr->next;
				}
				//create the account fr fri
				if(c!=NULL&&c->insession==1){
					send(socketfd,"currently in a service session, try end before creating new account\n",255,0);
					continue;
				}
				customer* new =(customer*)malloc(sizeof(customer));
				if(accexists==0){	
					//initialize acc info and add to global LL (add to head)
					new->account=accname;
					new->balance=0;
					new->sockfd=socketfd;
					new->insession=0;
					new->next=Chead;
					Chead=new;
				//	c=new;
					send(socketfd,"account created\n",255,0);
				}else{
					free(new);//bc it already esixis and theres no point of mallocing space for it
				}
sem_post(&mutex);
//------------------------------unlock
			}else if(message[0]=='s'){
				//serve	

				char* accname=&message[6];//i should memcpy this---this data gets written over otherwise
				//node should already exists, if create was called then c should be initialized
				accname[strlen(accname)]='\0';
				if(c==NULL){
//---------------------------------lock 
					//finding node for this costumer
sem_wait(&mutex);		
					customer* ptr=Chead;
					while(ptr!=NULL){
						if(strcmp(ptr->account,accname)==0){
							//acc found
							c=ptr;
							break;
						}
						ptr=ptr->next;
					}
sem_post(&mutex);
//---------------------------------unlock?
					if(ptr==NULL){
						//acc was not found
						send(socketfd,"account not found, try create\n",31,0);
					}else{
						if(c->insession==1){
							if(c->sockfd==socketfd){
								send(socketfd,"sessions already in service, try any command\n",256,0);
							}else{
								send(socketfd,"session in service by another user, try a different account\n",256,0);
							}
						}else{
							c->insession=1;
							c->sockfd=socketfd;
							send(socketfd,"account now in session, try any command\n",256,0);
						}
					}
				}else{
					//account was created and set just have to put it in session
					if(c->insession==1){
						if(c->sockfd==socketfd){
							send(socketfd,"sessions already in service, try any command\n",256,0);
						}else{
							send(socketfd,"session in service by another user, try a different account\n",256,0);
						}
					}else{
						c->insession=1;
						send(socketfd,"account now in session, try any command\n",256,0);
					}
				}				
			}else if(message[0]=='d'){
				if(c==NULL){
					send(socketfd,"account not found, try serve or create\n",255,0);
//					continue;//yes?
				}else if(c->insession==1){
					c->balance+=atof(&message[8]);//should i memcpy this too? or does the function do this?
					send(socketfd,"balance update\n",255,0);
				}else{
					send(socketfd,"account not in session, try serve\n", 256,0);
				}
			}else if(message[0]=='w'){
				if(c==NULL){
					send(socketfd,"account not found, try serve or create\n",255,0);
//					continue;//yes?
				}else if(c->insession==1){
					if(c->balance-atof(&message[9])<0){
						send(socketfd,"insufficient funds\n",255,0);
					}else{
						c->balance-=atof(&message[9]);
						send(socketfd,"balance updated\n",255,0);
//						send(socketfd,strcat("your current balance is ",c->balance),255,0);
					}
				}else{
					send(socketfd,"account not in session, try serve\n", 256,0);
				}
			}else if(message[0]=='q'&&message[2]=='e'){//
				if(c==NULL){
					send(socketfd,"account not found, try serve or create\n",255,0);
				}else{
					if(c->insession==1){
						char* str =(char*)malloc(sizeof(char)*256);
				//		printf("hi bih");
						snprintf(str,255,"%f",c->balance);
				//		printf(str);
						strcat(str," is your balance\n");
				//		printf(str);
						send(socketfd,str,256,0);
					}
				}
			}else if(message[0]=='e'){//end--just change the insession, switch c back to null
 				if(c==NULL){
					send(socketfd,"account not found, try serve or create\n",255,0);
				}else if(c->insession==1){
					c->insession=0;
					c=NULL;
					send(socketfd,"account no longer in session\n",255,0);						
				}else{
					send(socketfd,"account currently not in session\n",255,0);
				}
			}else if(message[0]=='q'&&message[2]=='i'){//quit
				//return the socketfd? 
				//so this thread is over, we have to exit from it and join the thread back in the accept thread?? idk lol lemme ask zain
				//travese through PThead LL till i find my sockfd and then change the insession flag to 0---- gotta lock when i do it
				PTnode* ptr=PThead;
				if(c!=NULL){
					c->insession=0;
				}
				send(socketfd, "Quit recieved: ending sessions and closing client\n",255,0);
printf("client disconnecting\n");
				close(socketfd);
			/*	while(ptr!=NULL){
					if(ptr->sockfd==socketfd){
						ptr->insession=0;
						break;
					}
					ptr=ptr->next;
				}
			*/	if(ptr==NULL){
					printf("THIS SHOUOLD NEVER PRINT except for when theres no accts?? \n");
				}
				break;
			}else if(message[0]=='c'&&message[1]=='t'){
				PTnode* ptr=PThead;
				if(c!=NULL){
					c->insession=0;
				}
			//	send(socketfd, "Quit recieved: ending sessions and closing client\n",255,0);
				close(socketfd);
			/*	while(ptr!=NULL){
					if(ptr->sockfd==socketfd){
						ptr->insession=0;
						break;
					}
					ptr=ptr->next;
				}
			*/	if(ptr==NULL){
					printf("THIS SHOUOLD NEVER PRINT except for when theres no accts?? \n");
				}
printf("client disconnecting\n");
				break;
	
			}else{
				send(socketfd,"if this ever prints then something is weidrly fucked\n",255,0);
			}

			bzero(message,256);
		}

//		if(send(socketfd,message,strlen(message),0)<=0){
//			error("error in sending ");
//		}
		bzero(message, 256);
	}
	return;
}


int main(int argc, char *argv[]){
	sem_init(&mutex,0,1);

	signal(SIGALRM,sighandler);
	signal(SIGINT,sigc);
	s_time.it_value.tv_sec=15;
	s_time.it_value.tv_usec=15;
	s_time.it_interval=s_time.it_value;
	if(setitimer(ITIMER_REAL,&s_time,NULL)<0){
		printf("error");
	}
	 

	 int n;
     	 if (argc < 2) {
        	 fprintf(stderr,"ERROR, no port provided\n");
         	return 0;
     	 }
     	

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
     	if (sockfd < 0) 
        	error("ERROR opening socket");
     	


	bzero((char *) &serv_addr, sizeof(serv_addr));
     	portno = atoi(argv[1]);
     	serv_addr.sin_family = AF_INET;
     	serv_addr.sin_addr.s_addr = INADDR_ANY;
     	serv_addr.sin_port = htons(portno);
     	

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){ 
              error("ERROR on binding");
	}



	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	pthread_create(&acceptor, NULL, connectionAcceptor, NULL);
 //join this thread in the sighandler-->make this thread global

	while(ctrlc==0){
		//blocking from return
	}
	
	//joining loop--go through PThead and 
	//send shutdown message 
	//close sockets and 
	//join all throughs
	//free node 
	PTnode* ptr=PThead;
	while(ptr!=NULL){
		send(ptr->sockfd,"Server shutting down\n",255,0);
//		printf("seding shutdown message to sockfd %d\n",ptr->sockfd);
		close(ptr->sockfd);
		pthread_join(*(ptr->tid),NULL);
		ptr=ptr->next;
//		free(ptr);
//		ptr=temp;
	}	


	return 0; 
}

