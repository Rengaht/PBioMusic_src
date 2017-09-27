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
    _mat_grab=cv::Mat(PHEIGHT,PWIDTH,CV_8UC3);
#elif defined(USE_REF)
	_img_ref.load("ref2.jpg");
    _mat_grab=cv::Mat(PHEIGHT,PWIDTH,CV_8UC3);
#else
    _camera.listDevices();
    _camera.setDeviceID(1);
	_camera.setVerbose(true);
	_camera.setup(PWIDTH,PHEIGHT);
    _mat_grab=cv::Mat(PHEIGHT,PWIDTH,CV_8UC3);
#endif

	
    _mat_scale==cv::Mat(PHEIGHT,PWIDTH,CV_8UC3);
    _mat_resize=cv::Mat(PHEIGHT,PHEIGHT,CV_8UC3);
    _mat_gray=cv::Mat(PHEIGHT,PHEIGHT,CV_8UC1);
    _mat_normalize=cv::Mat(PHEIGHT,PHEIGHT,CV_8UC1);
    _mat_thres=cv::Mat(PHEIGHT,PHEIGHT,CV_8UC1);
    _mat_edge=cv::Mat(PHEIGHT,PHEIGHT,CV_8UC1);

    _fbo_pacman.allocate(PHEIGHT,PHEIGHT,GL_RGBA);
    
    
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
    
    
    _hist_size=256;
    _do_fft=false;

	//setup spout
#ifdef _WIN64
	_frame_sender.init("_MicroMusic_Sender");
#elif TARGET_OS_MAC
    _frame_sender.setName("_MicroMusic_Sender");
#endif
    
    
    _img_mask.load("mask.png");
    _font.load("Menlo.ttc",7);
    

    
    _serial.listDevices();
    //_serial.setup(_param->_serial_port,9600);
    _serial.setup(0,9600);
}
                               
//--------------------------------------------------------------
void ofApp::update(){
	ofBackground(0);
    
    _report1.str("");
    _report2.str("");

	if(_debug){
		bool new_=updateSource();
		if(new_) cvProcess(_mat_resize);
		return;
	}

	_dmillis=ofClamp(ofGetElapsedTimeMillis()-_last_millis,0,1000.0/60.0);
	_last_millis+=_dmillis;
    cvAnalysis(_mat_resize);
    
    receiveOSC();
    updateSerial();

    

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
				
                    for(auto& p:_pacman){
                        p.update(_dmillis);
                        updatePacMan(p);
                        if(p.isDead()) p.restart(findWhiteStart(p._ghost),3);
                    }
                    checkPacManCollide();
                    
                    _fbo_pacman.begin();
                        ofPushStyle();
                        ofSetColor(0,10);
                        ofFill();
                            ofDrawRectangle(0, 0, PHEIGHT, PHEIGHT);
                        ofPopStyle();
                        for(auto& p:_pacman) p.draw();
                    _fbo_pacman.end();
					break;
				case BLOB_SELECT:
					_anim_select.update(_dmillis);
					if(_anim_select.val()>=1){						
                        if(_not_selected.size()>0){
                            float char_=selectBlob();
                            //remoteVolume(0, char_);
                            triggerSound(false);
                        }else{
                            setEffect(DEFFECT::BLOB_SELECT);
                        }
						_anim_select.restart();
					}
					for(auto& b:_selected) b.update(_dmillis);
					break;
			}
			break;


	}
	

	
}



