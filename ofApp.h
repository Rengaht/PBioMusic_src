#pragma once

#include "ofMain.h"
#include "ofxOpenCV.h"
#include "ofxOsc.h"


//#define USE_VIDEO

#define VWIDTH 640	
#define VHEIGHT 480

#define PWIDTH 640
#define PHEIGHT 480
#define SIZE_LOW .001
#define SIZE_HIGH .8
#define THRESHOLD 50
#define SCANSTARTVEL .01
#define SCANWIDTH 2
#define MAXBLOB 20

#define MMODE 4
#define MSCANREGION 12

#define BALLRAD 2

struct BlobComparator{
	bool operator()(const ofxCvBlob& a_,const ofxCvBlob& b_){ 
		return  a_.area<b_.area;
	}
};
struct DetectBlob{
	int _id;
	float _walk_pos;
	ofxCvBlob _blob;
	bool _trigger;
	int _life;
	DetectBlob(){
		_trigger=false;
		_walk_pos=0;
		_life=3;
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
		ofxCvColorImage _img_cam;
		ofxCvColorImage			_img_color;
		

		ofxCvGrayscaleImage 	_img_gray;
		ofxCvGrayscaleImage 	_img_bg;
		ofxCvGrayscaleImage 	_img_diff;

		ofxCvContourFinder 	_contour_finder;

		
		vector<DetectBlob> _collect_blob;
		void updateTrigger();

		void combineBlob(vector<ofxCvBlob>& detect_);		 

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
    

	   bool isScanned(ofxCvBlob blob_);
	   bool isSimilar(ofxCvBlob b1_,ofxCvBlob b2_);

	   void updateBlob(vector<ofxCvBlob>& blob_);

       void trigger(int mode_,int data_);
	   ofxOscSender _osc_sender;
	   void sendOSC(string address_,int num_);

	   
    
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
