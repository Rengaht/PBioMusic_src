#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	
    ofEnableSmoothing();
	ofSetVerticalSync(true);

	_param=new GlobalParam();
    
#if defined(USE_VIDEO)
    _video.load("ref.mov");
    _video.play();
    _video.setLoopState(OF_LOOP_NORMAL);
    _video.setVolume(0);
	_mat_grab=Mat(PHEIGHT,PWIDTH,CV_8UC3);
#elif defined(USE_REF)
	_img_ref.load("onion.jpg");
	_mat_grab=Mat(PHEIGHT,PWIDTH,CV_8UC3);
#else
	_camera.setVerbose(true);
	_camera.setup(VWIDTH,VHEIGHT);
	_mat_grab=Mat(PHEIGHT,PWIDTH,CV_8UC3);
#endif

	

	_mat_resize=Mat(PHEIGHT,PWIDTH,CV_8UC3);
	_mat_gray=Mat(PHEIGHT,PWIDTH,CV_8UC1);
	_mat_normalize=Mat(PHEIGHT,PWIDTH,CV_8UC1);
	_mat_thres=Mat(PHEIGHT,PWIDTH,CV_8UC1);
	_mat_edge=Mat(PHEIGHT,PWIDTH,CV_8UC1);


	_osc_sender.setup(_param->_live_addr,9000);
	_osc_receiver.setup(9001);
    

	_anim_detect=FrameTimer(200);

	setMode(MODE::RUN);

	_anim_scan=FrameTimer(_param->_scan_vel);
	_anim_scan.setContinuous(true);

	_anim_select=FrameTimer(_param->_select_vel);
//	_mselect_blob=_param->_mselect;

	_scan_dir=SCANDIR::VERT;

	for(int i=0;i<_param->_mscan_region;++i) _scan_touched.push_back(false);


	//setup spout
	_spout_sender.init("_MicroMusic_Sender");

}
                               
//--------------------------------------------------------------
void ofApp::update(){
	ofBackground(0);

	if(_debug){
		bool new_=updateSource();
		if(new_) cvProcess(_mat_resize);
		return;
	}

	_dmillis=ofClamp(ofGetElapsedTimeMillis()-_last_millis,0,1000.0/60.0);
	_last_millis+=_dmillis;


	bool new_=false;
	 new_=updateSource();
	
	switch(_mode){
		case RUN:
			break;
		case DETECT:
			
			// something for drawing
			_anim_detect.update(_dmillis);
		
			if(_anim_detect.val()>=1){
				_idetect_view++;
				
				if(_idetect_view>4){
					setMode(MODE::EFFECT);
					setEffect(_next_effect);
				}else _anim_detect.restart();				
			}
			break;
		case EFFECT:
			for(auto& b:_collect_blob) b.update(_dmillis);
			switch(_effect){
				case SCAN:
					_anim_scan.update(_dmillis);					
					checkScan(_scan_dir);
					break;
				case EDGE_WALK:
				
					for(auto& p:_pacman) updatePacMan(p);
					break;
				case BLOB_SELECT:
					_anim_select.update(_dmillis);
					if(_anim_select.val()>=1){						
						if(_not_selected.size()>0) selectBlob();
						_anim_select.restart();
					}
					for(auto& b:_selected) b.update(_dmillis);
					break;
			}
			break;


	}
	
	receiveOSC();


	//send spout
	_spout_image.grabScreen(0,0,ofGetWidth(),ofGetHeight());
	_spout_sender.send(_spout_image.getTexture());
}



