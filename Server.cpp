/** 
    For lab 3 Assignment
    Dan Zhang ID 6486592
    Akash Kanaujia ID 6560180
    
*/ 
#include <windows.h>
#include <winsock.h>
#include <time.h>
#include <stdio.h>
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include<fcntl.h>
#include <tchar.h>
#include <malloc.h>
#include <process.h>
#define SERVER_PORT 5001
#define CLIENT_PORT 7001
#define HEAD_LENGTH 16
#define DATA_LENGTH 1024
#define MAX_LENGTH 1040
#define WINDOW_SIZE 3
typedef enum {
     SYN ,
     SYNACK,
     ACK,
     NACK,
     OVER
     } Protocol;

int block;
typedef enum {
     Null,
     Get ,
     Put 
     } Method;     
     
typedef enum {
     Blank,
     Information ,
     Start,
     File_Not_Exist,
     Data,
     Connection_Over
     } Type;   
     
typedef struct {
       Protocol protocol ;
       int seq;
       Method method;
       Type type;
       char data[DATA_LENGTH];
       } Msg;
    
typedef struct {
        char User_Name[30];
        char Machine_Name[30];
        char File_Name[30];
        } Infor;
Msg MsgBuffer[WINDOW_SIZE]; 
Msg RecvMsg;
Msg msg;
Msg CopyMsg;
 int Local_Seq;
 int ReTransCount = 0 , TransCount = 0;
 int HandShake_Count=0 ;
int Last_Seq =-1;
static WSADATA WsaData;
static WORD wVersionRequested;
static struct sockaddr_in Server_Addr;
static struct timeval Control_Timeout={0,300000};
struct fd_set fds; 
static struct sockaddr_in Client_Addr;
int ServerSocket,ClientSocket;
FILE* log;
int WINDOW_SIZE2 = WINDOW_SIZE;
static struct timeval Window_Timeout={0,300000};
int WindowPt = 0;
int CLEAN =0 ;
Msg DirtyMsg;
int InRange = 0;  
int CurrentSeq = 0;
int NACKFLAG =0;
int ExpectedSeq = 0;
int count = 0;
int WindowN = 0;
int  SelectReceive();
int NACKJudge( int x,  int y);
int ACKSend();
int NACKSend();
int  File_Send(FILE* fp);
FILE* Check_Exist_File(char* filename);
int ResolveName(char* name);
int ServerSocketInitialization();
int ClientSocketInitialization(char* Server_Name);
int JudgeAck(int OverAck);
int MsgSend(Protocol protocol, int seq, Method method, Type type,  char* data,int OverAck);
int WindowSend(Protocol protocol, Method method, Type type, char* data, int Retrans);
int JudgeWAck();
int P_ACK = 0;
int Success_Count = 0;
int Fail_Count = 0;
int File_Recv(char* filename);
 int NackCount =0;
