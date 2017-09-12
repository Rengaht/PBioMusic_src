#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    ofEnableSmoothing();
    
#ifdef USE_VIDEO
    _video.load("ref.mov");
    _video.play();
    _video.setLoopState(OF_LOOP_NORMAL);
    _video.setVolume(0);
#else
	_camera.setVerbose(true);
	_camera.setup(PWIDTH,PHEIGHT);
#endif

	_img_color.allocate(PWIDTH,PHEIGHT);
	_img_gray.allocate(PWIDTH,PHEIGHT);
	_img_bg.allocate(PWIDTH,PHEIGHT);
	_img_bg.set(0);
	_img_diff.allocate(PWIDTH,PHEIGHT);

	_use_background = false;
	_threshold = THRESHOLD;
	_find_holes=true;

	_scan_vel=SCANSTARTVEL;

	_osc_sender.setup("127.0.0.1",12000);
    
    _mode=MODE::SCAN;
    _scan_dir=SCANDIR::VERT;
}
                               
//--------------------------------------------------------------
void ofApp::update(){
	ofBackground(0);
	
	bool bNewFrame = false;

#ifdef USE_VIDEO
    _video.update();
    bNewFrame=_video.isFrameNew();
#else
	_camera.update();
	bNewFrame=_camera.isFrameNew();
#endif

	if (bNewFrame){
#ifdef USE_VIDEO
        _img_color.setFromPixels(_video.getPixels());
#else
		_img_color.setFromPixels(_camera.getPixels());
#endif  
        
       

		_img_gray=_img_color;
		_img_diff.absDiff(_img_bg, _img_gray);
		_img_diff.threshold(_threshold);

        float area_=PWIDTH*PHEIGHT;
		_contour_finder.findContours(_img_diff,area_*SIZE_LOW,area_*SIZE_HIGH, MAXBLOB, _find_holes);	// find holes
		std::sort(_contour_finder.blobs.begin(),_contour_finder.blobs.end(),BlobComparator{});
		combineBlob(_contour_finder.blobs);
        
        switch(_mode){
            case SCAN:
                _scan_pos+=_scan_vel;
                if(_scan_pos>1) _scan_pos=0;
                checkScan(_scan_dir);
                break;
            case FIX_SCAN:
                _scan_pos=.5;
                checkScan(_scan_dir);
                break;
            case CONTOUR_WALK:
                break;
            case PINBALL:
                break;
        
        }
	}

	
	
	
}



//--------------------------------------------------------------
void ofApp::draw(){
	// draw the incoming, the grayscale, the bg and the thresholded difference
	ofSetHexColor(0xffffff);
	if(_debug){
		_img_color.draw(0,0,PWIDTH/2,PHEIGHT/2);
		_img_diff.draw(0,PHEIGHT/2,PWIDTH/2,PHEIGHT/2);
		_img_gray.draw(PWIDTH/2,0,PWIDTH/2,PHEIGHT/2);
		_img_bg.draw(PWIDTH/2,PHEIGHT/2,PWIDTH/2,PHEIGHT/2);
	}else{
		_img_color.draw(0,0,ofGetWidth(),ofGetHeight());
	}

	float ratio_=_debug?.5:1;
	ofPushMatrix();
	ofScale(ratio_,ratio_);

	int len=_collect_blob.size();
	for(int i=0;i<len;++i){
		//collectBlob[i].draw(0,0);
		ofPushStyle();
		ofSetColor(0,0,255);
		ofNoFill();
		ofDrawRectangle(_collect_blob[i].boundingRect);
		ofPopStyle();
        
        if(isScanned(_collect_blob[i])){

				ofPushStyle();
				ofSetColor(255,0,0);
				ofNoFill();
				float rad_=min(_collect_blob[i].boundingRect.width,_collect_blob[i].boundingRect.height);
				ofDrawEllipse(_collect_blob[i].centroid,rad_,rad_);
				ofPopStyle();
			
		}
	}

	switch(_mode){
        case SCAN:
        case FIX_SCAN:
            ofPushStyle();
            ofSetColor(255,120);
            ofNoFill();
            if(_scan_dir==SCANDIR::VERT) ofDrawLine(_scan_pos*PWIDTH,0,_scan_pos*PWIDTH,PHEIGHT);
            else if(_scan_dir==SCANDIR::RADIAL){
                float rad=_scan_pos*min(PWIDTH,PHEIGHT)/2;
                ofDrawEllipse(PWIDTH/2,PHEIGHT/2,rad*2,rad*2);
            }
            ofPopStyle();
            break;
        default:
            break;
	}

	ofPopMatrix();


	// finally, a report:
	ofSetHexColor(0xff0000);
	stringstream reportStr;
	reportStr << "bg subtraction and blob detection" << endl
		<<"press ' ' to capture bg" << endl
		<<"threshold " << _threshold << " (press: +/-)" << endl
		<<"num blobs found " << _contour_finder.nBlobs <<" collect blob: "<<len<<endl
		<<"fps: " << ofGetFrameRate()<<endl
		<<"mode "<<_mode;
	ofDrawBitmapString(reportStr.str(), 20, 600);
}


