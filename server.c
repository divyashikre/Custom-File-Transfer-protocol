#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h> 
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <aio.h>
#include <errno.h>

#define UDP_PACKET_SIZE 1500
#define FILE_DATA 1476
#define SEQ_SIZE 1480

int LAST_SEQ ; 
int last_rec_seq_num; 

struct timeval stop, start;

int file_size;
int nack_end_flag =0;

typedef struct 
{
	int nack_arr[370];
}NACK_PKT;



int s;
int* ACK_lost;
char* file_data;
struct sockaddr_in destination;
struct sockaddr_in source;
pthread_t NACK_THREAD;


void* nack_thread(void *arg) {
	FILE *fd;
	int nack_start_index=1;
	int nack_end_index;
	
	NACK_PKT *lost_packet_indexes = (NACK_PKT* )malloc(sizeof(NACK_PKT));
	
	char nack_buffer[UDP_PACKET_SIZE];
  
  	struct iphdr *ip = (struct iphdr *)nack_buffer;
  	ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(1500);     
    ip->frag_off = 0;             
    ip->ttl = 64;                 
    ip->protocol = IPPROTO_RAW;     
    ip->check = 0;                  
    ip->saddr = source.sin_addr.s_addr;
    ip->daddr = destination.sin_addr.s_addr;

    int checked_sequence=1;
    int temp_nack_start_index = nack_start_index;
    int temp_prev_start_index = temp_nack_start_index;
    int temp_last_seq_rec = last_rec_seq_num;
    int flag=0,copysize;
    fd = fopen("output3","w+");
    int last_pkt_size = file_size - (LAST_SEQ -1)*FILE_DATA;
    //printf("last_pkt_size = %d\n",last_pkt_size);
	while(nack_start_index!=LAST_SEQ){
		int i=0;

              if(checked_sequence != temp_prev_start_index || checked_sequence == LAST_SEQ){
                 
		//printf("checked sequence = %d and temp prev=%d\n",checked_sequence, temp_prev_start_index);
            if(checked_sequence == LAST_SEQ){
                   copysize = (checked_sequence - temp_prev_start_index )*1476+last_pkt_size;
            }
            else{
                   copysize = (checked_sequence - temp_prev_start_index)*1476;
            }
            // printf("copy size = %d\n",copysize);
		
                   fwrite(file_data+(temp_prev_start_index-1)*1476,copysize,1,fd);
		//	printf("%p and %p \n",file_data,file_data+(checked_sequence-temp_prev_start_index-1)*1476);
            temp_prev_start_index = checked_sequence;
    
          } 

		while(temp_nack_start_index<=temp_last_seq_rec){
			if(ACK_lost[temp_nack_start_index]==1){
				if(i==0)
					checked_sequence = temp_nack_start_index;
				temp_nack_start_index++;
			}
			else if (ACK_lost[temp_nack_start_index]==0){
				
				if(i==370)
					break;
				lost_packet_indexes->nack_arr[i] =  temp_nack_start_index;
				temp_nack_start_index++;
				i++;
			}
		}

	memset(nack_buffer+20,0,sizeof(nack_buffer)-20);
       	memcpy(nack_buffer+20,lost_packet_indexes,sizeof(NACK_PKT));
       	if(i>0)
        	sendto(s, (char *)nack_buffer, (i*4)+20, 0,(struct sockaddr *)&destination, (socklen_t)sizeof(destination));
        usleep(75000);
        memset(lost_packet_indexes,0,sizeof(lost_packet_indexes));
		nack_start_index = checked_sequence;
              if(last_rec_seq_num > LAST_SEQ)
		temp_last_seq_rec=last_rec_seq_num-1;
	      else
		temp_last_seq_rec=last_rec_seq_num;

		temp_nack_start_index=nack_start_index;

	}
              if(checked_sequence != temp_prev_start_index || checked_sequence == LAST_SEQ){
                 
	
            if(checked_sequence == LAST_SEQ){
                   copysize = (checked_sequence - temp_prev_start_index )*1476+last_pkt_size;
            }
            else{
                   copysize = (checked_sequence - temp_prev_start_index)*1476;
            }
                
 
                   fwrite(file_data+(temp_prev_start_index-1)*1476,copysize,1,fd); 
			//printf("%p and %p \n",file_data,file_data+(temp_prev_start_index-1)*1476);
}
                //   fwrite(file_data+(checked_sequence-temp_prev_start_index-1)*1476,copysize,1,fd);
	//nack_end_flag=1;
	memset(lost_packet_indexes,0,sizeof(lost_packet_indexes));
	lost_packet_indexes->nack_arr[0] = 2147483646;
	//printf("lost_packet_indexes->nack_arr[0] = %d\n", lost_packet_indexes->nack_arr[0]);
	memset(nack_buffer+20,0,sizeof(nack_buffer)-20);
	int k;

  	memcpy(nack_buffer+20,lost_packet_indexes,sizeof(NACK_PKT));

    int i;
    for(i=0;i<15;i++){

    	sendto(s, (char *)nack_buffer, 24, 0,(struct sockaddr *)&destination, (socklen_t)sizeof(destination));
    }
	nack_end_flag=1;

    return ;
}