//--------------------------------------------------------------
void ofApp::draw(){
	// draw the incoming, the grayscale, the bg and the thresholded difference
	ofSetHexColor(0xffffff);
	if(_debug){
		_img_resize.draw(0,0,PWIDTH,PHEIGHT);
		_img_normalize.draw(PWIDTH,0,PWIDTH,PHEIGHT);
		_img_edge.draw(0,PHEIGHT,PWIDTH,PHEIGHT);
		_img_thres.draw(PWIDTH,PHEIGHT,PWIDTH,PHEIGHT);
		
		for(auto& b:_collect_blob) b.drawDebug();


		return;
	}
	
	
    float ratio_=(float)VHEIGHT/PHEIGHT;
	
	float rw_=_anim_detect.val()*PHEIGHT;
	
	int len_;

	ofPushMatrix();
    ofTranslate(ofGetWidth()/2-VHEIGHT/2,ofGetHeight()/2-VHEIGHT/2);
    
    ofScale(ratio_,ratio_);
    
    
	switch(_mode){
		case RUN:
			_report1<<"mode:run"<<endl;
            _report2<<"compute histogram"<<endl;
            _img_resize.draw(0,0,PHEIGHT,PHEIGHT);
            break;
		case DETECT:
			_report1<<"mode:detect"<<endl
					 <<"mblob="<<_collect_blob.size()<<endl;
            switch(_idetect_view){
				case 0:
                    _report2<<"resize image"<<endl;
                    _img_resize.draw(0,0,PHEIGHT,PHEIGHT);
					_img_normalize.drawSubsection(0,0,rw_,PHEIGHT,0,0);
					break;
				case 1:
                    _report2<<"resize image"<<endl
                            <<"normalize image"<<endl;
					_img_normalize.draw(0,0,PHEIGHT,PHEIGHT);
					_img_thres.drawSubsection(0,0,rw_,PHEIGHT,0,0);
					break;
				case 2:
                    _report2<<"resize image"<<endl
                            <<"normalize image"<<endl
                            <<"adaptive threshold"<<endl;
                    _img_thres.draw(0,0,PHEIGHT,PHEIGHT);
					_img_edge.drawSubsection(0,0,rw_,PHEIGHT,0,0);
					break;
				case 3:
                    _report2<<"resize image"<<endl
                            <<"normalize image"<<endl
                            <<"adaptive threshold"<<endl
                            <<"find edge"<<endl;
                    ofPushStyle();
					ofSetColor(255,255.0*(1-_anim_detect.val()));
					_img_edge.draw(0,0,PHEIGHT,PHEIGHT);
					ofPopStyle();

					ofPushMatrix();
						for(auto& b:_collect_blob) b.draw(_anim_detect.val(),false,_font);
					ofPopMatrix();
					break;
				case 4:
                    _report2<<"resize image"<<endl
                        <<"normalize image"<<endl
                        <<"adaptive threshold"<<endl
                        <<"find edge"<<endl
                        <<"apply effect"<<endl;
                    ofPushStyle();
					ofSetColor(255,255.0*_anim_detect.val());

					if(_next_effect==DEFFECT::EDGE_WALK) _img_edge.draw(0,0,PHEIGHT,PHEIGHT);
					else{
						_img_resize.draw(0,0,PHEIGHT,PHEIGHT);
					}
					ofPopStyle();

					ofPushMatrix();
						for(auto& b:_collect_blob) b.draw(1.0,false,_font);
					ofPopMatrix();

					break;
			}

			break;
		case EFFECT:
			_report1<<"mode:effect"<<endl;


			switch(_effect){
			case SCAN:        
				_img_resize.draw(0,0,PHEIGHT,PHEIGHT);
				_report1<<"effect:scan"<<endl;
                _report1<<"mtrigger="<<endl;
                    for(auto& b:_collect_blob){
                        b.draw(1.0,false,_font);
                    }
				ofPushStyle();
				ofSetColor(255,0,0,255);
				ofNoFill();
                ofSetLineWidth(2);
                    
				if(_scan_dir==SCANDIR::VERT){
//					float h_=(float)PHEIGHT/_param->_mscan_region;
//					for(int i=0;i<_param->_mscan_region;++i){
//						if(_scan_touched[i]) ofSetColor(255,0,0);
//						else ofSetColor(255);
//
//						ofDrawLine(_anim_scan.val()*PHEIGHT,i*h_,_anim_scan.val()*PHEIGHT,(i+1)*h_);
//					}
                    ofDrawLine(_anim_scan.val()*PHEIGHT,0,_anim_scan.val()*PHEIGHT,PHEIGHT);
                    
				}else if(_scan_dir==SCANDIR::RADIAL){
					float rad_=_anim_scan.val()*max(PHEIGHT,PHEIGHT)/2;
//					ofPolyline arc_;
//					arc_.arc(0,0,rad_,rad_,0,360.0/_param->_mscan_region,100);				
//
//					for(int i=0;i<_param->_mscan_region;++i){
//						ofPushMatrix();
//						ofTranslate(PHEIGHT/2,PHEIGHT/2);
//						ofRotate(180.0+360.0/_param->_mscan_region*i);
//						if(_scan_touched[i]) ofSetColor(255,0,0);
//						else ofSetColor(255);
//						arc_.draw();
//						ofPopMatrix();
//					}
                    ofPolyline arc_;
                    arc_.arc(0,0,rad_,rad_,0,360.0,100);
                    ofPushMatrix();
                    ofTranslate(PHEIGHT/2,PHEIGHT/2);
                    arc_.draw();
                    ofPopMatrix();
				}
				ofPopStyle();
				break;
			case EDGE_WALK:
				_img_thres.draw(0,0,PHEIGHT,PHEIGHT);
				//for(auto& b:_collect_blob) b.draw(1.0,false);
                _fbo_pacman.draw(0,0);
                    
				_report1<<"effect:pacman"<<endl;
                _report1<<"mpac="<<_pacman.size()<<endl;
				
                    
                break;
			case BLOB_SELECT:
				_img_resize.draw(0,0,PHEIGHT,PHEIGHT);
				len_=_selected.size();
				if(len_>1){
					for(int i=0;i<len_-1;++i){
						_selected[i].draw(1.0,true,_font);
					}
					_selected[len_-1].draw(_anim_select.val(),true,_font);
				}

				_report1<<"effect:blob"<<endl;
                _report1<<"selected="<<_selected[len_-1]._blob._bounding.area()<<endl;
				break;
			default:
				break;
			}

		
			break;
	}
	ofPopMatrix();


    
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2-150,560);
    ofSetHexColor(0xffffff);
	
	_report1<<"fps=" << ofGetFrameRate()<<endl;
    
    
    //ofDrawBitmapString(reportStr.str(),0,0);
    if(_mode==MODE::RUN) drawAnalysis(-50,0,PWIDTH*.7,50);
    
    ofScale(ratio_,ratio_);
    
    _font.drawString(_report1.str(),0,0);
    _font.drawString(_report2.str(),60,-6);
    
    
    ofPopMatrix();
    
    ofPushMatrix();
    //ofSetColor(255,20);
    _img_mask.draw(0,0);
    ofPopMatrix();
    
    //send spout