void ofApp::combineBlob(vector<ofxCvBlob>& detect_){

	_collect_blob.clear();
	int len=detect_.size();
	for(int i=0;i<len;++i){
		ofxCvBlob b_=detect_[i];
		bool inside_=false;
		for(auto& c_:_collect_blob){
			// check
			if(b_.boundingRect.inside(c_.boundingRect)){
				inside_=true;									
			}		
			
		}
		if(!inside_) _collect_blob.push_back(b_);
	}
	
}
bool ofApp::isScanned(ofxCvBlob blob_){
	//ofLog()<<rect_.getMinX()<<"  "<<rect_.getMaxX();
//	ofRectangle rect_=blob_.boundingRect;
//	bool scan_=rect_.getMinX()<_scan_pos && rect_.getMaxX()>_scan_pos;
//	
//	// check if triggered
//	if(scan_){
//		bool new_=true;
//		for(ofxCvBlob &b:_last_trigger){
//			if(isSimilar(blob_,b)){
//				new_=false;
//				b=blob_;
//				break;
//			}
//		}
//		if(new_){
//			_last_trigger.push_back(blob_);
//			
//		}
//	}
//	return scan_;
    
    float spos_=(_scan_dir==SCANDIR::VERT)?_scan_pos*PWIDTH:_scan_pos*min(PWIDTH,PHEIGHT)/2;
    float dist=0;
    float ang=0;
    
    switch(_scan_dir){
        case VERT:
            if(spos_>=blob_.boundingRect.getMinX() && spos_<blob_.boundingRect.getMaxX()) return true;
            break;
        case RADIAL:
            dist=ofDist(blob_.centroid.x,blob_.centroid.y,PWIDTH/2,PHEIGHT/2);
            ang=atan2(blob_.centroid.y-PHEIGHT/2,blob_.centroid.x-PWIDTH/2)+HALF_PI;
            if(dist-blob_.length<spos_ && dist+blob_.length>spos_) return true;
            break;
                
    }
    return false;
    
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key){
		case '+':
			_threshold++;
			if(_threshold > 255) _threshold = 255;
			break;
		case '-':
			_threshold --;
			if(_threshold < 0) _threshold = 0;
			break;
		case ' ':
            _mode=MODE(((int)_mode+1)%MMODE);
			break;
        case 'a':
            _scan_dir=SCANDIR(((int)_scan_dir+1)%2);
            break;
		case 'd':
			_debug=!_debug;
			break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}


void ofApp::sendOSC(string address_,int num){

	ofLog()<<"send osc: "<<address_<<" "<<num;
	ofxOscMessage message_;
	message_.setAddress(address_);
	message_.addIntArg(num);
	_osc_sender.sendMessage(message_);
}


bool ofApp::isSimilar(ofxCvBlob b1_,ofxCvBlob b2_){
	if(abs(b1_.area-b2_.area)<100
	   && abs(b1_.centroid.y-b2_.centroid.y)<10) return true;
	return false;
}

void ofApp::updateTrigger(){
    for(auto i=_last_trigger.begin();i!=_last_trigger.end();){
		if(!isScanned(*i)){
			i=_last_trigger.erase(i);
		}else{
			++i;
		}
	}

}


void ofApp::checkScan(SCANDIR dir_){
    float epos_=(_scan_dir==SCANDIR::VERT)?PHEIGHT/(float)MSCANREGION:TWO_PI/(float)MSCANREGION;
    bool touched_[MSCANREGION];
    
    float ang=0;
    
    for(auto &b:_collect_blob){
        switch(_scan_dir){
            case VERT:
                if(isScanned(b)) touched_[int(floor(b.centroid.y/epos_))]=true;
                break;
            case RADIAL:
                ang=atan2(b.centroid.y-PHEIGHT/2,b.centroid.x-PWIDTH/2)+HALF_PI;
                if(isScanned(b)) touched_[(int)(floor(ang/TWO_PI))]=true;
                break;
            
        }
    }
    
}