int PacketNumber = 0;
int main(int argc, char *argv[])
{
   
    char buf[1024];
	 /** Create Two Sockets */
    ServerSocket = ServerSocketInitialization();
    ClientSocket = ClientSocketInitialization(argv[1]);
    Infor RemoteInfor;
    unsigned j;
    unsigned putj;
    char logbuf [100];
      /** Create Log File */
    log = fopen("UDPLog_Server.txt","wb+");
     /** Write Hostname to Log */  
	char Host_Name [100];
    memset(Host_Name,0,100);
    gethostname(Host_Name  , 30 );
    fprintf(log,"Server: starting on host %s \r\n",Host_Name);

    
    /** Zero the buffer */    
    memset(logbuf,0,sizeof(logbuf));

    /** Get local random number */
    srand( (unsigned)time( NULL ) ); 
    memset(&RecvMsg,0,sizeof(RecvMsg));
    j=rand()%256;
    memset(buf,0,1024);
    sprintf(buf,"%d",j);
    printf( "%d %d\n", j,(j<<31)>>31 ); 
     /** Wating until being connected 3-way HandShake Started */
    recvfrom(ServerSocket, (char*)&RecvMsg, MAX_LENGTH, 0, NULL, NULL);
    HandShake_Count++;
    // fprintf(log,"Server: received an SYN for packet %d  \r\n",RecvMsg.seq);
    Last_Seq = RecvMsg.seq;// Assinging value of first receive sequence number
    putj = Last_Seq ;
    	putj = (putj<<31)>>31;
    
    printf("Pro %d Seq %d\n", RecvMsg.protocol,RecvMsg.seq);

	// Send the Three-way handshake SYNACK back;
    MsgSend(SYNACK,RecvMsg.seq,Null,Blank,buf,0);
    //recvfrom(ServerSocket, (char*)&RecvMsg, MAX_LENGTH, 0, NULL, NULL);
    /*printf("Pro %d Seq %d\n", RecvMsg.protocol,RecvMsg.seq);
    printf(" Fail Count %d Success Count %d \n",Fail_Count,Success_Count);*/
    
    /** Get the Local initial seq  */
	Local_Seq = (j<<31)>>31;
	
     
      
    /** Send the SYN INformation Request */
    MsgSend(SYN,Local_Seq,Null,Information,NULL,0);    
    Local_Seq++;

    /** Get the Remote Information and save */
    memcpy(&RemoteInfor,&RecvMsg.data[0],DATA_LENGTH);
   
    // It is Get method;
	if(RecvMsg.method == 1)
   {
         /** The Requested File does not exist */         
    if(Check_Exist_File(RemoteInfor.File_Name) == NULL){
                        printf("I can't find it\n");
                        MsgSend(SYN,Local_Seq,Get,File_Not_Exist,NULL,0);
                        Local_Seq++;                       
                         sprintf(logbuf,"Recv Success Count %d    Transmit Count %d  Drop Rate %d %s\r\n",Success_Count, TransCount, (TransCount-Success_Count)*100/TransCount,"%");
                        fwrite(logbuf,sizeof(logbuf),1,log);
                        fclose(log);
                        printf("Connection Over\n");
                        getchar();
                        return 1;
                        }
   /** The Requested File exists*/ 
	else {
                   WindowN  =(j<<31)>>31;
          CurrentSeq =(j<<31)>>31;
         printf("I can find it\n");
         
         MsgSend(SYN,Local_Seq,Get,Start,NULL,0);
        
         Local_Seq++;
         File_Send(Check_Exist_File(RemoteInfor.File_Name));
         }
          
           
    }
    /** Client Using Put Method*/ 
    else
    {        
          WindowN = putj ;
      ExpectedSeq=  putj ;   
    File_Recv(RemoteInfor.File_Name);// Receive the file;

	/** Sending the last ACK with 3s timeout*/
    Control_Timeout.tv_sec = 3;
    Control_Timeout.tv_usec = 0;
    MsgSend(ACK,Local_Seq,Put,Blank,NULL,1);// Last Packet info and TimeOut
    printf("Timeout\n");
    fflush(stdout);
        
    }
    
	/** Log the rate */ 
	sprintf(logbuf,"Recv Success Count %d    Transmit Count %d  Drop Rate %d %s\r\n",Success_Count, TransCount, (TransCount-Success_Count)*100/TransCount,"%");
	fwrite(logbuf,sizeof(logbuf),1,log);
	fclose(log);
   
    printf("Recv Count %d\n",Success_Count);
    //printf("Method %d %s  %s  %s \n", RecvMsg.method, RemoteInfor.User_Name, RemoteInfor.Machine_Name,RemoteInfor.File_Name);
    getchar();
}

FILE* Check_Exist_File(char* filename){
    return fopen(filename,"rb+")  ;
}

