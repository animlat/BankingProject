#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <sys/time.h>
#include <ctype.h>
int go=1;
int sockfd, portno, n;

struct sockaddr_in serv_addr;
struct hostent *server;

char buffer[266];//for scanning in commands and recieving messages from server
int shutdwn =0;
struct itimerval it_val;
int connectbool=0;

int quit=0;

int isNum(char current){
	if(current=='0'||current=='1'||current=='2'||current=='3'||current=='4'||current=='5'||current=='6'||current=='7'||current=='8'||current=='9'){
	return 1;
	
	}
	else{
		return 0;
	}


}

int ctrlc=0;

void sigc(int var){
	
	send(sockfd,"ctrlc",5,0);
	ctrlc=1;
	shutdwn=1;
	printf("Client will now shutdown\n");
	sleep(5);
	exit(1);	
	
	return;

}

void connecthandler(int dummy){
	if(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0){
		//do nothing
		printf("connecting failed. retrying in 3 seconds\n");
	}else{	
		printf("Connection Accepted!\n");
		connectbool=1;
		it_val.it_value.tv_sec=0;
		it_val.it_value.tv_usec=0;
		it_val.it_interval.tv_sec=0;
		it_val.it_interval.tv_usec=0;
		setitimer(ITIMER_REAL,&it_val,NULL);
	}
	return;
}

void* cmdread(void* var){

		char *token1;
		char *token2;
		char * invalidMsg="Invalid command, please input one of the following commands:\n No spaces in front of first command,if command has two parts, only one space in front of first command and no spaces after second parameter\n create\n serve\n withdraw\n deposit\n query\n end\n quit\n\n\n";	

	while(go==1&&ctrlc==0&&shutdwn==0){
	

		bzero(buffer,266);
		fgets(buffer,265,stdin);
		char copy[266];
		memcpy(copy,buffer,266);
		char *rest=copy;

if(ctrlc==1)
	break;

		int j = 0;
		int scount1=0;//number of spaces in buffer, if it exceeds 1 then bad input
		for(j=0;j<strlen(buffer);j++){
			if(' '==buffer[j]){
				++scount1;
			}
		
		}	
//		printf("Number of spaces:%d\n",scount1);
		
		if(scount1>1){
			printf("Invalid command, please input one of the following commands:\n No spaces in front of first command,if command has two parts, only one space in front of first command and no spaces after second parameter\n create\n serve\n withdraw\n deposit\n query\n end\n quit\n\n\n");
			continue;
		}

		else{
			token1 = strtok_r(rest," ",&rest);
			if(scount1==1){
				token2= strtok_r(rest,"\n",&rest);
				if(token2==NULL){
					printf("Invalid command, please input one of the following commands:\n No spaces in front of first command,if command has two parts, only one space in front of first command and no spaces after second parameter\n create\n serve\n withdraw\n deposit\n query\n end\n quit\n\n\n");
					continue;	
				}
//				printf("token2 is: %s\n",token2);
			}
		}

//		printf("first token is: %s\n",token1);



//check spaces in buffer again ok good
//


		if(strcmp(token1,"quit\n")==0){
			go =0;
			quit=1;
//			shutdown=1;
			sleep(2);
			send(sockfd,buffer,strlen(buffer),0);
			continue;

		}else if(strcmp(token1,"query\n")==0){	
			sleep(2);
			send(sockfd,buffer,strlen(buffer),0);
			continue;

		}else if(strcmp(token1,"end\n")==0){
			sleep(2);
			send(sockfd,buffer,strlen(buffer),0);	
			continue;

		}else if(strcmp(token1,"withdraw")==0){
			int num=-1;
			int skip =0;
			if(atof(token2)<0){
				printf("%s",invalidMsg);
				continue;				
			}

			int m =0;
			int period=0;
			for(m=0;m<strlen(token2);m++){
				if(period>1){
					skip=1;
					break;
				}
				if(token2[m]=='.'){
					period++;
					continue;
				}			
				if(isNum(token2[m])==0){
					skip=1;
					break;
				}		
				
			}	
			

			if(skip == 1){
				continue;
			}
			sleep(2);
			send(sockfd,buffer,strlen(buffer),0);		
			continue;

		}else if(strcmp(token1,"create")==0){
			int m=0;
			int skip=0;
			for(m=0;m<strlen(token2);m++){
				if((isalpha(token2[m])==0)){
					skip=1;
					break;	
				}	
			}
			if(skip){
				continue;
			}
			sleep(2);
			send(sockfd,buffer,strlen(buffer),0);	
			continue;
			}else if(strcmp(token1,"finish\n")==0){
//				printf("got it\n");
				continue;
			
			}else if(strcmp(token1,"deposit")==0){
			int num=-1;
			int skip =0;
			if(atof(token2)<0){
				printf("%s",invalidMsg);	
				continue;				
			}

			int m =0;
			int period=0;
			for(m=0;m<strlen(token2);m++){
				if(period>1){
					printf("%s",invalidMsg);
					skip=1;
					break;
				}
				if(token2[m]=='.'){
					period++;
					continue;
				}			
				if(isNum(token2[m])==0){
					printf("%s",invalidMsg);
					skip=1;
					break;
				}		
				
			}	
			

			if(skip == 1){
				continue;
			}
			sleep(2);	
			send(sockfd,buffer,strlen(buffer),0);	
			continue;
		}else if(strcmp(token1,"serve")==0){
			int m=0;
			int skip=0;
			for(m=0;m<strlen(token2);m++){
				if(((isalpha(token2[m])==0))){
					skip=1;
					break;	
				}	
			}
			if(skip){
				continue;
			}

			sleep(2);
			send(sockfd,buffer,strlen(buffer),0);	
			continue;
		}else{	
		//command was no good
printf("%s",invalidMsg);	
		}
	}//end of while, want to reset token1 and check if command will have token 2
//printf("Need to be here if im going to exit both threads!\n");	
	quit=1; //dont know what this is for, maybe shutdown?
	return;	
}

