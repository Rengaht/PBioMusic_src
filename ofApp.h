#pragma once

#include "ofMain.h"
#include "ofxOpenCV.h"
#include "ofxOsc.h"


#define USE_VIDEO

#define PWIDTH 1280
#define PHEIGHT 720
#define SIZE_LOW .01
#define SIZE_HIGH .33
#define THRESHOLD 50
#define SCANSTARTVEL .01
#define SCANWIDTH 2
#define MAXBLOB 20

#define MMODE 4
#define MSCANREGION 12


struct BlobComparator{
	bool operator()(const ofxCvBlob& a_,const ofxCvBlob& b_){ 
		return  a_.area<b_.area;
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
    
		ofxCvColorImage			_img_color;

		ofxCvGrayscaleImage 	_img_gray;
		ofxCvGrayscaleImage 	_img_bg;
		ofxCvGrayscaleImage 	_img_diff;

		ofxCvContourFinder 	_contour_finder;

		
		vector<ofxCvBlob> _collect_blob;
		vector<ofxCvBlob> _last_trigger;
		void updateTrigger();

		void combineBlob(vector<ofxCvBlob>& detect_);		 

		int _threshold;
		bool _use_background;
		bool _find_holes;
    
        enum MODE {SCAN,FIX_SCAN,CONTOUR_WALK,PINBALL};
        MODE _mode;
        bool _debug;
    
	   float _scan_pos;
	   float _scan_vel;

       enum SCANDIR {VERT,RADIAL};
	   SCANDIR _scan_dir;
    

	   bool isScanned(ofxCvBlob blob_);
	   bool isSimilar(ofxCvBlob b1_,ofxCvBlob b2_);

       void trigger(int mode_,int data_);
	   ofxOscSender _osc_sender;
	   void sendOSC(string address_,int num_);
    
       //scan
       void checkScan(SCANDIR dir_);
    
};