int  File_Send(FILE* fp){
     
     long Fsize;
     fseek(fp, 0, SEEK_END);
     Fsize = ftell(fp);  /** Get the File Size */  
     rewind(fp);
     char buf[100];

     int Last_Data_Size;
     
	   /** If the File Size is less than 1KB*/  
     if(Fsize<=1024){ 
		      /** Sending the infor about block and last packet number */  
             sprintf(buf,"%d %d",1,Fsize);
             printf(" BUF %s \n", buf);
             MsgSend(SYN,Local_Seq,Get,Data, buf,0);   
             Local_Seq++; 

			 /** Read the data from disk to buffer */   
             fread(msg.data,Fsize,0,fp);
             printf(" DATA %s \n",msg.data);

			 /** Sending the file data */  
             MsgSend(SYN,Local_Seq,Get,Data,NULL,0);
             Local_Seq++;
             PacketNumber++;
         
             }
	   /** If the File Size is larger than 1KB*/  
     else{

		 /** Calculate the infor about block and last packet number */  
          block = Fsize/1024;
          Last_Data_Size = Fsize-block*1024;
          memset(buf,0,sizeof(buf));
           sprintf(buf,"%d %d",block,Last_Data_Size);
             printf(" BUF %s \n", buf);
            /** Sending the infor about block and last packet number */  
             MsgSend(SYN,Local_Seq,Get,Data, buf,0);            
             Local_Seq++;     
           printf(" WindowN  %d Current Seq %d\n", WindowN,CurrentSeq );
          getchar();
			 /** keep sending the data until the last packet */    
			 do{
                           fread(msg.data,1024,1,fp);
           //  printf("STATINLOOP\n"); 
            WindowSend(SYN, Get, Data , msg.data, 0);
           //  getchar();
             Local_Seq++;
             block--;
             PacketNumber++;
             }while(block>0);
             
			  /** Sending the last packet */    
             if(Last_Data_Size!=0)
             {printf("STATINLAST\n"); 
                                   printf(" I am HeRE!!!!\n");
                                   fread(msg.data,Last_Data_Size,1,fp);
            printf(" DATA %s \n",msg.data);
             // memset(&msg,0,sizeof(msg));
            WindowSend(SYN, Get, Data , msg.data, 0);
          //   getchar();
             PacketNumber++;
            
             Local_Seq++;
             
             }
                while(1){
              if(JudgeWAck()== 1)
              break;
              
              }
     }            
             
                                      
     printf("Reading file\n");    
     
     return 1;
     }




int WindowSend(Protocol protocol, Method method, Type type, char* data, int Retrans){
  //printf("RE %d \n", Retrans);
   if(!Retrans)
   {
   //memset(&msg.data,0,1024);            
   msg.protocol = protocol;   
   msg.method = method;
   msg.type = type;
   msg.seq = CurrentSeq;

   memcpy(msg.data,data,DATA_LENGTH);
   
   memcpy(&MsgBuffer[WindowPt], &msg, sizeof(msg));// Save the msg for retransmission;
  // getchar();
   sendto(ClientSocket,(char*)&msg, MAX_LENGTH, 0, (struct sockaddr *)&Client_Addr, sizeof(Client_Addr));  
   fprintf(log,"Client: sending  an SYN for packet %d \r\n",msg.seq);
  // printf("Send Seq %d  DAta %s\n",msg.seq, msg.data);
   printf("Send Seq %d    N %d WindowPt %d N is %d\n",msg.seq, WindowN,WindowPt,WindowN);     
   WindowPt++;
   
      if(   CurrentSeq == WINDOW_SIZE)
         CurrentSeq = 0;         
         else
               CurrentSeq++;
   
   if   (WindowPt == WINDOW_SIZE  )
    {
      
     while(1){
              if(JudgeWAck()== 1)
              break;
              
              }
             
    // WindowSend(protocol, method,  type,  data, Retrans);
     
     }            
  
   
     }
   
 

else
{
    //printf("RE\n");
for(int i = 0; i< WINDOW_SIZE2; i ++)
{
         memcpy(&msg,&MsgBuffer[i],  sizeof(msg));
         sendto(ClientSocket,(char*)&msg, MAX_LENGTH, 0, (struct sockaddr *)&Client_Addr, sizeof(Client_Addr));  
         fprintf(log,"Client: sending Re an SYN for packet %d \r\n",msg.seq);
         printf("Re  Send Seq %d  R i %d  N is %d block %d\n",msg.seq, i,WindowN,block);  
         }
          fprintf(log,"          ************       \n\r\n");
        printf("          ************       \n"); 
   //if(    MsgBuffer[0].seq != WindowN)
  /// getchar();
    // JudgeWAck();
    //WindowPt=0;   
        
}      
    
        
}