void* serverRec(void* var){
	char recbuf[256];
//	shutdown=0;
	while(shutdwn==0){
		if(recv(sockfd,recbuf,255,0)<=0){
		//	printf("problem recieving from server\n");
			break;
		}else{
			if(strcmp(recbuf,"Server shutting down\n")==0||strcmp("Quit recieved: ending sessions and closing client\n",recbuf)==0){
				
				printf("msg from server: %s\n",recbuf);
				//fprintf(stdin,"finish\n");
				shutdwn=1;
			//	return;
				exit(1);
			}
			printf("msg from server: %s\n",recbuf);
		}

		bzero(recbuf,256);
	}	
	return;
}



int main(int argc, char* argv[]){

	//first checking cmd line args
	if(argc!=3){
		//fatal error
	}
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0){
		//errpr in opening sockett
		//do we have to try again or something until it wokrs?
	}
	
	server = gethostbyname(argv[1]);
	if(server==NULL){
		//error host doesnt exist--fatal error? 
	}
	
	//setting server struct
	serv_addr.sin_family=AF_INET;
	bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
	serv_addr.sin_port = htons(portno);

	//set timer values for connecting
	it_val.it_value.tv_sec=3;
	it_val.it_value.tv_usec=3;
	it_val.it_interval=it_val.it_value;
	setitimer(ITIMER_REAL,&it_val,NULL);
	signal(SIGALRM,connecthandler);
	signal(SIGINT,sigc);
	//connecting to server
	while(connectbool==0){
		//how do i wait 3 seconds
		//--we're supposed to wait 3 seconds bewteen every try
	}

	pthread_t tid[2];
	pthread_create(&tid[0],NULL,cmdread,NULL);
	pthread_create(&tid[1],NULL,serverRec,NULL);

	//join when the quit cmd is called
	
	pthread_join(tid[0],NULL);
	pthread_join(tid[1],NULL);

/*
	scanf("%s\n",buffer);
	while(strcmp(buffer,"quit")!=0){
		write(sockfd, buffer,strlen(buffer));
		read(sockfd, buffer, 200);
		printf("message from server: %s",buffer);
		scanf("%s\n",buffer);
	}
*/
	return 0;
}


















