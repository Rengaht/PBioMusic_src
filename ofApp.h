#pragma once

#include <opencv2/opencv.hpp>
//using namespace cv;

#include "ofMain.h"
#include "ofxOsc.h"

#ifdef _WIN64
#include "ofxSpout.h"
#elif __APPLE__
#include "ofxSyphon.h"
#endif



#include "FrameTimer.h"
#include "Parameter.h"
#include "DetectBlob.h"

//#define USE_VIDEO
#define USE_REF

#define VWIDTH 580.0
#define VHEIGHT 580.0

#define PWIDTH 640.0
#define PHEIGHT 480.0
//#define SIZE_LOW 20
//#define SIZE_HIGH .1

#define MAXBLOB 20


#define MMODE 5
#define MEFFECT 3

#define MAXPACMAN 50

//struct BlobComparator{
//	bool operator()(const ofxCvBlob& a_,const ofxCvBlob& b_){ 
//		return  a_.area<b_.area;
//	}
//};




class ofApp : public ofBaseApp{
	private:
		float _last_millis,_dmillis;
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


		bool _debug;


		GlobalParam *_param;
		
#ifdef USE_VIDEO
        ofVideoPlayer _video;
#else
		ofVideoGrabber 		_camera;
#endif
        cv::Mat _mat_grab,_mat_scale,_mat_resize,_mat_gray,_mat_normalize,
			_mat_thres,_mat_edge;

		ofPixels _output_pixels;
		ofImage _img_resize,_img_gray,_img_normalize,_img_thres,
			    _img_contrast,_img_edge;

		ofImage _img_ref;

        cv::Mat imageToMat(ofImage& img_);
        ofImage matToImage(cv::Mat& mat_);

		
		enum MODE {RUN,DETECT,EFFECT};
        MODE _mode;
		void setMode(MODE set_);
		
		enum DEFFECT {SCAN,EDGE_WALK,BLOB_SELECT};   
		DEFFECT _effect,_next_effect;
		void setEffect(DEFFECT set_);
		
		bool updateSource();
        void cvProcess(cv::Mat& grab_);
        void cvAnalysis(cv::Mat& grab_);
        cv::Mat _hist;
        int _hist_size;
    
        void drawAnalysis(float x_,float y_,float wid_,float hei_);
		
	    void drawContours(float p_=1);

		vector<DetectBlob> _collect_blob;


	    bool isScanned(DetectBlob blob_);
	    bool isSimilar(Blob b1_,Blob b2_);

        void stretchContrast(cv::Mat& src_,cv::Mat& dst_,int c1_,int c2_);

        void updateBlob(vector<vector<cv::Point>>& contour_,vector<cv::Vec4i>& hierachy_);
        Blob contourApproxBlob(vector<cv::Point>& contour_);
    

        //osc
	    ofxOscSender _osc_sender;
		ofxOscReceiver _osc_receiver;
	    void sendOSC(string address_,vector<float> param_);
		void receiveOSC();

        vector<vector<cv::Point> > _contours;
        vector<cv::Vec4i> _hierarchy;


		//detect
		FrameTimer _anim_detect;
		int _idetect_view;
    
        cv::Mat _mat_fft;
        stringstream _report1,_report2;
    
    
        //scan
		FrameTimer _anim_scan;


		enum SCANDIR {VERT,RADIAL};
		SCANDIR _scan_dir;
		void checkScan(SCANDIR dir_);
	    vector<bool> _scan_touched;

 	
	   //pacman
        cv::Mat _mat_nonzero;
	   vector<cv::Point> _nonzero_point;
	   vector<cv::Point> _nonzero_start;
	   vector<cv::Point> _nonzero_gstart;


	   vector<PacMan> _pacman;
	   ofFbo _fbo_pacman;
        //vector<PacMan> _ghost;
	   
       void updatePacMan(PacMan& p_);
	   bool checkWhite(int x_,int y_);
	   bool goodStep(PacMan& p_,ofVec2f dir_);
	   ofVec2f findWhiteStart(bool ghost_);
	   void addPacMan(bool ghost_);
    
       void checkPacManCollide();
    
    
	   //blob
	   //int _mselect_blob;
	   FrameTimer _anim_select;
	   float selectBlob();
	   
	   vector<DetectBlob> _selected;
	   vector<DetectBlob> _not_selected;
	   

	   //sound
	   void triggerSound(bool short_);
       void remoteVolume(int track_,float vol_);
       void triggerTurn();
       void triggerCollide(bool ghost_);
       void triggerDead();
       void triggerScan(int num_,float area_);

	   //spout
	   ofImage _sender_image;
#ifdef _WIN64
	   ofxSpout::Sender _frame_sender;
#elif TARGET_OS_MAC
        ofxSyphonServer _frame_sender;
#endif
    
    
        ofImage _img_mask;
    
    
        ofTrueTypeFont _font;
    
    
        ofSerial	_serial;
        void updateSerial();
    
    
};