int JudgeWAck()
   
   { int distance = 0;
   
             FD_ZERO(&fds); 
             FD_SET(ServerSocket,&fds); 
            
               switch(select(ServerSocket+1,&fds,NULL,NULL,&Window_Timeout))   //select 
        { 
               case -1: return -1;  // Error
               case 0:   
                       printf("TIMEOUT\n");
                     WindowSend(ACK,Get,Start,NULL,1);
                
                 /** Last packet exit directly*/
                 /**Timeout keep retransmission*/   
                  //  Fail_Count++;
                     return 0;
               default:   
                 
                          /** Receive the remote packet */   
                          memset(&RecvMsg,0,1040);          
                        recvfrom(ServerSocket, (char*)&RecvMsg, MAX_LENGTH, 0, NULL, NULL);
                      //  printf("Client: received an ACK for packet %d  %d  \r\n",RecvMsg.seq,RecvMsg.protocol);
                        //getchar();
                       
                        int n =1;
                    //    printf("Client: received an SYN for packet %d  %d N %d  \r\n",RecvMsg.seq,RecvMsg.protocol,WindowN);
                        fflush(stdout);
                        while(1){
                                   switch(select(ServerSocket+1,&fds,NULL,NULL,&Window_Timeout))
                                   {
                                    case 0:  {CLEAN =1 ;  break;}
                                    default: {recvfrom(ServerSocket, (char*)&DirtyMsg, MAX_LENGTH, 0, NULL, NULL); 
                                    
                                 // printf("Dirty: received an ACK for packet %d  %d  \r\n",DirtyMsg.seq,RecvMsg.protocol);
                       // getchar();
                                    break;}
                                    }
                                    if(CLEAN !=0)
                                    break;
                                 }    
                       // 
                        if(RecvMsg.protocol == 3)// When NACK received 
                        { fprintf(log,"Client: received an NACK for packet %d  \r\n",RecvMsg.seq);
                                       printf("receiving NACK %d\n",RecvMsg.seq);
                                             InRange = WindowN  ;
                                             
                        for(int i=0; i<WINDOW_SIZE;i++)
                        {
                                 
                               
                       //  printf(" @@@@@@INRANGE %d\n",InRange);
                                 if(InRange == RecvMsg.seq)
                                { 
                                            //WindowPt = WindowN;
                                WindowSend(ACK,Get,Start,NULL,1);       
                                  return 0;
                                   }
                                   
                                   InRange++;
                                     if(InRange>WINDOW_SIZE)
                                 InRange=0;
                          }
                          printf("NACK not in range\n" );
                           for(int i=0; i<WINDOW_SIZE;i++)
                        {
                                 WindowN++;
                                 if(  WindowN == WINDOW_SIZE+1)
                                 WindowN=0;
                               //  printf(" N moing %d\n",WindowN);
                          }
                          WindowPt = 0;
                            printf("Although NACK %d N is %d  sending next block %d\n",RecvMsg.seq,WindowN, block);         
                             fprintf(log,"------------------------------------------ \r\n",RecvMsg.seq);    
                            return 1;        
                        }
                        else
                        {
                         if(RecvMsg.protocol == 2)// When ACK received
                         {  fprintf(log,"Client: received an ACK for packet %d  \r\n",RecvMsg.seq);
                             int expectedACK;
                             
                             expectedACK = WindowN;
                             for(int i = 0; i< WINDOW_SIZE-1; i++)
                          {
                                  expectedACK = expectedACK +1;
                                  if(expectedACK == WINDOW_SIZE +1 )
                                  {
                                             expectedACK = 0; 
                                             
                                  }
                                             
                            
                          }  
                             //  printf("PRO %d EXpectedACK %d  REcv %d N is %dsending next block %d\n",RecvMsg.protocol,expectedACK,RecvMsg.seq,WindowN, block);
                             
                             if(RecvMsg.seq == expectedACK)
                            {   
                                fprintf(log,"------------------------------------------ \r\n",RecvMsg.seq);
                             for(int i = 0; i< WINDOW_SIZE; i++)
                             {
                                  WindowN++;
                                  if(WindowN == WINDOW_SIZE + 1)
                                  {
                                             WindowN = 0; 
                                             
                                  }
                                             
                             WindowPt = 0;     
                             }  
                             printf("ACK %d N is %dsending next block %d\n",RecvMsg.seq,WindowN, block);        
                               return 1;
                             
                          }   
                        
                          else {
                              // printf("GET HERE\n");
                          return -1;          
                          }              
                           /// We need to handle OVER protocol here                                                                                                      
                                                  
                                             
                          }   
                            
                            
                         }
                       
                                 
        
        }
        
            
            }



     