//--------------------------------------------------------------
void ofApp::draw(){
	// draw the incoming, the grayscale, the bg and the thresholded difference
	ofSetHexColor(0xffffff);
	float ratio_=1;	
	float ratioy_=1;
	if(_debug){
		_img_resize.draw(0,0,PWIDTH,PHEIGHT);
		_img_normalize.draw(PWIDTH,0,PWIDTH,PHEIGHT);
		_img_edge.draw(0,PHEIGHT,PWIDTH,PHEIGHT);
		_img_thres.draw(PWIDTH,PHEIGHT,PWIDTH,PHEIGHT);
		
		for(auto& b:_collect_blob) b.drawDebug();


		return;
	}else{
		ratio_=min((float)ofGetWidth()/PWIDTH,(float)ofGetHeight()/PHEIGHT);		
	}
	
	stringstream reportStr;
	
	float rw_=_anim_detect.val()*PWIDTH;
	float sw_=_anim_detect.val()*PWIDTH;

	int len_;

	ofPushMatrix();
	ofScale(ratio_,ratio_);

	switch(_mode){
		case RUN:
			reportStr<<"mode:run"<<endl;
			_img_resize.draw(0,0,PWIDTH,PHEIGHT);
			break;
		case DETECT:
			reportStr<<"mode:detect"<<endl
					 <<"detect blob= "<<_collect_blob.size()<<endl;
			
			switch(_idetect_view){
				case 0:
					_img_resize.draw(0,0,PWIDTH,PHEIGHT);					
					_img_normalize.drawSubsection(0,0,rw_,PHEIGHT,0,0);
					break;
				case 1:
					_img_normalize.draw(0,0,PWIDTH,PHEIGHT);					
					_img_thres.drawSubsection(0,0,rw_,PHEIGHT,0,0);
					break;
				case 2:
					_img_thres.draw(0,0,PWIDTH,PHEIGHT);					
					_img_edge.drawSubsection(0,0,rw_,PHEIGHT,0,0);
					break;
				case 3:
					ofPushStyle();
					ofSetColor(255,255.0*(1-_anim_detect.val()));
					_img_edge.draw(0,0,PWIDTH,PHEIGHT);		
					ofPopStyle();

					ofPushMatrix();
						for(auto& b:_collect_blob) b.draw(_anim_detect.val(),false);
					ofPopMatrix();
					break;
				case 4:
					ofPushStyle();
					ofSetColor(255,255.0*_anim_detect.val());

					if(_next_effect==DEFFECT::EDGE_WALK) _img_edge.draw(0,0,PWIDTH,PHEIGHT);		
					else{
						_img_resize.draw(0,0,PWIDTH,PHEIGHT);		
					}
					ofPopStyle();

					ofPushMatrix();
						for(auto& b:_collect_blob) b.draw(1.0,false);
					ofPopMatrix();

					break;
			}

			break;
		case EFFECT:
			reportStr<<"mode:effect"<<endl;

		


			switch(_effect){
			case SCAN:        
				_img_resize.draw(0,0,PWIDTH,PHEIGHT);
				reportStr<<"effect:scan"<<endl;

				for(auto& b:_collect_blob) b.draw(1.0,false);

				ofPushStyle();
				ofSetColor(255,120);
				ofNoFill();
				if(_scan_dir==SCANDIR::VERT){
					float h_=(float)PHEIGHT/_param->_mscan_region;
					for(int i=0;i<_param->_mscan_region;++i){
						if(_scan_touched[i]) ofSetColor(255,0,0);
						else ofSetColor(255);

						ofDrawLine(_anim_scan.val()*PWIDTH,i*h_,_anim_scan.val()*PWIDTH,(i+1)*h_);
					}
				}else if(_scan_dir==SCANDIR::RADIAL){
					float rad_=_anim_scan.val()*max(PWIDTH,PHEIGHT)/2;
					ofPolyline arc_;
					arc_.arc(0,0,rad_,rad_,0,360.0/_param->_mscan_region,100);				

					for(int i=0;i<_param->_mscan_region;++i){
						ofPushMatrix();
						ofTranslate(PWIDTH/2,PHEIGHT/2);
						ofRotate(180.0+360.0/_param->_mscan_region*i);
						if(_scan_touched[i]) ofSetColor(255,0,0);
						else ofSetColor(255);
						arc_.draw();
						ofPopMatrix();
					}
				}
				ofPopStyle();
				break;
			case EDGE_WALK:
				_img_thres.draw(0,0,PWIDTH,PHEIGHT);
				//for(auto& b:_collect_blob) b.draw(1.0,false);

				reportStr<<"effect:edge"<<endl;
				for(auto& p:_pacman) p.draw();
				
				break;
			case BLOB_SELECT:
				_img_resize.draw(0,0,PWIDTH,PHEIGHT);
				len_=_selected.size();
				if(len_>1){
					for(int i=0;i<len_-1;++i){
						_selected[i].draw(1.0,true);
					}
					_selected[len_-1].draw(_anim_select.val(),true);
				}

				reportStr<<"effect:blob"<<endl;
				break;
			default:
				break;
			}

		
			break;
	}
	ofPopMatrix();



	// finally, a report:
	ofSetHexColor(0xff0000);
	
	reportStr<<"fps: " << ofGetFrameRate()<<endl;			 			
	ofDrawBitmapString(reportStr.str(), 20, 600);
}

