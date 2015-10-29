#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <pthread.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;
                        
Mat     img;
int     is_data_ready = 0;
int	data_printed = 1; 
int     listenSock, connectSock;
int 	listenPort;

int BUFSIZE=32768;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* streamServer(void* arg);
void  quit(string msg, int retval);
  


int main(int argc, char** argv)
{
        pthread_t thread_s;
        int width, height, key;
	width = 1280;  
	height = 720; 
 	
	if (argc != 2) {
                quit("Usage: netcv_server <listen_port> ", 0);
        }
	
	listenPort = atoi(argv[1]);
 	
        img = Mat::zeros( height,width, CV_8UC1);
        
        /* run the streaming server as a separate thread */
        if (pthread_create(&thread_s, NULL, streamServer, NULL)) {
                quit("pthread_create failed.", 1);
        }

        cout << "\n-->Press 'q' to quit." << endl;
        namedWindow("stream_server", CV_WINDOW_AUTOSIZE);

        while(key != 'q') {

                pthread_mutex_lock(&mutex);
                        if (is_data_ready) {
				//img=img.reshape(0,1);
                                imshow("stream_server", img);
                                is_data_ready = 0;
				data_printed=1;
                        }
                pthread_mutex_unlock(&mutex);
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

	vector<unsigned char> buff;
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
                
        //if (listen(listenSock, 5) == -1) {
        //        quit("listen() failed.", 1);
        //}
        
        //int  imgSize = img.total()*img.elemSize();
        //char sockData[imgSize];
        //int  bytes=0;
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
				//Seg fault here
		                recvlen = recvfrom(listenSock, reinterpret_cast<unsigned char*>(&buff[0]), BUFSIZE*sizeof(unsigned char), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
			

						

				//
				if(recvlen>0)
				{
				img = imdecode(buff,CV_LOAD_IMAGE_COLOR);
				//imshow("stream_server", img);
				cv::Size s = img.size();
				Vec3b intensity = img.at<uchar>(200,200);
				float blue = intensity.val[0];
				float green = intensity.val[1];
				float red = intensity.val[2];
				std::cerr << "Elements: " << blue << std::endl;
				}
		        	pthread_mutex_unlock(&mutex);
				is_data_ready = 1;
				data_printed = 0;
			}
		//}
	}

        /* have we terminated yet? */
        pthread_testcancel();
	
  	/* no, take a rest for a while */
        usleep(1000);
	
}
/////////////////////////////////////////////////////////////////////////////////////////////////
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

