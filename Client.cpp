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
#define SERVER_PORT 5000
#define CLIENT_PORT 7000
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
int count = 0;        
int NackCount =0;        
int block;  
int NACKFLAG =0;        
int ExpectedSeq =0 ;   
int Last_Seq = 1;        
int ServerSocket,ClientSocket;  
int Fail_Count=0;
Msg msg;
int PacketNumber = 0;
int Success_Count = 0;
int All_Sending_Count = 0;
int HandShake_Count=0 ;
static unsigned  putj;    
static WSADATA WsaData;
static WORD wVersionRequested;
static struct sockaddr_in Server_Addr;
static struct sockaddr_in Client_Addr;
static struct timeval Control_Timeout={0,300000};
struct fd_set fds; 
Msg RecvMsg;
Msg DirtyMsg;
Msg CopyMsg;
FILE* log;
int InRange = 0;  
int CurrentSeq = 0;
int WindowPt = 0;
int WindowN = 0;
int WindowSz = WINDOW_SIZE;
Msg MsgBuffer[WINDOW_SIZE];    

int  SelectReceive();
int NACKJudge( int x,  int y);
int ACKSend();
int NACKSend();
int JudgeWAck();
int WindowSend(Protocol protocol, Method method, Type type, char* data, int Retrans);
int ClientGetLocalInfor(Infor* Local_Infor);
int ResolveName(char* name);
int ServerSocketInitialization();
int ClientSocketInitialization(char* Server_Name);
int MsgSend(Protocol protocol, int seq, Method method, Type type, char* data, int OverAck);
int JudgeAck(int OverAck);
int File_Recv(char* filename);
int File_Send(char* filename);
void PutMode(int argc, char *argv[ ]);
void GetMode(int argc, char *argv[ ]);
int CheckUsage(int argc, char *argv[]);
void PrintUsage();  
Infor Local_Infor;
static struct timeval Window_Timeout={0,300000};
     int WINDOW_SIZE2 = WINDOW_SIZE;
        
          int CLEAN =0 ;
int CheckUsage(int argc, char *argv[]){
    
 /** Check the Method */  
/* This Method checks the number of arguments and handle error like File not found while Put*/
	
if(argc<4){
             PrintUsage();
return -1;
}
FILE* fp1 = fopen(argv[3],"rb+");
if(fp1 == NULL && (strcmp(argv[2],"Put")==0)){
         PrintUsage();
       //  fclose(fp1);
return -1;
}



if((strcmp(argv[2],"Put")==0)||(strcmp(argv[2],"Get")==0)&&(argc==4))
  return 1;
else 
  {
    fprintf(stderr,"Please Following the Usage:    Too Few Argruments\n");
  PrintUsage();  
  return -1;
  }     
   
}

/*This method Print the warning if arguments or file not found*/
void PrintUsage(){
        fprintf(stderr,"----------------------------------------------------------------------\n");
       fprintf(stderr,"Usage:UdpTransfer [-Server_Hostname] [-Method] [-FileName] \n");
       fprintf(stderr,"----------------------------------------------------------------------\n");
       fprintf(stderr,"[-Server_Hostname:          <Server's Hostname>\n");
       fprintf(stderr,"[-Method]  include:          -Get: To download file from the  server\n");
       fprintf(stderr,"                             -Put: To upload file to the  server\n\n");
       fprintf(stderr,"[-File_Name]:                <Name of the file to be processed>\n");
       fprintf(stderr,"----------------------------------------------------------------------\n");
       fprintf(stderr,"Example : UdpTransfer sun Get 1.jpg                    \n");
       fprintf(stderr,"Example : UdpTransfer sun Put 2.jpg       \n");
       fprintf(stderr,"----------------------------------------------------------------------\n");
       
}
     