bool ofApp::updateSource(){
	bool bNewFrame = false;

#if defined(USE_VIDEO)
	_video.update();
	bNewFrame=_video.isFrameNew();
#elif defined(USE_REF)
	bNewFrame=true;
#else
	if(ofGetFrameNum()%3==0) _camera.update();
	bNewFrame=_camera.isFrameNew();
#endif

	if(bNewFrame){
#if defined(USE_VIDEO)
		_mat_grab=Mat(_video.getHeight(),_video.getWidht(),CV_8UC3,_video.getPixelsRef().getData());
#elif defined(USE_REF)
		_mat_grab=Mat(_img_ref.getHeight(),_img_ref.getWidth(),CV_8UC3,_img_ref.getPixelsRef().getData()); 
#else			
		_mat_grab=Mat(_camera.getHeight(),_camera.getWidth(),CV_8UC3,_camera.getPixelsRef().getData());
#endif  
	}
	
	cv::resize(_mat_grab,_mat_resize,Size(PWIDTH,PHEIGHT));
	_img_resize=matToImage(_mat_resize);


	return bNewFrame;
}

// for main opencv processing
void ofApp::cvProcess(Mat& grab_){


	//stretchContrast(_mat_resize,_mat_contrast,_param->_contrast_low,_param->_contrast_high);

	cv::cvtColor(grab_,_mat_gray,CV_BGR2GRAY);
	cv::blur(_mat_gray,_mat_gray,Size(3,3));

	//cv::equalizeHist(_mat_gray,_mat_normalize);
	cv::normalize(_mat_gray,_mat_normalize,0,255,CV_MINMAX);
	/*auto clahe=cv::createCLAHE();
	clahe->apply(_mat_gray,_mat_normalize);*/

	//cv::threshold(_mat_gray,_mat_thres,_param->_bi_thres,255,THRESH_BINARY_INV);
	cv::adaptiveThreshold(_mat_normalize,_mat_thres,255,ADAPTIVE_THRESH_GAUSSIAN_C,THRESH_BINARY_INV,33,0);


	float area_=PWIDTH*PHEIGHT;

	cv::Canny(_mat_thres, _mat_edge,50,150,3); 	
	_contours.clear();
	_hierarchy.clear();

	Mat copy_;
	_mat_thres.copyTo(copy_);

	cv::findContours(copy_,_contours,_hierarchy,CV_RETR_CCOMP,CHAIN_APPROX_SIMPLE,Point(0,0));
	updateBlob(_contours,_hierarchy);

	
	_img_gray=matToImage(_mat_gray);
	_img_thres=matToImage(_mat_thres);
	_img_edge=matToImage(_mat_edge);
	_img_normalize=matToImage(_mat_normalize);


}

void ofApp::updateBlob(vector<vector<Point>>& contour_,vector<Vec4i>& hierachy_){
	
	// approx contour
	int clen=contour_.size();
	vector<Blob> blob_;
	float frame_=PWIDTH*PHEIGHT;

	for(int i=0;i<clen;++i){
		float area_=cv::contourArea(contour_[i]);		
		if(area_>_param->_blob_small && area_<_param->_blob_large*frame_)
			blob_.push_back(contourApproxBlob(contour_[i]));
	}

	_collect_blob.clear();
	int len=blob_.size();
	for(int i=0;i<len;++i){
		DetectBlob b_;
		b_._id=_collect_blob.size();
		b_._blob=blob_[i];
		_collect_blob.push_back(b_);
	}

	// track exist
	
	//int len=blob_.size();
	//
	//for(int i=0;i<len;++i){
	//	bool exist_=false;
	//	for(auto& b:_collect_blob){
	//		if(isSimilar(b._blob,blob_[i])){
	//			//b._blob=blob_[i];
	//			exist_=true;
	//			b._life=LIFESPAN;
	//			break;
	//		}
	//	}
	//	if(!exist_){
	//		DetectBlob b_;
	//		b_._id=_collect_blob.size();
	//		b_._blob=blob_[i];
	//		_collect_blob.push_back(b_);
	//	}
	//}
	//
	//// erase dead
	//for(auto i=_collect_blob.begin();i!=_collect_blob.end();){
	//	if(i->_life<0)
	//		i=_collect_blob.erase(i);
	//	else{
	//		i->_life--;
	//		i++;
	//	}
	//}

}