int File_Recv(char* filename){
     int Last_Data_Size;

	 /** Sending the SYN Request with Start Command*/    
     MsgSend(SYN,Local_Seq,Get,Start,NULL,0);
     Local_Seq++;

	 /** Read the block number and last packet size number */  
     sscanf(RecvMsg.data,"%d %d",&block, &Last_Data_Size);
     printf("Data %s BLOCk %d Last_Data_Size%d \n",RecvMsg.data,block, Last_Data_Size);
     fflush(stdout);
    Control_Timeout.tv_sec = 3;
    Control_Timeout.tv_usec = 0;
    printf(" WindowN  %d  Expected %d\n", WindowN ,ExpectedSeq);
     MsgSend(ACK,Local_Seq,Get,Blank,NULL,1); //Ack for the File Infor  Message;
     
    // WindowN = Local_Seq+1;
     // ExpectedSeq=  Local_Seq+1;
     // count++;
      printf("Recv Seq %d  EXpected %d NACKFLAG %d NACKCOUNT %d  DATA %s\n",RecvMsg.seq,ExpectedSeq,NACKFLAG,NackCount,RecvMsg.data);  
    
   // getchar();
	 /** Open the wanted file*/
	 FILE* fp = fopen(filename,"wb+");
    
    /** If the File Size is larger than 1KB*/ 
    if(block > 0){
              do{
                     /** Saving the data to local disk*/  
                      
                    
                
                      SelectReceive();
                      fwrite(RecvMsg.data,1024,1,fp);
                            block--;
                      printf("INSIDE LOOP   %d block %d\n", RecvMsg.seq,block);
                      Local_Seq++;
                      //printf("Block -- %d\n", block);
                      }while(block !=0);

			      /** Saving the last packet data to local disk*/ 
                 /* printf("Last Packet %s", RecvMsg.data); 
                  fflush(stdout);  */
                 fwrite(RecvMsg.data,Last_Data_Size,1,fp);
                 fclose(fp);
                 return 1;
                 }
	 /** If the File Size is less than 1KB*/ 
   else      {
                  fwrite(RecvMsg.data,Last_Data_Size,1,fp);
				  fclose(fp);
				  return 1;
              }    
}


int  SelectReceive(){
    
    int n;
    int flag =0;
   
                       while(flag == 0)
                       {  
                        
                        n  = recvfrom(ServerSocket, (char*)&RecvMsg, MAX_LENGTH, 0, NULL, NULL);
                  
                         printf("Recv Seq %d  EXpected %d NACKFLAG %d NACKCOUNT %d\n",RecvMsg.seq,ExpectedSeq,NACKFLAG,NackCount);     
                          NackCount ++;
                          if(NackCount   == WINDOW_SIZE+1)
                          NackCount  =0;
                        //getchar();
                       
                        if(RecvMsg.seq == ExpectedSeq){
                        // printf("Right Recv Seq %d  Recv data  %s Expected %d\n",RecvMsg.seq,RecvMsg.data,ExpectedSeq);  
                          fprintf(log,"Server: received an SYN for packet %d  \r\n",RecvMsg.seq);
                         //memset(&RecvMsg,0,MAX_LENGTH);           
                        ExpectedSeq++;
                        count++;
                        //printf("COUnt Is %d WINDOWSIZE is %d\n",count,WINDOW_SIZE);
                         if(count == WINDOW_SIZE){
                                    
                        for(int i=0; i<WINDOW_SIZE;i++)
                        {
                                 WindowN++;
                                 if(  WindowN == WINDOW_SIZE+1)
                                 WindowN=0;
                          }
                                                        ACKSend();
                                                         // fprintf(log,"------------------------------------------ \r\n",RecvMsg.seq);    
                                                       // getchar();
                                                        count=0;
                                                                   }
                         if(ExpectedSeq == WINDOW_SIZE+1){
            
                                             ExpectedSeq = 0;
                                                          }
                                                          flag = 1;
                        }
                        else {
                             if(NACKJudge(RecvMsg.seq, ExpectedSeq) >0)
                             {NACKFLAG = 1;
                                                     //  printf("NEED %d NACK %d \n",NACKFLAG,ExpectedSeq);
                              
                              }
                               if((  NackCount   == WINDOW_SIZE )&& (NACKFLAG ==1) ) {//printf("SENDING\n"); 
                               NACKSend(); NACKFLAG = 0;  NackCount =0;}
                              }
                        
                        
                      //  printf(" %d" ,NACKJudge(RecvMsg.seq, ExpectedSeq));
                        
                        
                       }
                        flag = 0;
                        n=0;
                        
                        return 1;

}
int NACKJudge( int x,  int y)
{
    int xLocation;
    int yLocation;
    int InRange;
    
     InRange = WindowN  ;
       for(int i=0; i<WINDOW_SIZE+1;i++)
                        {
                               // printf( "INRANGE %d \n",InRange);
                                 
                                 if(InRange==WINDOW_SIZE)
                                 InRange=0;
                                 else
                                 InRange++;
                                  
                          }
   // 
    if(InRange == y)
                                    return 1;
    
    
     InRange = WindowN  ;
    for(int i=0; i<WINDOW_SIZE+1;i++)
                        {
                                 
                                
                                 if(InRange == x)
                                xLocation = i;
                                   if(InRange == y)
                                yLocation = i;
                                InRange++;
                                 if(InRange>WINDOW_SIZE)
                                 InRange=0;
                                
                          }
                //    printf(" Y is %d YLOCATION %d  Xis %d XLOCATION %d N is %d NACK %d \n",y,yLocation,x,xLocation,WindowN,xLocation-yLocation);
  return xLocation-yLocation ;
}
    
    