#ifdef _WIN64
    _sender_image.grabScreen(0,0,ofGetWidth(),ofGetHeight());
    _frame_sender.send(_sender_image.getTexture());
#elif TARGET_OS_MAC
    _frame_sender.publishScreen();
#endif
    
    
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
        _mat_grab=cv::Mat(_video.getHeight(),_video.getWidth(),CV_8UC3,_video.getPixelsRef().getData());
#elif defined(USE_REF)
        _mat_grab=cv::Mat(_img_ref.getHeight(),_img_ref.getWidth(),CV_8UC3,_img_ref.getPixelsRef().getData());
#else			
        _mat_grab=cv::Mat(_camera.getHeight(),_camera.getWidth(),CV_8UC3,_camera.getPixelsRef().getData());
#endif  
	}
    
    _mat_scale=_mat_grab(cv::Rect(PWIDTH/2-PHEIGHT/2,0,PHEIGHT,PHEIGHT));
    cv::resize(_mat_scale,_mat_resize,cv::Size(PHEIGHT,PHEIGHT));
	
    _img_resize=matToImage(_mat_resize);


	return bNewFrame;
}

// for main opencv processing
void ofApp::cvProcess(cv::Mat& grab_){


	//stretchContrast(_mat_resize,_mat_contrast,_param->_contrast_low,_param->_contrast_high);

	cv::cvtColor(grab_,_mat_gray,CV_BGR2GRAY);
    cv::blur(_mat_gray,_mat_gray,cv::Size(3,3));

	//cv::equalizeHist(_mat_gray,_mat_normalize);
	cv::normalize(_mat_gray,_mat_normalize,0,255,CV_MINMAX);
	/*auto clahe=cv::createCLAHE();
	clahe->apply(_mat_gray,_mat_normalize);*/

	//cv::threshold(_mat_gray,_mat_thres,_param->_bi_thres,255,THRESH_BINARY_INV);
    cv::adaptiveThreshold(_mat_normalize,_mat_thres,255,cv::ADAPTIVE_THRESH_GAUSSIAN_C,cv::THRESH_BINARY_INV,33,0);


	float area_=PWIDTH*PHEIGHT;

	cv::Canny(_mat_thres, _mat_edge,50,150,3); 	
	_contours.clear();
	_hierarchy.clear();

    cv::Mat copy_;
	_mat_thres.copyTo(copy_);

    cv::findContours(copy_,_contours,_hierarchy,CV_RETR_CCOMP,cv::CHAIN_APPROX_SIMPLE,cv::Point(0,0));
	updateBlob(_contours,_hierarchy);
    

	
	_img_gray=matToImage(_mat_gray);
	_img_thres=matToImage(_mat_thres);
	_img_edge=matToImage(_mat_edge);
	_img_normalize=matToImage(_mat_normalize);


}

