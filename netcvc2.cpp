#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <pthread.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>


using namespace std;
using namespace cv;
                        
Mat     img,upscale_img;
int     is_data_ready = 0;
int	data_printed = 1; 
int     listenSock, connectSock,fd;
int 	listenPort;
char*   server_ip;
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
 	
	if (argc != 3) 
	{
		quit("Usage: netcv_server <server_address> <listen_port> ", 0);
    }
	std::cerr << "Here" << std::endl;
	server_ip = argv[1];
	listenPort = atoi(argv[2]);
    img = Mat::zeros( height,width, CV_8UC1);
    /* run the streaming server as a separate thread */
    if (pthread_create(&thread_s, NULL, streamServer, NULL)) 
	{
        quit("pthread_create failed.", 1);
    }
    std::cerr << "Here 2" << std::endl;
    cout << "\n-->Press 'q' to quit." << endl;
    namedWindow("stream_server", CV_WINDOW_AUTOSIZE);

    while(key != 'q') 
	{
        pthread_mutex_lock(&mutex);
        if (is_data_ready) 
	{
	    cv::resize(img, upscale_img, Size(960, 480), 0, 0, INTER_CUBIC); 
            imshow("stream_server", upscale_img);
	    //imshow("stream_server",img);
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
}
/////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * This is the streaming server, run as separate thread
 */
void* streamServer(void* arg)
{
	vector<unsigned char> buff,init_buff;
	buff.resize(BUFSIZE);
	init_buff.resize(2);
	init_buff[0] = 1;
    struct  sockaddr_in   serverAddr,  clientAddr;
    socklen_t             clientAddrLen = sizeof(clientAddr);
	
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    if ((listenSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
        quit("socket() failed.", 1);
	}
	
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
        quit("fd socket", 1);
	}
	
	memset((char*)&serverAddr, 0, sizeof(serverAddr));
	memset((char*)&clientAddr, 0, sizeof(clientAddr));
	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(listenPort);
	serverAddr.sin_addr.s_addr = inet_addr(server_ip);
	
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddr.sin_port = htons(listenPort);
    std::cerr << "Here 1" << std::endl;
	
    if(sendto(fd,reinterpret_cast<unsigned char*>(&init_buff[0]), init_buff.size()*sizeof(unsigned char), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
		quit("Failed to send", 1);
    }

    //if(bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) 
    //{
    //    quit("bind() failed", 1);
    //}
    std::cerr << "Here 2" << std::endl;
    int recvlen =0;
    while(1)
    {
		std::cerr << "Here 3" << std::endl;
		recvlen =0;
		if(data_printed == 1)
		{
		    pthread_mutex_lock(&mutex);
		    std::cerr << "Here 3" << std::endl;
		    recvlen = recvfrom(fd, reinterpret_cast<unsigned char*>(&buff[0]), BUFSIZE*sizeof(unsigned char), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
		     std::cerr << "Here 3" << std::endl;
			if(recvlen>0)
			{
				img = imdecode(buff,CV_LOAD_IMAGE_COLOR);
				cv::Size s = img.size();
				//Vec3b intensity = img.at<uchar>(200,200);
				//float blue = intensity.val[0];
				//float green = intensity.val[1];
				//float red = intensity.val[2];
				//std::cerr << "Elements: " << blue << std::endl;
			}
		    pthread_mutex_unlock(&mutex);
			is_data_ready = 1;
			data_printed = 0;
			//usleep(16000);
		}
	}
    pthread_testcancel();
}
/////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * This function provides a way to exit nicely from the system
 */
void quit(string msg, int retval)
{
        if (retval == 0) {
            cout << (msg == "NULL" ? "" : msg) << "\n" <<endl;
        } 
		else 
		{
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