int ACKSend()
{
      int expectedACK;
                             
                             expectedACK = WindowN;
                             for(int i = 0; i< WINDOW_SIZE; i++)
                          {
                                  expectedACK = expectedACK +1;
                                  if(expectedACK == WINDOW_SIZE +1 )
                                  {
                                             expectedACK = 0; 
                                             
                                  }
                                             
                            
                          }  
    
    
   msg.protocol = ACK;   
   msg.method = RecvMsg.method;
   msg.type = RecvMsg.type;
   msg.seq = RecvMsg.seq;
   sendto(ClientSocket,(char*)&msg, MAX_LENGTH, 0, (struct sockaddr *)&Client_Addr, sizeof(Client_Addr));  
    fprintf(log,"Server: sending  an ACK for packet %d \r\n",msg.seq);
      fprintf(log,"------------------------------------------ \r\n",RecvMsg.seq);    
  printf("Sending ACK %d  \n",msg.seq);
 // getchar();
   // getchar();
}

int NACKSend()
{
   msg.protocol = NACK;   
   msg.method = RecvMsg.method;
   msg.type = RecvMsg.type;
   msg.seq = ExpectedSeq;
  printf("Sending NACK  %d\n",ExpectedSeq);
   sendto(ClientSocket,(char*)&msg, MAX_LENGTH, 0, (struct sockaddr *)&Client_Addr, sizeof(Client_Addr));  
     fprintf(log,"Server: sending  an NACK for packet %d \r\n",msg.seq);
    // fprintf(log,"------------------------------------------ \r\n",RecvMsg.seq);    
}









int MsgSend(Protocol protocol, int seq, Method method, Type type, char* data, int OverAck){
    /** Copy from parameter to buffer */     
	if(data != NULL) // IF null means have data buffer 
    memset(&msg,0,sizeof(msg));    
    msg.protocol = protocol;
    msg.seq = seq;  
    msg.method = method;
    msg.type = type;
    if(data != NULL)
    {
           
    memcpy(&msg.data, data, DATA_LENGTH);
}
    memcpy(&CopyMsg, &msg, sizeof(msg));// Save the msg for retransmission;
                 
     do{      
     /** Sending the packet  */       
    sendto(ClientSocket,(char*)&msg, MAX_LENGTH, 0, (struct sockaddr *)&Client_Addr, sizeof(Client_Addr));  
   // if(HandShake_Count >=2)
   // fprintf(log,"Server: sending  an SYN for packet %d \r\n",msg.seq);
    TransCount++;    
    
    }while(JudgeAck(OverAck)!=1);  //Need retransmission;  
    
    memset(&msg,0,sizeof(msg));
}