void ofApp::cvAnalysis(cv::Mat& grab_){
    
    float range[]={0, 255};
    const float* histRange={range};
    int channels_[]={0};
    cv::calcHist(&grab_,1,channels_,cv::Mat(),_hist,1,&_hist_size,&histRange);
    
    if(!_do_fft) return;
    cv::cvtColor(grab_,_mat_gray,CV_BGR2GRAY);
    cv::Mat padded;
    int m = cv::getOptimalDFTSize(grab_.rows);
    int n = cv::getOptimalDFTSize(grab_.cols);
    cv::copyMakeBorder(_mat_gray, padded, 0, m-grab_.rows, 0, n-grab_.cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));
    
    cv::Mat planes[] = {cv::Mat_<float>(padded), cv::Mat::zeros(padded.size(), CV_32F)};
    cv::Mat complexImg;
    cv::merge(planes, 2, complexImg);
    cv::dft(complexImg, complexImg);

    
    
    split(complexImg, planes);
    magnitude(planes[0], planes[1], planes[0]);
    _mat_fft= planes[0];
    _mat_fft += cv::Scalar::all(1);
    log(_mat_fft, _mat_fft);
    
    _mat_fft=_mat_fft(cv::Rect(0, 0, _mat_fft.cols & -2, _mat_fft.rows & -2));
    
    normalize(_mat_fft, _mat_fft, 0, 1, CV_MINMAX);

    
}

void ofApp::drawAnalysis(float x_,float y_,float wid_,float hei_){
    
    float skip_=80;
    float mplot_=50;
    
    float histwid_=wid_/skip_;
    
    double maxval_;
    cv::minMaxLoc(_hist,0,&maxval_,0,0);
    _report2<<"hist max="<<maxval_<<endl;
    
    ofPushMatrix();
    ofTranslate(x_,y_+hei_/2);
    ofPushStyle();
    ofSetColor(255,120);
    ofNoFill();
    //ofSetLineWidth(histwid_);
    
    ofBeginShape();
    for(int i=0;i<skip_;++i){
        float binVal=_hist.at<float>((int)(i+ofGetFrameNum()+skip_)%(int)(_hist_size-skip_*2));
        float intensity=round(binVal*hei_/maxval_);
        float x=(i)*histwid_;
        
        ofVertex(x,-intensity);
        ofPushStyle();
        ofFill();
        ofDrawCircle(x,-intensity,1);
        ofPopStyle();
    }
    ofEndShape();
//    for(int i=skip_;i<_hist_size-skip_;++i){
//        float binVal=_hist.at<float>(i);
//        float intensity=round(binVal*hei_/maxval_);
//        ofVertex((i-skip_)*histwid_,-intensity);
//    }
    
    if(_do_fft){
    double fftmax_;
    cv::minMaxLoc(_mat_fft,0,&fftmax_,0,0);
    _report2<<"calc dft"<<endl;
    
    float fwid_=histwid_*2.0;
    ofTranslate(-wid_/6,hei_*3);
    //ofRotate(-90);
    ofSetColor(255,120);
    ofNoFill();
    //ofSetLineWidth(histwid_);
    int fr_=ofGetFrameNum();
    int col_=_mat_fft.cols;
    int row_=_mat_fft.rows;
    
    for(int i=0;i<mplot_;++i){
        
        ofPushMatrix();
//        ofTranslate(sin(_mat_fft.at<float>(i,col_-1))*wid_*.5,-hei_*.3*i);
          ofTranslate(0,-hei_*.3*i);
//        ofBeginShape();
           
            float step_=2;
            float avb_=0;
            for(int j=0;j<mplot_;j++){
                float binVal=_mat_fft.at<float>(i,(int)(j)%col_);
                float intensity=round(binVal*3*hei_/fftmax_);
//                ofVertex(j*histwid_,-intensity);
                
                ofPushStyle();
                ofFill();
                ofDrawCircle(j*fwid_,-intensity,1);
                ofPopStyle();
                avb_+=binVal;
            }
           // if(ofNoise(avb_)<.33) _font.drawString(ofToString(avb_/mplot_),wid_/2,0);
        ofPopMatrix();
//        ofEndShape();
    }
    }
    
    ofPopStyle();
    ofPopMatrix();
}