bool ofApp::isScanned(DetectBlob detect_){
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
    
    float spos_=(_scan_dir==SCANDIR::VERT)?_anim_scan.val()*PWIDTH:_anim_scan.val()*min(PWIDTH,PHEIGHT)/2;
    float dist=0;
    float ang=0;
    
    switch(_scan_dir){
        case VERT:
            if(spos_>=detect_._blob._bounding.x && spos_<detect_._blob._bounding.x+detect_._blob._bounding.width) return true;
            break;
        case RADIAL:
            dist=ofDist(detect_._blob._center.x,detect_._blob._center.y,PWIDTH/2,PHEIGHT/2);
           // ang=atan2(blob_.centroid.y-PHEIGHT/2,blob_.centroid.x-PWIDTH/2)+HALF_PI;
			float brad_=max(detect_._blob._bounding.width,detect_._blob._bounding.height)/2;
			if(dist-brad_<spos_ && dist+brad_>spos_) return true;
            break;
                
    }
    return false;
    
}


void ofApp::drawContours(float p_){

	//for(auto& b:_collect_blob) b.draw(p_);
	
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key){
		
		case '1':
            setMode(MODE::RUN);
			break;
		case '2':
			setMode(MODE::DETECT);
			break;
		case 'S':
			_next_effect=DEFFECT::SCAN;
			setMode(MODE::DETECT);
			break;
		case 'B':
			_next_effect=DEFFECT::BLOB_SELECT;			
			setMode(MODE::DETECT);
			break;
		case 'E':
			_next_effect=DEFFECT::EDGE_WALK;
			setMode(MODE::DETECT);
			break;

        case 'a':
			switch(_effect){
				case SCAN:
					_scan_dir=SCANDIR(((int)_scan_dir+1)%2);
					break;		
				case EDGE_WALK:
					addPacMan(false);
					break;
				case BLOB_SELECT:
					setEffect(DEFFECT::BLOB_SELECT);
					break;
			}
			break;
		case 'q':
			switch(_effect){
				case EDGE_WALK:
					addPacMan(true);
					break;
			}
			break;
		case 'd':
			_debug=!_debug;
			break;
		case 'c':
			_collect_blob.clear();
		
		case 's':
			_param->saveParameterFile();
			break;
		case 'k':
			triggerSound(true);
			break;
		case 'l':
			triggerSound(false);
			break;
	}
}
#pragma region OF_UI
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
#pragma endregion


void ofApp::setMode(MODE set_){
	switch(set_){
		case DETECT:
			_anim_detect.restart();
			_idetect_view=0;
			cvProcess(_mat_resize);
			for(auto& b:_collect_blob) b.setTrigger(true);			
			break;
		case EFFECT:
			for(auto& b:_collect_blob) b.setTrigger(false);			
			break;				
	}
	_mode=set_;
}

void ofApp::sendOSC(string address_,vector<int> param_){

	cout<<"send osc: "<<address_<<" ";
	ofxOscMessage message_;
	message_.setAddress(address_);
	for(auto a:param_){
		message_.addIntArg(a);
		cout<<a<<" ";
	}
	cout<<endl;
	_osc_sender.sendMessage(message_);
}


bool ofApp::isSimilar(Blob b1_,Blob b2_){
	if(abs(b1_._bounding.area()-b2_._bounding.area())<200
	   && abs(b1_._center.y-b2_._center.y)<20) return true;
	return false;
}

