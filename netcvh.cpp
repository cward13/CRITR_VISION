#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>
#include <pthread.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>



#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>	/* for fprintf */
#include <string.h>	/* for memcpy */

using namespace std;
using namespace cv;
                        
Mat     img;
int     is_data_ready = 0;
int	data_printed = 1; 
int     listenSock, connectSock,listenSock2;
int 	listenPort,listenPort2;


//This is the image buffer
vector<unsigned char> buff;


int BUFSIZE=32768;
int fd;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* streamServer(void* arg);
void* streamHost(void* arg);
void  quit(string msg, int retval);
  


int main(int argc, char** argv)
{
        pthread_t thread_s;
	pthread_t thread_t;
        int width, height, key;
	width = 1280;  
	height = 720; 
 	
	if (argc != 3) {
                quit("Usage: netcv_server <listen_port> <reply_port>", 0);
        }
	
	listenPort = atoi(argv[1]);
 	listenPort2 = atoi(argv[2]);
        img = Mat::zeros( height,width, CV_8UC1);
        
        /* run the streaming server as a separate thread */
        if (pthread_create(&thread_s, NULL, streamServer, NULL)) {
                quit("pthread_create failed.", 1);
        }


	if (pthread_create(&thread_t, NULL, streamHost, NULL)) {
                quit("pthread_create failed.", 1);
        }

        cout << "\n-->Press 'q' to quit." << endl;
        namedWindow("stream_server", CV_WINDOW_AUTOSIZE);

        while(key != 'q') {
                key = waitKey(10);
        }

        if (pthread_cancel(thread_s)) 
	{
                quit("pthread_cancel failed.", 1);
        }

        destroyWindow("stream_server");
        quit("NULL", 0);
//return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * This is the streaming server, run as separate thread
 */
void* streamServer(void* arg)
{
	buff.resize(BUFSIZE);
        struct  sockaddr_in   serverAddr,  clientAddr;
        socklen_t             clientAddrLen = sizeof(clientAddr);
        /* make this thread cancellable using pthread_cancel() */
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

        if ((listenSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            quit("socket() failed.", 1);
        }  
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(listenPort);

        if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
                quit("bind() failed", 1);
        }
                

        int recvlen =0;
        /* start receiving images */
        while(1)
        {
		//memset(sockData, 0x0, sizeof(sockData));
		//while(1){

                	/*for (int i = 0; i < imgSize; i += bytes) {
                        	if ((bytes = recv(connectSock, sockData +i, imgSize  - i, 0)) == -1) {
 	                              	quit("recv failed", 1);
				}
                	}
			*/
                	/* convert the received data to OpenCV's Mat format, thread safe */
			if(data_printed == 1)
			{
		        	pthread_mutex_lock(&mutex);
		                recvlen = recvfrom(listenSock, reinterpret_cast<unsigned char*>(&buff[0]), BUFSIZE*sizeof(unsigned char), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
		        	pthread_mutex_unlock(&mutex);
				is_data_ready = 1;
				data_printed = 0;
		}
	}

        /* have we terminated yet? */
        pthread_testcancel();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void* streamHost(void* arg)
{

	vector<unsigned char> init_buff;
	int INIT_BUFFSIZE = 40;
	init_buff.resize(INIT_BUFFSIZE);
        struct  sockaddr_in   serverAddr,  clientAddr;
        socklen_t             clientAddrLen = sizeof(clientAddr);
        /* make this thread cancellable using pthread_cancel() */
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
        if ((listenSock2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            quit("socket() failed.", 1);
        }  
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            quit("\n--> socket() failed.", 1);
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(listenPort2);
	
        if (bind(listenSock2, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
                quit("bind() failed", 1);
        }

        int recvlen =0;
	//Wait for initialization comms
	//while(recvlen == 0)
	recvlen = recvfrom(listenSock2, reinterpret_cast<unsigned char*>(&init_buff[0]), INIT_BUFFSIZE*sizeof(unsigned char), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
	std::cerr << "Here" << std::endl;
        while(1)
        {
			if (is_data_ready) 
			{
		        	pthread_mutex_lock(&mutex);
				if (sendto(fd,reinterpret_cast<unsigned char*>(&buff[0]), buff.size()*sizeof(unsigned char), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) 
				{
					perror("sendto failed");
					return 0;
				} 
				std::cerr << "Here" << std::endl;
				is_data_ready = 0;
				data_printed=1;
		        	pthread_mutex_unlock(&mutex);
			}
	}
        pthread_testcancel();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * This function provides a way to exit nicely from the system
 */
void quit(string msg, int retval)
{
        if (retval == 0) {
                cout << (msg == "NULL" ? "" : msg) << "\n" <<endl;
        } else {
                cerr << (msg == "NULL" ? "" : msg) << "\n" <<endl;
        }
         
        if (listenSock){
                close(listenSock);
        }

        if (connectSock){
                close(connectSock);
        }
                                
        if (!img.empty()){
                (img.release());
        }
                
        pthread_mutex_destroy(&mutex);
        exit(retval);
}
