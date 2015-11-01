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

VideoCapture    capture;
Mat             img0, img1, img2;
int             is_data_ready = 1;
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
                capture.open(argv[3]);
        } else {
                capture.open(0);
        }

        if (!capture.isOpened()) {
                quit("\n--> cvCapture failed", 1);
        }

        server_ip   = argv[1];
        server_port = atoi(argv[2]);



	capture.set(CV_CAP_PROP_FRAME_WIDTH,640);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT,480);



        capture >> img0;
        img1 = Mat::zeros(img0.rows, img0.cols ,CV_8UC1);

        // run the streaming client as a separate thread 
        if (pthread_create(&thread_c, NULL, streamClient, NULL)) {
                quit("\n--> pthread_create failed.", 1);
        }

        cout << "\n--> Press 'q' to quit. \n\n" << endl;

        /* print the width and height of the frame, needed by the client */
        cout << "\n--> Transferring  (" << img0.cols << "x" << img0.rows << ")  images to the:  " << server_ip << ":" << server_port << endl;

        //namedWindow("stream_client", CV_WINDOW_AUTOSIZE);
                        //flip(img0, img0, 1);
                        //cvtColor(img0, img1, CV_BGR2GRAY);

        while(key != 'q') {
                /* get a frame from camera */
                //capture >> img0;
                //if (img0.empty()) break;

                pthread_mutex_lock(&mutex);

		
                capture >> img0;
                if (img0.empty()) break;

                        //flip(img0, img0, 1);
                        //cvtColor(img0, img1, CV_BGR2GRAY);

                is_data_ready = 1;

                pthread_mutex_unlock(&mutex);

                /*also display the video here on client */
	
                imshow("stream_client", img2);
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
        /*if ((clientSock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
            quit("\n--> socket() failed.", 1);
        }

        serverAddr.sin_family = PF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(server_ip);
        serverAddr.sin_port = htons(server_port);

        if (connect(clientSock, (sockaddr*)&serverAddr, serverAddrLen) < 0) {
                quit("\n--> connect() failed.", 1);
        }

        int  imgSize = img0.total()*img0.elemSize();
        int  bytes=0;
        //img2 = (img1.reshape(0,1)); // to make it continuous
	*/



	struct hostent *hp;     /* host information */
	struct sockaddr_in servaddr;    /* server address */

	/* fill in the server's address and data */
	memset((char*)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(server_port);

	/* look up the address of the server given its name */
	//hp = gethostbyname(host);
	//if (!hp) {
	//	fprintf(stderr, "could not obtain address of %s\n", host);
	//	return 0;
	//}

	/* put the host's address into the server address structure */
	//memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

	int  imgSize = img0.total()*img0.elemSize();
        int  bytes=0;
        //img2 = (img0.reshape(0,1)); // to make it continuous
 	img2 = img0;
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
  			cv::resize(img0, img0, cv::Size(640, 480));
			imencode(".jpg", img0, buff, compression_params);		
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
                //usleep(1000);   //1000 Micro Sec
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
        if (capture.isOpened()){
                capture.release();
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




