#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	
    ofEnableSmoothing();
	ofSetVerticalSync(true);
    
#ifdef USE_VIDEO
    _video.load("ref.mov");
    _video.play();
    _video.setLoopState(OF_LOOP_NORMAL);
    _video.setVolume(0);
#else
	_camera.setVerbose(true);
	_camera.setup(VWIDTH,VHEIGHT);
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

	_walk_vel=SCANSTARTVEL;
	_ball_rad=BALLRAD;

	for(int i=0;i<MSCANREGION;++i) _scan_touched.push_back(false);
}
                               
//--------------------------------------------------------------
void ofApp::update(){
	ofBackground(0);
	
	bool bNewFrame = false;

#ifdef USE_VIDEO
    _video.update();
    bNewFrame=_video.isFrameNew();
#else
	if(ofGetFrameNum()%3==0) _camera.update();
	bNewFrame=_camera.isFrameNew();
#endif

	if (bNewFrame){
#ifdef USE_VIDEO		
        _img_cam.setFromPixels(_video.getPixels());
#else	
		_img_cam.setFromPixels(_camera.getPixels());
#endif  
        
        _img_color.scaleIntoMe(_img_cam);
		_img_gray=_img_color;
		//_img_color.convertToGrayscalePlanarImage(_img_gray,255);
		_img_diff.absDiff(_img_bg, _img_gray);
		_img_diff.threshold(_threshold);

        float area_=PWIDTH*PHEIGHT;
		_contour_finder.findContours(_img_diff,area_*SIZE_LOW,area_*SIZE_HIGH, MAXBLOB, _find_holes);	// find holes
		std::sort(_contour_finder.blobs.begin(),_contour_finder.blobs.end(),BlobComparator{});
		updateBlob(_contour_finder.blobs);
		combineBlob(_contour_finder.blobs);

        switch(_mode){
            case SCAN:
                _scan_pos+=_scan_vel;
                if(_scan_pos>1) _scan_pos=0;
                checkScan(_scan_dir);
                break;
            case FIX_SCAN:
               // _scan_pos=.5;
                checkScan(_scan_dir);
                break;
            case CONTOUR_WALK:
				for(int i=0;i<_collect_blob.size();++i){
					_collect_blob[i]._walk_pos+=_walk_vel;
					if(_collect_blob[i]._walk_pos>1) _collect_blob[i]._walk_pos-=1;
				}
                break;
            case PINBALL:
				for(auto& b:_collect_blob) b._trigger=false;
				for(auto& b:_pinball) updatePinball(b);
                break;
        
        }
	}

	
	
	
}