void ofApp::updateBlob(vector<vector<cv::Point>>& contour_,vector<cv::Vec4i>& hierachy_){
    
    // approx contour
    int clen=contour_.size();
    vector<Blob> blob_;
    float frame_=PHEIGHT*PHEIGHT;
    
    for(int i=0;i<clen;++i){
        Blob b_=contourApproxBlob(contour_[i]);
        float d_=ofDist(b_._center.x, b_._center.y,PHEIGHT/2,PHEIGHT/2);
        if(d_>PHEIGHT/2) continue;
        
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
    
    float spos_=(_scan_dir==SCANDIR::VERT)?_anim_scan.val()*PHEIGHT:_anim_scan.val()*min(PHEIGHT,PHEIGHT)/2;
    float dist=0;
    float ang=0;
    
    switch(_scan_dir){
        case VERT:
            if(spos_>=detect_._blob._bounding.x && spos_<detect_._blob._bounding.x+detect_._blob._bounding.width) return true;
            break;
        case RADIAL:
            dist=ofDist(detect_._blob._center.x,detect_._blob._center.y,PHEIGHT/2,PHEIGHT/2);
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
            //sendOSC("/live/tempo",vector<float>());
			triggerSound(true);
			break;
		case 'l':
			triggerSound(false);
			break;
        case 'f':
            _do_fft=!_do_fft;
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

void ofApp::sendOSC(string address_,vector<float> param_){

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
    
    int mtrigger=0;
    int trigger_area=0;
    
	for(auto& b:_collect_blob){
		
        switch(_scan_dir){
            case VERT:
                if(isScanned(b)){
					int pos=int(floor(b._blob._center.y/epos_));
					pos=ofClamp(pos,0,_scan_touched.size()-1);
					_scan_touched[pos]=true;
					b.setTrigger(true);
                    mtrigger++;
                    trigger_area+=b._blob._bounding.area();
                    
				}else b.setTrigger(false);
                break;
            case RADIAL:
                ang=atan2(b._blob._center.y-PHEIGHT/2,b._blob._center.x-PWIDTH/2)+PI;				
                if(isScanned(b)){
					int region_=(int)(floor(ang/TWO_PI*_param->_mscan_region));
					region_=ofClamp(region_,0,_scan_touched.size()-1);
					_scan_touched[region_]=true;
					b.setTrigger(true);
                    mtrigger++;
                    trigger_area+=b._blob._bounding.area();
                    
                    
				}else b.setTrigger(false);
                break;
            
        }
    }
    _report2<<"mtrigger="<<mtrigger<<endl
            <<"cover area="<<trigger_area<<endl;
    
    
    //if(ofGetFrameNum()%60==0)
        triggerScan(mtrigger,trigger_area);
    
    
    
    
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
                
                cv::Point p_=_mat_nonzero.at<cv::Point>(i);
                
                _nonzero_point.push_back(p_);
                
                if(ofDist(p_.x,p_.y,PHEIGHT/2,PHEIGHT/2)<PHEIGHT/2){
                    if(i<len_/10.0){
                        _nonzero_start.push_back(p_);
                    }else if(i>len_/10.0*9){
                        _nonzero_gstart.push_back(p_);
                    }
                }
			}
			_pacman.clear();
			for(int i=0;i<3;++i){
				addPacMan(false);			
				addPacMan(true);			
			}
            _fbo_pacman.begin();
                ofBackground(0,0);
            _fbo_pacman.end();
            
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


Blob ofApp::contourApproxBlob(vector<cv::Point>& contour_){
	Blob b;
    vector<cv::Point> poly_;
    cv::Point2f center_;
    cv::Rect bounding_;
	float rad_;

    cv::approxPolyDP(cv::Mat(contour_),poly_,2,false);
    bounding_=cv::boundingRect(cv::Mat(poly_));
	minEnclosingCircle(poly_,center_,rad_);

	b._contours=contour_;
	b._center=center_;
	b._bounding=bounding_;
	b._rad=rad_;
	b._npts=contour_.size();

	return b;
}


cv::Mat ofApp::imageToMat(ofImage& img_){

    return cv::Mat(img_.getHeight(),img_.getWidth(),CV_8UC3,img_.getPixels().getData());

}

ofImage ofApp::matToImage(cv::Mat& mat_){
	
	ofPixels pix_;
	pix_.setFromExternalPixels(mat_.ptr(),mat_.cols,mat_.rows,mat_.channels());
	return ofImage(pix_);
	
}

void ofApp::stretchContrast(cv::Mat& src_, cv::Mat& dst_,int c1_,int c2_){

	int s1=0;
	int s2=255;

	dst_=src_.clone();
	for(int y=0;y<src_.rows;++y){
		for(int x=0;x<src_.cols;++x){			
			for(int c=0;c<3;++c){
                float val=src_.at<cv::Vec3b>(y,x)[c];
				int output;
				if(0<=val && val<=c1_) output=s1/c1_*val;
				else if(c1_<val && val<=c2_) output=((s2-s1)/(c2_-c1_))*(val-c1_)+s1;
				else if(c2_<val && val<=255) output=((255-s2)/(255-c2_))*(val-c2_)+s2;

                dst_.at<cv::Vec3b>(y,x)[c]=output;
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
		//p_.restart(findWhiteStart(p_._ghost),3);
        p_.goDie();
        triggerDead();
        
	}else{
		p_.setPos(pos_+(p_._ghost?PacMan::GDirection[next_]:PacMan::Direction[next_])*PACMANVEL);
		p_._dir=next_;
        
        //if(abs(next_)>=2) triggerTurn();
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

    return std::find(_nonzero_point.begin(),_nonzero_point.end(),cv::Point(x_,y_))!=_nonzero_point.end();

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

void ofApp::checkPacManCollide(){
    int len_=_pacman.size();
    for(int i=0;i<len_;++i){
        if(_pacman[i].isDead()) continue;
            
        for(int j=i+1;j<len_;++j){
            if(_pacman[j].isDead()) continue;
            if(_pacman[i].getPos().distance(_pacman[j].getPos())<PacMan::Rad){
                _pacman[i].goDie();
                _pacman[j].goDie();
                
                if(_pacman[i]._ghost || _pacman[j]._ghost) triggerCollide(true);
                else triggerCollide(false);
            }
            
        }
    }
}


float ofApp::selectBlob(){

	
	/*random_shuffle(_collect_blob.begin(),_collect_blob.end());
	for(int i=0;i<_mselect_blob;++i){
		_collect_blob[i].setTrigger(true);
	}*/
    if(_not_selected.size()<1) return 0;
    
	DetectBlob b=_not_selected[0];
	DetectBlob::_SelectStart=ofVec2f(b._blob._center.x,b._blob._center.y);

	_not_selected.erase(_not_selected.begin());
	
    float frame_=PWIDTH*PHEIGHT;
    float char_=ofMap(b._blob._bounding.area(),_param->_blob_small,_param->_blob_large*frame_,.3,5.0);
    
	b.setTrigger(true);
	_selected.push_back(b);
	
	std::sort(_not_selected.begin(),_not_selected.end());
	
    return char_;
}

#pragma mark LIVE_TRIGGER

void ofApp::triggerSound(bool short_){
	vector<float> p_;
//	if(short_){
//		p_.push_back(0);
//		p_.push_back(floor(ofRandom(7)));
//		sendOSC("/live/play/clip",p_);
//	}else{
//		p_.push_back(1);
//		p_.push_back(floor(ofRandom(13)));
//		sendOSC("/live/play/clip",p_);
//	}
    p_.push_back(0);
    p_.push_back(floor(ofRandom(6)));
    sendOSC("/live/play/clip",p_);

}

void ofApp::remoteVolume(int track_,float vol_){
    
    vector<float> p_;
    p_.push_back(track_);
    p_.push_back(vol_);
    sendOSC("/live/volume",p_);
    
}

void ofApp::triggerTurn(){
    triggerSound(true);
}
void ofApp::triggerCollide(bool ghost_){
    triggerSound(false);
}
void ofApp::triggerDead(){
    triggerSound(false);
}

void ofApp::triggerScan(int num_,float area_){
   // remoteVolume(2,area_/(PHEIGHT*PHEIGHT*_param->_blob_large));
    for(int i=0;i<ofClamp(num_,0,2);++i) triggerSound(true);
}

#pragma mark COMMUNICATION
void ofApp::receiveOSC(){
	while(_osc_receiver.hasWaitingMessages()){
		// get the next message
		ofxOscMessage m;
		_osc_receiver.getNextMessage(m);
		cout<<"recieve message: "<<m.getAddress()<<" ";
        for(int i=0;i<m.getNumArgs();++i){
            auto t_=m.getArgType(i);
            if(t_==OFXOSC_TYPE_INT32) cout<<m.getArgAsInt(i)<<" ";
            else if(t_==OFXOSC_TYPE_FLOAT) cout<<m.getArgAsFloat(i)<<" ";

        }
		cout<<endl;
	}

}

void ofApp::updateSerial(){
    
    if(_serial.isInitialized() && _serial.available()){
        vector<string> val=readSerialString(_serial,'#');
        if(val.size()<1) return -1;
        //ofLog()<<"serial read: "<<ofToString(val)<<endl;
        
        
        if(val[0]=="set_0"){
            if(_mode!=MODE::EFFECT || _effect!=DEFFECT::SCAN){
                _next_effect=DEFFECT::SCAN;
                setMode(MODE::DETECT);
            }else{
                setMode(MODE::RUN);
            }
        }else if(val[0]=="set_1"){
            if(_mode!=MODE::EFFECT || _effect!=DEFFECT::BLOB_SELECT){
                _next_effect=DEFFECT::BLOB_SELECT;
                setMode(MODE::DETECT);
            }else{
                setMode(MODE::RUN);
            }
        }else if(val[0]=="set_2"){
            if(_mode!=MODE::EFFECT || _effect!=DEFFECT::EDGE_WALK){
                _next_effect=DEFFECT::EDGE_WALK;
                setMode(MODE::DETECT);
            }else{
                setMode(MODE::RUN);
            }
        }
        
        // motor
        if(val[0]=="motor_clock"){
            
        }else if(val[0]=="motor_reverse"){
            
        }
        
        // effect tune
        if(_mode!=MODE::EFFECT) return;
        if(_effect==DEFFECT::EDGE_WALK){
            if(val[0]=="add_pac"){
                addPacMan(false);
            }else if(val[0]=="add_ghost"){
                addPacMan(true);
            }
        }
        if(_effect==DEFFECT::BLOB_SELECT){
            if(val[0]=="blob_reset"){
                setEffect(DEFFECT::BLOB_SELECT);
            }else if(val[0]=="speed_b"){
                float v=ofToFloat(val[1]);
                _anim_select.setDue(_param->_select_vel*(1.0+v/255.0*4.0));
            }
        }
        if(_effect==DEFFECT::SCAN){
            if(val[0]=="scan_line"){
                _scan_dir=SCANDIR::VERT;
            }else if(val[0]=="scan_radial"){
                _scan_dir=SCANDIR::RADIAL;
            }else if(val[0]=="speed_a"){
                float v=ofToFloat(val[1]);
                _anim_scan.setDue(_param->_scan_vel*(1.0+v/255.0*4.0));
            }
        }
        
    
    }
    
    
}







