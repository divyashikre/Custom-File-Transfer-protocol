#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h> 
#include <fcntl.h>

#define UDP_PACKET_SIZE 1500
#define FILE_DATA 1476
#define SEQ_SIZE 1480



void* retrans_lost(void* arg);

int LAST_SEQ;
struct sockaddr_in source,destination;
int s;
char *file_data;
struct stat stat_buffer;
int file_size;

typedef struct {
   int seqno;
   char data[FILE_DATA];
}SEND_Pkt;

typedef struct{

  int nack_arr[370];

}NACK_PKT;

FILE *fdout;

int main (int argc, char *argv[])
{
        pthread_t RETRANS_THREAD;
       
        int packet_size=0;
        char send_packet[UDP_PACKET_SIZE];

 
        int fdin;
        if ((fdin = open (argv[2], O_RDONLY)) < 0)
                 perror("error opening file:");

         if (fstat (fdin,&stat_buffer) < 0)
                perror("fstat error");

        if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
                perror("error:");
                exit(0);
        }
       int sockbuf=1610612736;
        setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&sockbuf, sizeof(sockbuf));
        setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&sockbuf, sizeof(sockbuf));



         if ((file_data = mmap (0, stat_buffer.st_size, PROT_READ, MAP_SHARED, fdin, 0)) == (caddr_t) -1)
           perror("mmap error");


                LAST_SEQ = ((int)stat_buffer.st_size % FILE_DATA==0) ? ((int)stat_buffer.st_size / FILE_DATA):((int)stat_buffer.st_size / FILE_DATA +1);
               


       
        SEND_Pkt *App_Packet = (SEND_Pkt *)malloc(sizeof(SEND_Pkt));
        memset(App_Packet,0,sizeof(App_Packet));
        struct iphdr *ip = (struct iphdr *)send_packet;
        memset(send_packet,0, sizeof(send_packet));

           destination.sin_family = AF_INET;
           destination.sin_port = 0; 
     
       int len = sizeof(destination);

                inet_pton(AF_INET, argv[1], (struct in_addr *)&destination.sin_addr.s_addr);
            
                 source.sin_addr.s_addr = INADDR_ANY;

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




                int seq_num=0,i;
               


                        App_Packet->seqno = seq_num;
    
                        sprintf(App_Packet->data,"%ld",stat_buffer.st_size);
                        memcpy(send_packet+20,App_Packet,  sizeof(App_Packet->data) + sizeof(App_Packet->seqno));

                        for(i=0;i<15;i++){
                                sendto(s, (char *)send_packet, sizeof(send_packet), 0,(struct sockaddr *)&destination, (socklen_t)sizeof(destination));
                        }

             
              pthread_create(&RETRANS_THREAD, NULL, &retrans_lost, NULL);


                while(seq_num <= LAST_SEQ) {
                        seq_num++;

                        App_Packet->seqno = seq_num;
                        if(seq_num > LAST_SEQ){
                            memcpy(App_Packet->data,"last",sizeof("last"));
                            memcpy(send_packet+20,App_Packet,SEQ_SIZE);
                        }
                        if(seq_num < LAST_SEQ){
                                memcpy(App_Packet->data,file_data+(seq_num-1)*FILE_DATA,FILE_DATA);
                                memcpy(send_packet+20,App_Packet,SEQ_SIZE);
                                packet_size = UDP_PACKET_SIZE;

                        }
                        else if (seq_num == LAST_SEQ){
                            memset(App_Packet,0,sizeof(App_Packet));
                            App_Packet->seqno = seq_num;
                            memset(send_packet+20,0,sizeof(send_packet)-20);
                            int rem_pkt = stat_buffer.st_size - (LAST_SEQ -1)*FILE_DATA;
                            memcpy(App_Packet->data,file_data+(seq_num-1)*FILE_DATA,rem_pkt);
                            memcpy(send_packet+20,App_Packet, rem_pkt + 4);
                            packet_size = rem_pkt+24;
                           

                        }


                        sendto(s, (char *)send_packet, packet_size, 0,(struct sockaddr *)&destination, (socklen_t)sizeof(destination));

                }

        (void) pthread_join(RETRANS_THREAD, NULL);
        munmap(file_data,stat_buffer.st_size);
        return 1;
}

void set_ip(struct iphdr *ip){
  ip->ihl = 5;
  ip->tot_len = htons(1500);        
  ip->version = 4;
    ip->frag_off = 0;               
  ip->ttl = 64;                   
    ip->protocol = IPPROTO_RAW;     
  ip->tos = 0;
    ip->check = 0;                
    ip->saddr = source.sin_addr.s_addr;
    ip->daddr = destination.sin_addr.s_addr;
}

int last_packet(char packet[], SEND_Pkt *App_Packet, int seq_num){
  int packet_size;
  int rem_pkt = file_size - (LAST_SEQ -1)*FILE_DATA;;
  memset(packet+20,0,sizeof(packet)-20);
  memset(App_Packet+20,0,sizeof(App_Packet)-20);
  memcpy(App_Packet->data,file_data+(seq_num-1)*FILE_DATA,rem_pkt);
  memcpy(packet+20,App_Packet,rem_pkt+4);
  packet_size = rem_pkt+24;
  return packet_size;
}

void large_packet(char packet[], SEND_Pkt *App_Packet, int seq_num){
  memcpy(App_Packet->data,file_data+(seq_num-1)*FILE_DATA,FILE_DATA);
  memcpy(packet+20,App_Packet,SEQ_SIZE);  
}

void* retrans_lost(void *arg){

  char lost_pkt[UDP_PACKET_SIZE];
  char recv_packet[UDP_PACKET_SIZE];
  int i,n,seq_num,packet_size;
  NACK_PKT *pkt_nack;
  SEND_Pkt *lost = (SEND_Pkt *)malloc(sizeof(SEND_Pkt));
  struct iphdr *ip = (struct iphdr *)lost_pkt;
  memset(lost_pkt,0,sizeof(lost_pkt));
  memset(lost,0,sizeof(lost));
    set_ip(ip);
    int len = sizeof(destination);
  while(1) {
    memset(recv_packet,0,sizeof(recv_packet));
    n=recvfrom(s, (char *)&recv_packet, sizeof(recv_packet), 0,(struct sockaddr *)&destination, &len);
    pkt_nack = (NACK_PKT*)malloc(sizeof(recv_packet)-20);
    memcpy(pkt_nack,(recv_packet+20),sizeof(recv_packet)-20);
    printf("pkt_nack->nack_arr[0] = %d\n", pkt_nack->nack_arr[0]);
    if (pkt_nack->nack_arr[0] != 2147483646) {
      int temp=(n-20)/4;
      i = 0;
      while (i < temp && n>0 ) {
                seq_num = pkt_nack->nack_arr[i];
        packet_size = UDP_PACKET_SIZE;
        lost->seqno = seq_num;
        if(seq_num < LAST_SEQ){
          large_packet(lost_pkt, lost, seq_num);
                }
                else {
                int packet_size;
        packet_size = last_packet(lost_pkt, lost, seq_num);
                }
                sendto(s, (char *)lost_pkt, packet_size, 0,(struct sockaddr *)&destination, (socklen_t)sizeof(destination));
        i = i+1;
            }
        }
        else {
            memset(lost_pkt+20,0,sizeof(lost_pkt)-20);
            int i=0;
      while (i < 15) {
        sendto(s, (char *)lost_pkt,20, 0,(struct sockaddr *)&destination, (socklen_t)sizeof(destination));
        i = i+1;
      }
            break;
        }
    }
return;
}