int JudgeAck(int OverAck){
         /** Add the socket to select() */         
        FD_ZERO(&fds); 
        FD_SET(ServerSocket,&fds); 

        switch(select(ServerSocket+1,&fds,NULL,NULL,&Control_Timeout))   //select 
        { 
            case -1: return -1;  // Error
            case 0:   // Timeout 
                 /** Last packet exit directly*/    
                    if(OverAck == 1){
                               //printf("OVer ACK 1\n");
                     return 1;}
                      /**Timeout keep retransmission*/   
                    Fail_Count++;
                     return 0;
            default:    
                          /** Receive the remote packet */             
                        recvfrom(ServerSocket, (char*)&RecvMsg, MAX_LENGTH, 0, NULL, NULL);
                       
                       //printf("Recving Last %d  Pro %d Seq %d  Type %d DAta ; %s \n",Last_Seq,RecvMsg.protocol,RecvMsg.seq, RecvMsg.type,RecvMsg.data);
                        if(Last_Seq != RecvMsg.seq) {
                        Last_Seq = RecvMsg.seq;
                    //    printf(" In If Last Seq %d, Recv %d \n", Last_Seq,RecvMsg.seq);
                        Success_Count++;
                        HandShake_Count++;
                       // if(HandShake_Count >2)
                      //   fprintf(log,"Server: received an ACK for packet %d  \r\n",RecvMsg.seq);
                        }
                        else{
                              /** Do the retransmission */ 
                         if(OverAck != 1)
                        MsgSend(CopyMsg.protocol, CopyMsg.seq, CopyMsg.method,CopyMsg.type, CopyMsg.data,0);
                        else 
                        MsgSend(CopyMsg.protocol, CopyMsg.seq, CopyMsg.method,CopyMsg.type, CopyMsg.data,1);
                        ReTransCount++;
                        }
                        return 1;
          }// end switch 
     
}



int ServerSocketInitialization(){
    int Server_Sock;
    wVersionRequested = MAKEWORD( 2, 2 );
    
    if(WSAStartup(wVersionRequested,&WsaData) != 0){
          fprintf(stderr,"Error in starting WSAStartup()\n");
          return -1;
          }
              
    if ((Server_Sock = socket(AF_INET, SOCK_DGRAM,0)) < 0){
	      fprintf(stderr,"Can not create the SERVER socket,exit\n");
	      return -1;
       }
	                                                //Fill-in Server Port and Address info.
	memset(&Server_Addr, 0, sizeof(Server_Addr));   /* Zero out structure */
	Server_Addr.sin_family = AF_INET;                /* Internet address family */
	Server_Addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	Server_Addr.sin_port = htons(SERVER_PORT);      /* Local port */
	
	//Bind the server socket
    if (bind(Server_Sock, (struct sockaddr *) &Server_Addr, sizeof(Server_Addr)) < 0){
		fprintf(stderr,"Can not create the SERVER socket,exit\n");
	    return -1;
     }
	    

	return Server_Sock;
}
                                                           
int ClientSocketInitialization(char* Server_Name){
    int Server_Sock;

    

    if ((Server_Sock = socket(AF_INET, SOCK_DGRAM,0)) < 0){
	      fprintf(stderr,"Can not create the SERVER socket,exit\n");
	      return -1;
       }
       
    if(ResolveName(Server_Name)==-1){
    fprintf(stderr,"Can not Resolve the Server's Hostname \n");
    return -1;
    }   
   	memset(&Client_Addr, 0, sizeof(Client_Addr));     /* Zero out structure */
	Client_Addr.sin_family      = AF_INET;             /* Internet address family */
	Client_Addr.sin_addr.s_addr = ResolveName(Server_Name);   /* Server IP address */
	Client_Addr.sin_port        = htons(CLIENT_PORT); /* Server port */
	
	return Server_Sock;
}


int ResolveName(char* name)
{
	struct hostent *host;            /* Structure containing host information */
	
	if ((host = gethostbyname(name)) == NULL){
		fprintf(stderr,"Gethostbyname() failed\n");
        return -1;
        }
	/* Return the binary, network byte ordered address */
	return *((unsigned long *) host->h_addr_list[0]);
}                       
  
