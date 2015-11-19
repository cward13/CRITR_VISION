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


int HEIGHT = 240; 
int WIDTH = 320;


VideoCapture    captureL,captureR;
Mat             img0, img1, img2;
int             is_data_ready = 0;
int             fd;
char*     	server_ip;
int       	server_port;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* streamClient(void* arg);
void  quit(string msg, int retval);

int main(int argc, char** argv)
{
        pthread_t   thread_c;
        int         key;

        if (argc < 3) {
                quit("Usage: netcv_client <server_ip> <server_port> <input_file>(optional)", 0);
        }
        if (argc == 4) {
                captureL.open(argv[3]);
		captureR.open(argv[4]);
        } else {
                captureL.open(1);
		//captureR=captureL;
		captureR.open(0);
        }

        if (!captureL.isOpened()) {
                quit("\n--> cvCapture failed", 1);
        }

	if (!captureR.isOpened()) {
                quit("\n--> cvCapture failed", 1);
        }

        server_ip   = argv[1];
        server_port = atoi(argv[2]);



	captureL.set(CV_CAP_PROP_FRAME_WIDTH,WIDTH);
	captureL.set(CV_CAP_PROP_FRAME_HEIGHT,HEIGHT);

	captureR.set(CV_CAP_PROP_FRAME_WIDTH,WIDTH);
	captureR.set(CV_CAP_PROP_FRAME_HEIGHT,HEIGHT);


        captureL >> img0;
	captureR >> img1;
        // run the streaming client as a separate thread 
        if (pthread_create(&thread_c, NULL, streamClient, NULL)) {
                quit("\n--> pthread_create failed.", 1);
        }



        //namedWindow("stream_client", CV_WINDOW_AUTOSIZE);
                        //flip(img0, img0, 1);
                        //cvtColor(img0, img1, CV_BGR2GRAY);

        /* print the width and height of the frame, needed by the client */
        cout << "\n--> Transferring  Left (" << img0.cols << "x" << img0.rows << ")  images to the:  " << server_ip << ":" << server_port << endl;
	cout << "\n--> Transferring  Right (" << img1.cols << "x" << img1.rows << ")  images to the:  " << server_ip << ":" << server_port << endl;


	img2 = Mat(Size(2*WIDTH, HEIGHT), CV_8UC3, cv::Scalar(0,0,0));
	Mat left(img2.colRange(0,WIDTH).rowRange(0,HEIGHT));
    	Mat right(img2.colRange(WIDTH,2*WIDTH).rowRange(0,HEIGHT));	

        while(key != 'q') {

		if(is_data_ready == 0)
		{
                	pthread_mutex_lock(&mutex);
                	captureL >> img0;
               		if (img0.empty()) break;	
			captureR >> img1;
                	if (img1.empty()) break;
    			img0.copyTo(left);
    			img1.copyTo(right);		
                	pthread_mutex_unlock(&mutex);
		}
                is_data_ready = 1;
		usleep(32000);
                /*also display the video here on client */
                key = waitKey(30);
        }

        /* user has pressed 'q', terminate the streaming client */
        if (pthread_cancel(thread_c)) {
                quit("\n--> pthread_cancel failed.", 1);
        }

        /* free memory */
        //destroyWindow("stream_client");
        quit("\n--> NULL", 0);
return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * This is the streaming client, run as separate thread
 */
void* streamClient(void* arg)
{
        //struct  sockaddr_in serverAddr;
	//socklen_t           serverAddrLen = sizeof(serverAddr);

        /* make this thread cancellable using pthread_cancel() */
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            quit("\n--> socket() failed.", 1);
        }

	struct hostent *hp;     /* host information */
	struct sockaddr_in servaddr;    /* server address */

	/* fill in the server's address and data */
	memset((char*)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(server_port);

	int  imgSize = 2*img2.total()*img2.elemSize();
        int  bytes=0;

	vector<unsigned char>buff;
	vector<int> compression_params;
    	compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    	compression_params.push_back(20);
	
	servaddr.sin_addr.s_addr = inet_addr(server_ip);
        /* start sending images */
        while(1)
        {
                /* send the grayscaled frame, thread safe */
                if (is_data_ready) {
                        pthread_mutex_lock(&mutex);
                        /* send a message to the server */
  			//cv::resize(img2, img2, cv::Size(640, 480));
			//imshow("stream_client", img2);
			imencode(".jpg", img2, buff, compression_params);		
			//if (sendto(fd,img0.data, imgSize, 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
			if (sendto(fd,reinterpret_cast<unsigned char*>(&buff[0]), buff.size()*sizeof(unsigned char), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
			{
				perror("sendto failed");
				return 0;
			} 
			std::cerr << "Elements: " << buff.size     () << std::endl;
 			std::cerr << "Capacity: " << buff.capacity () << std::endl;			
			is_data_ready = 0;
                        pthread_mutex_unlock(&mutex);
                }
                pthread_testcancel();
                /* no, take a rest for a while */
        }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * this function provides a way to exit nicely from the system
 */
void quit(string msg, int retval)
{
        if (retval == 0) {
                cout << (msg == "NULL" ? "" : msg) << "\n" << endl;
        } else {
                cerr << (msg == "NULL" ? "" : msg) << "\n" << endl;
        }
        if (captureL.isOpened()){
                captureL.release();
		captureR.release();
        }
        if (!(img0.empty())){
                (~img0);
        }
        if (!(img1.empty())){
                (~img1);
        }
        if (!(img2.empty())){
                (~img2);
        }
        pthread_mutex_destroy(&mutex);
        exit(retval);
}




