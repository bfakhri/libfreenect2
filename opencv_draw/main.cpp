#include "opencv2/objdetect.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <iostream>
#include <stdio.h>
#include <vector>
#include <time.h>

#define SHMSZ 27

using namespace std;
using namespace cv;

/** Function Headers */
void detectAndDisplay( Mat frame );

/** Global variables */
String window_name = "EyeGazer";

// Resolution of the screen we are using
//const int X_RES = 1680;
//const int Y_RES = 1050;
const int X_RES = 1280;
const int Y_RES = 720;
//const int X_RES = 1920;
//const int Y_RES = 1080;
// Number of sections per axis we are targeting 
const float X_SEG = 10;
const float Y_SEG = 10;

// Number of nanoseconds till bubble collapses
const long NS_TILL_COLLAPSE = 2e9;

// Initial bubble radius
const int INIT_RAD = 100;


key_t key = 85991;	/* key to be passed to shmget() */ 
int shmflg; /* shmflg to be passed to shmget() */ 
int shmid; /* return value from shmget() */ 
int size; /* size to be passed to shmget() */
void *shm, *shared;	

/** @function main */
int main( void )
{
	// Create Shared Segment
	if ((shmid = shmget(key, SHMSZ, 0666)) < 0) {
	      perror("shmget");
	      exit(1);
	}
	// Attach to the segment
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
	    perror("shmat");
	    exit(1);
	}
	// Do something to the segment
	shared = shm;

	int t;
	// Test loop
	/*
	while(1){
		cin >> t;
		cout << "Value: " << *((uint8_t*)shared) << endl;
		//*((uint8_t*)shared) = 0xFF;
		if(*((uint8_t*)shared) == 0x00)
			*((uint8_t*)shared) = 0xFF;	
		else	
			*((uint8_t*)shared) = 0x00;	
		
		cout << "Value: " << *((uint8_t*)shared) << endl;
		
	}
	*/

	Mat frame = Mat(Y_RES, X_RES, CV_8UC3, Scalar::all(0));
	imshow( window_name, frame ); 

	float x_ss = X_RES/X_SEG;
	float y_ss = Y_RES/Y_SEG;
	vector<Point> all_points;

	for(int x=0; x<X_SEG; x++){
		for(int y=0; y<Y_SEG; y++){
			all_points.push_back(Point(x*x_ss+x_ss/2, y*y_ss+y_ss/2));
		}
	}

	
	// Shuffle the vector
	random_shuffle(all_points.begin(), all_points.end());    

	// Timing vars
	long ms;
	time_t s;
	struct timespec start, end;

	while ( all_points.size() > 0 )
	{
		// Get latest time
		clock_gettime(CLOCK_REALTIME, &start);
		clock_gettime(CLOCK_REALTIME, &end);
		long time_diff = 0; 
		while(time_diff < NS_TILL_COLLAPSE){
			// Draw proper bubble w/ scaled size
			frame = Mat(Y_RES, X_RES, CV_8UC3, Scalar::all(0));
			int rad = INIT_RAD*(float(NS_TILL_COLLAPSE-time_diff)/NS_TILL_COLLAPSE);
			Scalar color = Scalar(255,255,255); 
			int thickness = 1;
			if(float(rad)/INIT_RAD > 0.15){
				color = Scalar(255, 255, 255);
				thickness = 3;
			}else{
				color = Scalar(0, 255, 255);
				thickness = -1; 
			}
			circle(frame, all_points.back(), rad, color, thickness);
			imshow( window_name, frame ); 
			int c = waitKey(10);
			if( (char)c == ' ' ){
				if(thickness < 0) { 
					// Successful shot
					all_points.pop_back();
					cout << "Success:\t(" << all_points.back().x << "," << all_points.back().y << ")\t" << all_points.size() << " left" << endl;
					// Capture frame of person's face
					if(*((uint8_t*)shared) == 0x00)
						*((uint8_t*)shared) = 0xFF;	
					else	
						*((uint8_t*)shared) = 0x00;	
				}else{
					// Failed shot - put this one back into queue
					all_points.insert(all_points.begin(), all_points.back());
					all_points.pop_back();
					cout << "Failed:\t(" << all_points.back().x << "," << all_points.back().y << ")\t" << all_points.size() << " left" << endl;
				}
				
			} 
			clock_gettime(CLOCK_REALTIME, &end);
			time_diff = (end.tv_sec - start.tv_sec)*1e9 + end.tv_nsec - start.tv_nsec;
		}

		//frame = Mat(Y_RES, X_RES, CV_8UC3, Scalar::all(0));
	}
	// Escape exits the program
	while(1){
		int c = waitKey(10);
		if( (char)c == 27 ) { break; } // escape
	}
	return 0;
}