void ofApp::checkScan(SCANDIR dir_){
    float epos_=(_scan_dir==SCANDIR::VERT)?PHEIGHT/(float)_param->_mscan_region
				:TWO_PI/(float)_param->_mscan_region;
    //bool touched_[MSCANREGION];
	for(int i=0;i<_scan_touched.size();++i) _scan_touched[i]=false;

    float ang=0;
    
	for(auto& b:_collect_blob){
		
        switch(_scan_dir){
            case VERT:
                if(isScanned(b)){
					int pos=int(floor(b._blob._center.y/epos_));
					pos=ofClamp(pos,0,_scan_touched.size()-1);
					_scan_touched[pos]=true;
					b.setTrigger(true);
				}else b.setTrigger(false);
                break;
            case RADIAL:
                ang=atan2(b._blob._center.y-PHEIGHT/2,b._blob._center.x-PWIDTH/2)+PI;				
                if(isScanned(b)){
					int region_=(int)(floor(ang/TWO_PI*_param->_mscan_region));
					region_=ofClamp(region_,0,_scan_touched.size()-1);
					_scan_touched[region_]=true;
					b.setTrigger(true);
				}else b.setTrigger(false);
                break;
            
        }
    }
    
}

void ofApp::setEffect(DEFFECT set_){

	setMode(MODE::EFFECT);
	int len_=0;
	switch(set_){
		case EDGE_WALK:
			cv::findNonZero(_mat_thres,_mat_nonzero);
			len_=_mat_nonzero.total();
			_nonzero_point.clear();
			_nonzero_start.clear();
			for(int i=0;i<len_;++i){
				_nonzero_point.push_back(_mat_nonzero.at<Point>(i));
				if(i<len_/10.0){
					_nonzero_start.push_back(_mat_nonzero.at<Point>(i));
				}else if(i>len_/10.0*9){
					_nonzero_gstart.push_back(_mat_nonzero.at<Point>(i));
				}
			}
			_pacman.clear();
			for(int i=0;i<3;++i){
				addPacMan(false);			
				addPacMan(true);			
			}
			break;
		case SCAN:
			_anim_scan.restart();
			break;
		case BLOB_SELECT:
			_anim_select.restart();

			_selected.clear();
			_not_selected.clear();
			for(auto b:_collect_blob) _not_selected.push_back(b);
			
			random_shuffle(_not_selected.begin(),_not_selected.end());
			selectBlob();

			break;
		default:
			break;
	}
	_effect=set_;
}


Blob ofApp::contourApproxBlob(vector<Point>& contour_){
	Blob b;
	vector<Point> poly_;
	Point2f center_;
	Rect bounding_;
	float rad_;

	cv::approxPolyDP(Mat(contour_),poly_,2,false);
	bounding_=cv::boundingRect(Mat(poly_));
	minEnclosingCircle(poly_,center_,rad_);

	b._contours=contour_;
	b._center=center_;
	b._bounding=bounding_;
	b._rad=rad_;
	b._npts=contour_.size();

	return b;
}


Mat ofApp::imageToMat(ofImage& img_){

	return Mat(img_.getHeight(),img_.getWidth(),CV_8UC3,img_.getPixelsRef().getData());

}

ofImage ofApp::matToImage(Mat& mat_){
	
	ofPixels pix_;
	pix_.setFromExternalPixels(mat_.ptr(),mat_.cols,mat_.rows,mat_.channels());
	return ofImage(pix_);
	
}

void ofApp::stretchContrast(Mat& src_, Mat& dst_,int c1_,int c2_){

	int s1=0;
	int s2=255;

	dst_=src_.clone();
	for(int y=0;y<src_.rows;++y){
		for(int x=0;x<src_.cols;++x){			
			for(int c=0;c<3;++c){
				float val=src_.at<Vec3b>(y,x)[c];
				int output;
				if(0<=val && val<=c1_) output=s1/c1_*val;
				else if(c1_<val && val<=c2_) output=((s2-s1)/(c2_-c1_))*(val-c1_)+s1;
				else if(c2_<val && val<=255) output=((255-s2)/(255-c2_))*(val-c2_)+s2;

				dst_.at<Vec3b>(y,x)[c]=output;
			}
		}
	}
}