int main(int argc, char *argv[])
{
    
    Method method;
    /** Check the Command Usage  */  
	if(CheckUsage(argc, argv) == -1)
    return  -1;
    
	 /** Check the Command of Method  */ 
    if(strcmp(argv[2],"Put")==0) method = Put;
    else method = Get;

	 /** Create Two Sockets */
    ServerSocket = ServerSocketInitialization();
    ClientSocket = ClientSocketInitialization(argv[1]);
     unsigned j;
     int Remote_Seq;
     int Local_Seq;

	 /** Get local random number */
    srand( (unsigned)time( NULL ) ); 
    j=rand()%256;
    printf( "%d %d\n", j,(j<<31)>>31 ); 
    Local_Seq = (j<<31)>>31;
         
    
     /** Create Log File */
     log = fopen("UDP_CLient_Log.txt","wb+");
     char Host_Name [100];
     memset(Host_Name,0,100);
     gethostname(Host_Name  , 30 );
     fprintf(log,"Client: starting on host %s \r\n",Host_Name);

     /** Start the  three-way handshake */
	 MsgSend(SYN,j,Null,Blank,NULL,0);     
     sscanf(RecvMsg.data,"%d",&Remote_Seq);


 putj = Remote_Seq ;
    	putj = (putj<<31)>>31;
	/** Send the ACK for 3-way handshake */
    MsgSend(ACK,Remote_Seq,Null,Blank,NULL,0);
    
   
    printf("!!!\n");

	
    /** Get local basic information*/
    memset(&Local_Infor,0,sizeof(Local_Infor));   
    strcpy(Local_Infor.File_Name,argv[3]);
    ClientGetLocalInfor(&Local_Infor);
    printf("Local Infor %s  %s  %s \n", Local_Infor.User_Name, Local_Infor.Machine_Name,Local_Infor.File_Name);
    fflush(stdout);
    
    MsgSend(ACK,Last_Seq,(Method)method,Blank,(char*)&Local_Infor,0);//Ack Infor Request
    
    printf("Method %d\n" ,method);
	 /** Get Mehod */
    if(msg.method == 1)
    {   WindowN = putj ;
      ExpectedSeq=  putj ;   
        MsgSend(ACK,Last_Seq,Get,Blank,NULL,0);//Ack Get Start
        File_Recv(Local_Infor.File_Name);// Receving the file;
          
         /** chage the waiting time of last packet */ 
		Control_Timeout.tv_sec = 3;
        Control_Timeout.tv_usec = 0;
    
	MsgSend(ACK,Last_Seq,Get,Blank,NULL,1);//This is the last packet

	 /** Log the drop rate */
    fprintf(log,"Recv Success Count %d    Transmit Count %d  Drop Rate %d %s\r\n",Success_Count, All_Sending_Count, (All_Sending_Count-Success_Count)*100/All_Sending_Count,"%");
    fclose(log);
    printf("Timeout\n");
    fflush(stdout);
    }
    else /** Put Mehod */
    {
            WindowN  =(j<<31)>>31;
          CurrentSeq = (j<<31)>>31;
       File_Send(Local_Infor.File_Name);
	   /** Log the drop rate */
       fprintf(log,"Recv Success Count %d    Transmit Count %d  Drop Rate %d %s\r\n",Success_Count, All_Sending_Count, (All_Sending_Count-Success_Count)*100/All_Sending_Count,"%");
       fclose(log);
    }
  
    getchar();
}

//Akash Changes