//--------------------------------------------------------------
void ofApp::draw(){
	// draw the incoming, the grayscale, the bg and the thresholded difference
	ofSetHexColor(0xffffff);
	float ratio_=1;
	if(_debug){
		_img_color.draw(0,0,PWIDTH,PHEIGHT);
		_img_diff.draw(0,PHEIGHT,PWIDTH,PHEIGHT);
		_img_gray.draw(PWIDTH,0,PWIDTH,PHEIGHT);
		_img_bg.draw(PWIDTH,PHEIGHT,PWIDTH,PHEIGHT);
	}else{
		ratio_=min((float)ofGetWidth()/PWIDTH,(float)ofGetHeight()/PHEIGHT);
		_img_color.draw(0,0,PWIDTH*ratio_,PHEIGHT*ratio_);
	}

	ofPushMatrix();
	ofScale(ratio_,ratio_);

	for(auto& b:_collect_blob){
		//collectblob[i].draw(0,0);
		ofPushStyle();
		if(b._trigger) ofSetColor(255,0,0);
		else ofSetColor(0,0,255);
		ofNoFill();
			ofDrawRectangle(b._blob.boundingRect);
			ofBeginShape();
			for(auto& p:b._blob.pts) ofVertex(p.x,p.y);
			ofEndShape();
		ofPopStyle();
        
        /*if(isScanned(_collect_blob[i])){
				ofPushStyle();
				ofSetColor(255,0,0);
				ofNoFill();
				float rad_=min(_collect_blob[i].boundingRect.width,_collect_blob[i].boundingRect.height);
				ofDrawEllipse(_collect_blob[i].centroid,rad_,rad_);
				ofPopStyle();
			
		}*/
	}

	switch(_mode){
        case SCAN:
        case FIX_SCAN:
            ofPushStyle();
            ofSetColor(255,120);
            ofNoFill();
            if(_scan_dir==SCANDIR::VERT){
				float h_=(float)PHEIGHT/MSCANREGION;
				for(int i=0;i<MSCANREGION;++i){
					 if(_scan_touched[i]) ofSetColor(255,0,0);
					 else ofSetColor(255);

 					ofDrawLine(_scan_pos*PWIDTH,i*h_,_scan_pos*PWIDTH,(i+1)*h_);
				}
			}else if(_scan_dir==SCANDIR::RADIAL){
                float rad_=_scan_pos*min(PWIDTH,PHEIGHT)/2;
				ofPolyline arc_;
				arc_.arc(0,0,rad_,rad_,0,360.0/MSCANREGION,100);				
				
				for(int i=0;i<MSCANREGION;++i){
					ofPushMatrix();
					ofTranslate(PWIDTH/2,PHEIGHT/2);
					ofRotate(180.0+360.0/MSCANREGION*i);
						if(_scan_touched[i]) ofSetColor(255,0,0);
						else ofSetColor(255);
						arc_.draw();
					ofPopMatrix();
				}
            }
            ofPopStyle();
            break;
		case CONTOUR_WALK:
			ofPushStyle();
			ofSetColor(255,0,0);
			for(int i=0;i<_collect_blob.size();++i){
				float p_=(float)_collect_blob[i]._blob.nPts*_collect_blob[i]._walk_pos;
				int begin_=floor(p_);		
				int end_=(begin_+1)%_collect_blob[i]._blob.nPts;
				p_-=begin_;

				float x_=ofLerp(_collect_blob[i]._blob.pts[begin_].x,_collect_blob[i]._blob.pts[end_].x,p_);
				float y_=ofLerp(_collect_blob[i]._blob.pts[begin_].y,_collect_blob[i]._blob.pts[end_].y,p_);
				ofDrawCircle(x_,y_,2);
			}
			ofPopStyle();
			break;
		case PINBALL:
			ofPushStyle();
			ofSetColor(255,0,0);
			for(auto& p:_pinball){
				ofDrawCircle(p._pos,BALLRAD);
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
		<<"num blobs found " << _contour_finder.nBlobs 
		<<"fps: " << ofGetFrameRate()<<endl
		<<"mode "<<_mode;
	ofDrawBitmapString(reportStr.str(), 20, 600);
}

void ofApp::updateBlob(vector<ofxCvBlob>& blob_){
	int len=blob_.size();
	bool exist_=false;
	for(int i=0;i<len;++i){
		for(auto& b:_collect_blob){
			if(isSimilar(b._blob,blob_[i])){
				//b._blob=blob_[i];
				exist_=true;
				b._life=3;
				break;
			}
		}
		if(!exist_){
			DetectBlob b_;
			b_._id=_collect_blob.size();
			b_._blob=blob_[i];
			_collect_blob.push_back(b_);
		}
	}
	
	

}

void ofApp::combineBlob(vector<ofxCvBlob>& detect_){

	//_collect_blob.clear();
	//int len=detect_.size();
	//for(int i=0;i<len;++i){
	//	ofxCvBlob b_=detect_[i];
	//	bool inside_=false;
	//	for(auto& c_:_collect_blob){
	//		// check
	//		if(b_.boundingRect.inside(c_.boundingRect)){
	//			inside_=true;									
	//		}		
	//		
	//	}
	//	if(!inside_) _collect_blob.push_back(b_);
	//}

	for(auto i=_collect_blob.begin();i!=_collect_blob.end();){
		if(i->_life<0)
			i=_collect_blob.erase(i);
		else i->_life--;
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
           // ang=atan2(blob_.centroid.y-PHEIGHT/2,blob_.centroid.x-PWIDTH/2)+HALF_PI;
			float brad_=min(blob_.boundingRect.width,blob_.boundingRect.height)/2;
			if(dist-brad_<spos_ && dist+brad_>spos_) return true;
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
            initMode(MODE(((int)_mode+1)%MMODE));
			break;
        case 'a':
			switch(_mode){
				case SCAN:
				case FIX_SCAN:
					_scan_dir=SCANDIR(((int)_scan_dir+1)%2);
					break;
				case CONTOUR_WALK:
					//TODO: reverse!
					break;
				case PINBALL:
					addPinball();
					break;
			}
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
   /* for(auto i=_last_trigger.begin();i!=_last_trigger.end();){
		if(!isScanned(*i)){
			i=_last_trigger.erase(i);
		}else{
			++i;
		}
	}*/

}


void ofApp::checkScan(SCANDIR dir_){
    float epos_=(_scan_dir==SCANDIR::VERT)?PHEIGHT/(float)MSCANREGION:TWO_PI/(float)MSCANREGION;
    //bool touched_[MSCANREGION];
	for(int i=0;i<MSCANREGION;++i) _scan_touched[i]=false;

    float ang=0;
    
	for(auto& b:_collect_blob){
		
        switch(_scan_dir){
            case VERT:
                if(isScanned(b._blob)){
					_scan_touched[int(floor(b._blob.centroid.y/epos_))]=true;
					b._trigger=true;
				}else b._trigger=false;
                break;
            case RADIAL:
                ang=atan2(b._blob.centroid.y-PHEIGHT/2,b._blob.centroid.x-PWIDTH/2)+PI;				
                if(isScanned(b._blob)){
					int region_=(int)(floor(ang/TWO_PI*MSCANREGION));
					_scan_touched[region_]=true;
					b._trigger=true;
				}else b._trigger=false;
                break;
            
        }
    }
    
}

void ofApp::initMode(MODE set_){
	switch(set_){
		case CONTOUR_WALK:
			for(auto& b:_collect_blob){
				b._walk_pos=0;
			}
			break;
		default:
			_pinball.clear();
			break;
	}
	_mode=set_;
}

void ofApp::addPinball(){
	PinBall ball_;
	ball_._pos=ofVec2f(ofRandom(PWIDTH),ofRandom(PHEIGHT));
	ball_._vel=ofVec2f(ofRandom(10.0),0);
	ball_._vel.rotate(ofRandom(360));
	
	_pinball.push_back(ball_);
}

void ofApp::updatePinball(PinBall& p_){
	//check collide
	p_._acc*=0;
	
	for(auto& b:_collect_blob){		
		if(b._blob.boundingRect.inside(p_._pos)){
			ofVec2f a_(p_._pos.x-b._blob.centroid.x,p_._pos.y-b._blob.centroid.y);
			a_.normalize();
			a_*=2.0;
			p_._acc+=a_;
			b._trigger=true;
		}				
	}
	p_._acc.normalize();
	p_._acc*=2.0;

	if(p_._pos.x<0) p_._acc.x+=5.0;
	if(p_._pos.x>PWIDTH) p_._acc.x-=5.0;

	if(p_._pos.y<0) p_._acc.y+=5.0;
	if(p_._pos.y>PHEIGHT) p_._acc.y-=5.0;

	

	//update pos
	p_._vel+=p_._acc;
	p_._vel.normalize();
	p_._vel*=5.0;


	p_._pos+=p_._vel;
	
	 
}