int main(){
	char recv_packt[UDP_PACKET_SIZE];
	if((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0){
		perror("Error opening socket");
		exit(0);
	}
	int sockbuf = 1610612736;
	setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&sockbuf, sizeof(sockbuf));
	
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&sockbuf, sizeof(sockbuf));
	
	source.sin_family = AF_INET;
	source.sin_port = 0;
	source.sin_addr.s_addr = INADDR_ANY;
	int bytes_received;
	int current_seqno, *recv_seqno;
	int len = sizeof(destination);
	int calloc_flag = 0;
	while(1) {
		memset(recv_packt,0,sizeof(recv_packt));
	if(nack_end_flag==1){
			//printf("reached break\n");
						break;
		}
		bytes_received=recvfrom(s, (char *)&recv_packt,sizeof(recv_packt),0,(struct sockaddr *)&destination,&len);

               if(bytes_received == 20)
                         break;

		recv_seqno = ((int*)(recv_packt+20));
		current_seqno = *recv_seqno;
	
	if(nack_end_flag==1){
			//printf("reached break\n");
						break;
		}
		if(current_seqno > LAST_SEQ && current_seqno!=0 && nack_end_flag==1){
			//printf("reached break\n");
						break;
		}
		if(last_rec_seq_num < current_seqno)
			last_rec_seq_num = current_seqno;
		if(current_seqno == 0){
			gettimeofday(&start, NULL);
			if (calloc_flag ==0 ){
				
				file_size = atoi(recv_packt+24);
			
				if(atoi(recv_packt+24)%FILE_DATA ==0)
					LAST_SEQ = (atoi(recv_packt+24)/FILE_DATA);
				else
					LAST_SEQ = (atoi(recv_packt+24)/FILE_DATA + 1);
				ACK_lost = (int*)calloc(LAST_SEQ+1,sizeof(int));
				file_data = (char*)calloc(file_size,sizeof(char));
				pthread_create(&NACK_THREAD,0,&nack_thread,0);
				calloc_flag = 1;
			}

		}
		else if (current_seqno <= LAST_SEQ){
	if(current_seqno == 75000 || current_seqno == 200000 || current_seqno == 450000 || current_seqno == 650000)
	printf("Packet received with sequence number: %d\n",current_seqno);
			
			ACK_lost[current_seqno] = 1;
			memcpy(file_data+(current_seqno-1)*FILE_DATA,recv_packt+24,(bytes_received - 24));
		}
	}
	(void) pthread_join(NACK_THREAD, NULL);
	gettimeofday(&stop, NULL);
    long time_rec;
	time_rec = (stop.tv_sec-start.tv_sec)*1000000 + stop.tv_usec-start.tv_usec;
    printf("total time %ld\n",time_rec/1000000);


	return 0;

}


