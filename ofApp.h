#pragma once

#include "ofMain.h"
#include "ofxOsc.h"

#include <opencv2/opencv.hpp>
using namespace cv;

//#define USE_VIDEO

#define VWIDTH 640	
#define VHEIGHT 480

#define PWIDTH 640
#define PHEIGHT 480
#define SIZE_LOW 200
#define SIZE_HIGH .5
#define THRESHOLD 20
#define SCANSTARTVEL .01
#define SCANWIDTH 2
#define MAXBLOB 20

#define LIFESPAN 1

#define MMODE 4
#define MSCANREGION 12

#define BALLRAD 2

#define CONTRASTR1 90
#define CONTRASTR2 140

//struct BlobComparator{
//	bool operator()(const ofxCvBlob& a_,const ofxCvBlob& b_){ 
//		return  a_.area<b_.area;
//	}
//};
struct Blob{
	vector<Point> _contours;
	int _npts;

	Point2f _center;
	Rect _bounding;
	float _rad;
	
};
struct DetectBlob{
	int _id;
	float _walk_pos;

	Blob _blob;

	bool _trigger;
	int _life;
	DetectBlob(){
		_trigger=false;
		_walk_pos=0;
		_life=LIFESPAN;
	}
};
struct PinBall{
	ofVec2f _pos;
	ofVec2f _vel;
	ofVec2f _acc;
	PinBall(){
		_pos=ofVec2f(0,0);
		_vel=ofVec2f(0,0);
		_acc=ofVec2f(0,0);
	}
};



class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
#ifdef USE_VIDEO
        ofVideoPlayer _video;
#else
		ofVideoGrabber 		_camera;
#endif
		Mat _mat_grab,_mat_gray,_mat_contrast,_mat_thres;
		ofPixels _output_pixels;
		ofImage _img_gray,_img_thres,_img_contrast;

		Mat imageToMat(ofImage& img_);
		ofImage matToImage(Mat& mat_);

		vector<DetectBlob> _collect_blob;

 

		int _threshold;
		bool _use_background;
		bool _find_holes;
    
        enum MODE {SCAN,FIX_SCAN,CONTOUR_WALK,PINBALL};
        MODE _mode;
		void initMode(MODE set_);
		
		bool _debug;
    
	   float _scan_pos;
	   float _scan_vel;

       enum SCANDIR {VERT,RADIAL};
	   SCANDIR _scan_dir;
    

	   bool isScanned(DetectBlob blob_);
	   bool isSimilar(Blob b1_,Blob b2_);

	   int _contrast_r1,_contrast_r2;
	   void stretchContrast(Mat& src_,Mat& dst_);

	   void updateBlob(vector<vector<Point>>& contour_,vector<Vec4i>& hierachy_);
	   Blob contourApproxBlob(vector<Point>& contour_);


       void trigger(int mode_,int data_);
	   ofxOscSender _osc_sender;
	   void sendOSC(string address_,int num_);


	   vector<vector<Point> > _contours;
	   vector<Vec4i> _hierarchy;

	   
    
       //scan
       void checkScan(SCANDIR dir_);
	   vector<bool> _scan_touched;

	   //contour
	   float _walk_vel;
	
	   //pinBall
	   float _ball_rad;
	   vector<PinBall> _pinball;
	   void addPinball();
	   void updatePinball(PinBall& b_);
};