int File_Send(char* filename)
{

	/** Open the wanted file */ 
     FILE* fp = fopen(filename,"rb+");
   
     long Fsize;
     fseek(fp, 0, SEEK_END);
     Fsize = ftell(fp); /** Get the file size*/  
     rewind(fp);
     char buf[100];

     int Last_Data_Size;
     printf(" Fsize %d \n", Fsize);

	 /** Operation if the file size less than 1KB*/  
     if(Fsize<=1024){ 
             sprintf(buf,"%d %d",1,Fsize); // Block is 1
             printf(" BUF %s \n", buf);
             MsgSend(SYN,Last_Seq,Put,Data, buf,0);    //ACK for the GEt START;
                  
             fread(msg.data,Fsize,0,fp);
             printf(" DATA %s \n",msg.data);
             MsgSend(SYN,Last_Seq,Put,Data,NULL,0); // Send the File Data to remote
             
             PacketNumber++; //Packet Count
           
             }

	 /** Operation if the file size greater than 1KB*/    
     else 
	 {
          block = Fsize/1024;
          Last_Data_Size = Fsize-block*1024;
          printf("IN FILE SEND\n");
          memset(buf,0,sizeof(buf));
          sprintf(buf,"%d %d",block,Last_Data_Size);  // Calculate the Block number and last size
          printf(" BUF %s \n", buf);
           
          MsgSend(SYN,Last_Seq,Put,Data, buf,0);  //ACK for the GEt START;         
    
          printf(" WindowN  %d Current Seq %d\n", WindowN,CurrentSeq );
         /*
            FD_ZERO(&fds); 
             FD_SET(ServerSocket,&fds); 
             while(1){
                                   switch(select(ServerSocket+1,&fds,NULL,NULL,&Window_Timeout))
                                   {
                                    case 0:  {CLEAN =1 ;  break;}
                                    default: {recvfrom(ServerSocket, (char*)&DirtyMsg, MAX_LENGTH, 0, NULL, NULL); 
                                    
                                    printf("Dirty: received an ACK for packet %d  %d  \r\n",DirtyMsg.seq,RecvMsg.protocol);
                       // getchar();
                                    break;}
                                    }
                                    if(CLEAN !=0)
                                    break;
                                 }   
          CLEAN =0;
          */
           getchar();
           do
		   {
              fread(msg.data,1024,1,fp);
             //  printf("READ DAta %s\n", msg.data);
             memset(&RecvMsg,0,1040);
           //  printf("STATINLOOP\n"); 
           // MsgSend(SYN,Last_Seq,Put,Data,NULL,0); // Send the Data Block
           //  getchar();
              WindowSend(SYN, Put, Data , msg.data, 0);
             block--;  // decrementing Block size
              printf("1Block -- %d  %d\n", block,CurrentSeq);
             PacketNumber++;
             memset(msg.data,0,1024);
               //getchar();
             }while(block>0);
          
             if(Last_Data_Size!=0)   // Send the last packet
             {
				 printf("STATINLAST\n"); 
                 //printf(" I am HeRE!!!!\n");
                 fread(msg.data,Last_Data_Size,1,fp);
                 printf(" DATA %s \n",msg.data);
             // memset(&msg,0,sizeof(msg));
              WindowSend(SYN, Put, Data , msg.data, 0);
              //  MsgSend(SYN,Last_Seq,Put,Data,NULL,0);
            //getchar();
          printf("Last Seq is %d \n",msg.seq );
        //  getchar();
             WINDOW_SIZE2 = WindowPt;
               while(1){
              if(JudgeWAck()== 1)
              break;
              
              }
              
              PacketNumber++;          
                         
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
    int block,Last_Data_Size;

	 /** Get the block number and last packet size from remote */  
    sscanf(RecvMsg.data,"%d %d",&block, &Last_Data_Size);
    printf("Data %s BLOCk %d Last_Data_Size%d \n",RecvMsg.data,block, Last_Data_Size);
    fflush(stdout);
    MsgSend(ACK,Last_Seq,Get,Blank,NULL,1); //Ack for the File Infor  Message;
    printf(" WindowN  %d  Expected %d\n", WindowN ,ExpectedSeq);
    /** Create the receiving file */
	FILE* fp = fopen(filename,"wb+");
    
    /** Start receiving the file larger that 1KB*/  
    if(block > 0){
              do{ SelectReceive();
                    //printf(" DAta ; %s \n",RecvMsg.data);
                    fwrite(RecvMsg.data,1024,1,fp);
                 //   printf("INSIDE LOOP LAST SEQ %d I should ACK %d\n",Last_Seq,RecvMsg.seq);
                    block--;
                    //  MsgSend(ACK,Last_Seq,Get,Blank,NULL,0);  // Send the ack for data
                      printf("Block -- %d\n", block);
                      }while(block !=0);
                  //printf("Last Packet %s", RecvMsg.data); 
                  fflush(stdout);  
				   /** Save the last file*/ 
				   SelectReceive();
                 fwrite(RecvMsg.data,Last_Data_Size,1,fp);
                 fclose(fp);
                 return 1;
                 }
	  /** Start receiving the file less that 1KB*/  
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
   

int ClientGetLocalInfor(Infor* Local_Infor)
{
 unsigned long length =30;
 
 //Find the UserName of the client;    
 if (GetUserName(Local_Infor->User_Name  , &length)!=1){
    fprintf(stderr,"Get User Name Failed\n");
    return -1;
 }
//Find the hostname of the local machine; 
 if(gethostname(Local_Infor->Machine_Name,30)!=0){
    fprintf(stderr,"Get Machine Name Failed\n");                                            
    return -1;	
  // printf("Local Infor %s  %s  %s \n", Local_Infor->User_Name, Local_Infor->Machine_Name,Local_Infor->File_Name); 
 
 
 }


 fflush(stdout);
  return 1;
}
int MsgSend(Protocol protocol, int seq, Method method, Type type, char* data,int OverAck){
     /** Copy from parameter to buffer */ 
    if(data != NULL)// IF null means have data buffer 
    memset(&msg,0,sizeof(msg));
    msg.protocol = protocol;
    msg.seq = seq;
    msg.method = method;
    msg.type = type;
     if(data != NULL)
    memcpy(msg.data,data,DATA_LENGTH);
    memcpy(&CopyMsg, &msg, sizeof(msg)); // Save the msg for retransmission;
   
    do{
		/** Sending the packet  */  
    sendto(ClientSocket,(char*)&msg, MAX_LENGTH, 0, (struct sockaddr *)&Client_Addr, sizeof(Client_Addr));  
    All_Sending_Count++;
     //if(HandShake_Count >=2) // Do not log the handshake packet
    // fprintf(log,"Client: sending  an ACK for packet %d \r\n",msg.seq);
    //printf("Sending Last %d  Pro %d Seq %d  Type %d DAta ; %s  \n",Last_Seq,msg.protocol,msg.seq, msg.type,msg.data);
    }while(JudgeAck(OverAck)!=1);  //Need retransmission;      
   // printf("Sending Data %s  \n",msg.data);
}


int JudgeAck(int OverAck){
	 /** Add the socket to select() */                           
        FD_ZERO(&fds); 
        FD_SET(ServerSocket,&fds); 

        switch(select(ServerSocket+1,&fds,NULL,NULL,&Control_Timeout))   //select
        { 
            case -1: return -1; // Error
            case 0:    // Timeout 
                   /** Last packet exit directly*/   
                     if(OverAck ==1)
                     return 1;
                     Fail_Count++;
                     /**Timeout keep retransmission*/    
                     return 0;
            default:        
                            int m,n=0;       
                         /** Receive the remote packet */     
                        memset(&RecvMsg,0,MAX_LENGTH);
                        m = recvfrom(ServerSocket,(char*) &RecvMsg, MAX_LENGTH , 0, NULL, NULL);
                        
                   // printf("Recving Last %d  Pro %d Seq %d  Type %d DAta ; %s \n",Last_Seq,RecvMsg.protocol,RecvMsg.seq, RecvMsg.type,RecvMsg.data);
                        //  printf("HERE %d %d\n",WSAGetLastError(),m); 
                      // printf(" Last Seq is %d  %d \n" ,Last_Seq,RecvMsg.seq);
                        fflush(stdout);
                        if(Last_Seq != RecvMsg.seq) {
                                    Success_Count++;
                                    HandShake_Count++;
                        Last_Seq = RecvMsg.seq;
                       // if(HandShake_Count >=2)
                      //    fprintf(log,"Client: received an SYN for packet %d  \r\n",RecvMsg.seq);
                        }
                        else{
                             if(OverAck != 1)
                        MsgSend(CopyMsg.protocol, CopyMsg.seq, CopyMsg.method,CopyMsg.type, CopyMsg.data,0);
                        else 
                        MsgSend(CopyMsg.protocol, CopyMsg.seq, CopyMsg.method,CopyMsg.type, CopyMsg.data,1);
                        }
                        return 1;
 
          }// end switch 
     
}
 //select

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
  