void ofApp::updatePacMan(PacMan& p_){

	ofVec2f pos_=p_.getPos();

	//find next white
	int next_=-100;	
	vector<int> change_(4);
	change_[0]=1;
	change_[1]=-1;
	change_[2]=2;
	change_[3]=-2;
	
	random_shuffle(change_.begin(),change_.end());

	
	//check original direction first
	if(goodStep(p_,(p_._ghost?PacMan::GDirection[p_._dir]:PacMan::Direction[p_._dir]))){
		next_=p_._dir;		
	}else{	
		

		for(int i=0;i<4;++i){
			int n_=(p_._dir+change_[i]+5)%5;
			if(goodStep(p_,(p_._ghost?PacMan::GDirection[n_]:PacMan::Direction[n_]))){
				next_=n_;
				break;
				//cout<<endl;
			}
						
		}
		//cout<<endl;

	}
	if(next_==-100){	
		p_.restart(findWhiteStart(p_._ghost),3);		

	}else{
		p_.setPos(pos_+(p_._ghost?PacMan::GDirection[next_]:PacMan::Direction[next_])*PACMANVEL);
		p_._dir=next_;
	}	

}
bool ofApp::goodStep(PacMan& p_,ofVec2f dir_){
	int x_,y_;
	ofVec2f pos_=p_.getPos();
	x_=pos_.x+dir_.x*PACMANVEL;
	y_=pos_.y+dir_.y*PACMANVEL;

	return !(x_>=PWIDTH||x_<0 || y_>=PHEIGHT||y_<0) &&
		checkWhite(x_,y_) && !p_.alreadyPass(x_,y_);
}

void ofApp::addPacMan(bool ghost_){

	if(_pacman.size()<MAXPACMAN){
		ofVec2f p=findWhiteStart(ghost_);
		_pacman.push_back(PacMan(p.x,p.y,ghost_));		
	}
}


bool ofApp::checkWhite(int x_,int y_){
	/*if(x_<0 || x_>=_mat_thres.cols || y_<0 || y_>=_mat_thres.rows) return false;
 	return _mat_thres.at<uchar>(y_,x_)>0;*/

	return std::find(_nonzero_point.begin(),_nonzero_point.end(),Point(x_,y_))!=_nonzero_point.end();

}

ofVec2f ofApp::findWhiteStart(bool ghost_){
	
	if(!ghost_){
		int len_=_nonzero_start.size();
		int i_=floor(ofRandom(len_));

		return ofVec2f(_nonzero_start[i_].x,_nonzero_start[i_].y);
	}else{
		int len_=_nonzero_gstart.size();
		int i_=floor(ofRandom(len_));

		return ofVec2f(_nonzero_gstart[i_].x,_nonzero_gstart[i_].y);
	}

	/*if(_collect_blob.size()<1) return ofVec2f(0,0);

	int i=(int)ofRandom(_collect_blob.size());
	int j=(int)ofRandom(_collect_blob[i]._blob._contours.size());
	ofVec2f p(_collect_blob[i]._blob._contours[j].x,_collect_blob[i]._blob._contours[j].y);
	
	int d=10;
	for(int i=-d;i<d;++i)
		for(int j=-d;j<d;++j){
			int x=ofClamp(p.x+i,0,PWIDTH);
			int y=ofClamp(p.y+j,0,PHEIGHT);
			if(checkWhite(x,y)){
				return p;
			}
		}
	return p;*/
}

void ofApp::selectBlob(){

	
	/*random_shuffle(_collect_blob.begin(),_collect_blob.end());
	for(int i=0;i<_mselect_blob;++i){
		_collect_blob[i].setTrigger(true);
	}*/
	DetectBlob b=_not_selected[0];
	DetectBlob::_SelectStart=ofVec2f(b._blob._center.x,b._blob._center.y);

	_not_selected.erase(_not_selected.begin());
	
	b.setTrigger(true);
	_selected.push_back(b);
	
	std::sort(_not_selected.begin(),_not_selected.end());
	
	
}


void ofApp::triggerSound(bool short_){
	vector<int> p_;
	if(short_){
		p_.push_back(2);
		p_.push_back(floor(ofRandom(6)));
		sendOSC("/live/play/clip",p_);
	}else{
		p_.push_back(3);
		p_.push_back(0);
		sendOSC("/live/play/clip",p_);
	}


}


void ofApp::receiveOSC(){
	while(_osc_receiver.hasWaitingMessages()){
		// get the next message
		ofxOscMessage m;
		_osc_receiver.getNextMessage(m);
		cout<<"recieve message: "<<m.getAddress()<<" ";
		for(int i=0;i<m.getNumArgs();++i)
			cout<<m.getArgAsInt(i)<<" ";
		cout<<endl;
	}

}