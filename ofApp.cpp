#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	
    ofEnableSmoothing();
	ofSetVerticalSync(true);

	_param=new GlobalParam();
    
#if defined(USE_VIDEO)
    _video.load("ref/5.mp4");
    _video.play();
    _video.setLoopState(OF_LOOP_NONE);
    _video.setVolume(0);
	ofLog()<<"movie loaded ="<<_video.isLoaded()<<" "<<_video.getWidth()<<"x"<<_video.getHeight();
	
    _mat_grab=cv::Mat(PHEIGHT,PWIDTH,CV_8UC3);
#elif defined(USE_REF)
	_img_ref.load("onion.jpg");
    _mat_grab=cv::Mat(PHEIGHT,PWIDTH,CV_8UC3);
#else
    _camera.listDevices();
    _camera.setDeviceID(1);
	_camera.setVerbose(true);
	_camera.setup(PWIDTH,PHEIGHT);
    _mat_grab=cv::Mat(PHEIGHT,PWIDTH,CV_8UC3);
#endif

	
    _mat_scale==cv::Mat(VHEIGHT,VWIDTH,CV_8UC3);
    _mat_resize=cv::Mat(VHEIGHT,VWIDTH,CV_8UC3);
    _mat_contrast=cv::Mat(VHEIGHT,VWIDTH,CV_8UC3);

    _mat_gray=cv::Mat(VHEIGHT,VWIDTH,CV_8UC1);
    _mat_normalize=cv::Mat(VHEIGHT,VWIDTH,CV_8UC1);
    _mat_thres=cv::Mat(VHEIGHT,VWIDTH,CV_8UC1);
    _mat_edge=cv::Mat(VHEIGHT,VWIDTH,CV_8UC1);
    _mat_morph=cv::Mat(VHEIGHT,VWIDTH,CV_8UC1);
    _mat_pacman=cv::Mat(VHEIGHT,VWIDTH,CV_8UC3);

    _fbo_pacman.allocate(VWIDTH,VHEIGHT,GL_RGBA);
    
    
	_osc_sender.setup(_param->_live_addr,9000);
	_osc_receiver.setup(9001);
    

	_anim_detect=FrameTimer(250);

	setMode(MODE::RUN);

	_anim_scan=FrameTimer(_param->_scan_vel);
//	_anim_scan.setContinuous(true);

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
#ifdef TARGET_OS_MAC
    _font.load("Menlo.ttc",7);
#else
	_font.load("verdana.ttf",7);
#endif

    //_img_pac.load("texture.png");
    
    
    
    _serial.listDevices();
	if(_serial.getDeviceList().size()>0) _serial.setup(0,19200);
    
    

	//init bird
	for(int i=0;i<MBIRDREGION;++i)
		for(int j=0;j<MBIRDREGION;++j) _region_bird.push_back(vector<DetectBlob>());

    //_light.setPosition(1000, 1000, 2000);
    //
    //_postprocess.init(ofGetWidth(), ofGetHeight());
    //_postprocess.createPass<FxaaPass>()->setEnabled(false);
    //_postprocess.createPass<BloomPass>()->setEnabled(false);
    //_postprocess.createPass<DofPass>()->setEnabled(false);
    //_postprocess.createPass<KaleidoscopePass>()->setEnabled(false);
    //_postprocess.createPass<NoiseWarpPass>()->setEnabled(false);
    //_postprocess.createPass<PixelatePass>()->setEnabled(false);
    //_postprocess.createPass<EdgePass>()->setEnabled(false);
    //_postprocess.createPass<VerticalTiltShifPass>()->setEnabled(false);
    //_postprocess.createPass<GodRaysPass>()->setEnabled(false);
    
    _ipost=0;
}
                               
//--------------------------------------------------------------
void ofApp::update(){
	
    ofBackground(0);
    
    _report1.str("");
    _report2.str("");

	if(_debug){
		bool new_=updateSource();
		if(new_) cvProcess(_mat_contrast);
		return;
	}

	_dmillis=ofClamp(ofGetElapsedTimeMillis()-_last_millis,0,1000.0/60.0);
	_last_millis+=_dmillis;
    cvAnalysis(_mat_resize);
    
    //receiveOSC();
    updateSerial();

    int len;
    bool alldead_;
    ofVec2f bdir_;
    
	bool new_=false;
    
	float bwid_,bhei_;
    
	
	switch(_mode){
		case RUN:
			new_=updateSource();
          	break;
		case DETECT:
			new_=updateSource();
			// something for drawing
			_anim_detect.update(_dmillis);
		
			if(_anim_detect.val()>=1){
				_idetect_view++;
				
				if(_idetect_view>5){
					setMode(MODE::EFFECT);
					setEffect(_next_effect);
                }else{
                    if(_idetect_view>4) _anim_detect.setDue(500);
                    else _anim_detect.setDue(250);
                    _anim_detect.restart();
                }
			}
			break;
		case EFFECT:
			for(auto& b:_collect_blob) b.update(_dmillis);
			switch(_effect){
				case SCAN:
					new_=updateSource();
					_anim_scan.update(_dmillis);
                    if(_anim_scan.val()>=1){
                        _anim_scan.restart();
                        if(_scan_pos<_scan_range) _scan_pos++;
                        else{
                          _scan_pos=0;
                          for(auto& b:_collect_blob) b.setTrigger(false);
                        }
                    }
                    checkScan(_scan_dir);
					break;
				case EDGE_WALK:
					new_=updateSource();
                    for(auto it=_pacman.begin();it!=_pacman.end();){
                        (*it).update(_dmillis);
                        updatePacMan(*it);
                        
                        if((*it).isDead()){
                            _pacman.erase(it);
                            addPacMan(ofRandom(2)<1);
                        }else{
                            it++;
                        }
                    }
                    //checkPacManCollide();
//                    for(auto& p:_pacman){
//                        cv::Rect comp(0,0,10,10);
//                        int area=cv::floodFill(_mat_pacman,cv::Point(p.getPos().x,p.getPos().y),cv::Scalar(0),&comp,cv::Scalar(20, 20, 20),cv::Scalar(20, 20, 20));
//                        ofLog()<<"flood area: "<<area;
//                        //floodFill(_mat_morph,cv::Point(p.getPos().x,p.getPos().y),cv::Scalar(255,0,0),&comp,cv::Scalar(20, 20, 20),cv::Scalar(20, 20, 20));
//                    }
                    _fbo_pacman.begin();
                        //ofEnableBlendMode(ofBlendMode::);
                        //ofPushStyle();
////                        ofSetColor(0,1);
////                        ofFill();
////                            ofDrawRectangle(0, 0, PHEIGHT, PHEIGHT);
////                        ofPopStyle();
                        for(auto& p:_pacman){
                            p.draw(_img_pac);
                        }
                    _fbo_pacman.end();
                    //_img_pacman=matToImage(_mat_pacman);

					break;
				case BLOB_SELECT:
					new_=updateSource();
//					_anim_select.update(_dmillis);
//					if(_anim_select.val()>=1){						
//                        if(_not_selected.size()>mSelectBlob){
//                            float char_=selectBlob();
//                            _anim_select.setDue(char_*_param->_select_vel);
//                            //remoteVolume(0, char_);
//                            triggerSound();
//                        }else{
//                            setEffect(DEFFECT::BLOB_SELECT);
//                        }
//						_anim_select.restart();
//					}
                    alldead_=true;
					len=_selected.size();
					for(int i=0;i<len;++i){
							_selected[i].update(_dmillis);
							updateSelectSeq(_selected[i]);
							if(!_selected[i]._dead) alldead_=false;
					}
					
                    
                   // if(_selected.size()<3 && ofRandom(30)<1) addSelectKey();
                    
                    len=_not_selected.size();
                    if(len<1){
                        if(alldead_) setEffect(DEFFECT::BLOB_SELECT);
                    }
					break;
                case BIRD:
                    bdir_=ofVec2f(PHEIGHT*.1,0);
                    bdir_.rotate(360.0*abs(sin(ofGetFrameNum()/50.0)));
                    bdir_+=ofVec2f(PWIDTH/2,PHEIGHT/2);
                    DetectBlob::Center=bdir_;

					bwid_=VWIDTH/(float)MBIRDREGION;
					bhei_=VHEIGHT/(float)MBIRDREGION;
                    updateBirdRegion();

                    for(auto& b:_bird){
						
						int rx=ofClamp(floor(b._floc.x/(bwid_)),0,MBIRDREGION-1);
						int ry=ofClamp(floor(b._floc.y/(bhei_)),0,MBIRDREGION-1);

                        b.fupdate(_region_bird[rx*MBIRDREGION+ry],_dmillis);
                        //if(DetectBlob::Center.distance(ofVec2f(b._floc.x,b._floc.y))>PHEIGHT/2*.9){
						if(b._floc.x>VWIDTH*.9 || b._floc.x<VWIDTH*.1
						   || b._floc.y>VHEIGHT*.9 || b._floc.y<VHEIGHT*.1){
                            if(!b.getTrigger()){
                                b.setTrigger(true);
                                triggerSound();
                            }
                        }else{
                            b.setTrigger(false);
                        }
                    }
                    break;
                case BUG:
					new_=updateSource();
                    cvProcess(_mat_contrast);
                    for(auto& box:_bug_box) checkBugTouch(box);
                    break;
			}
			break;

	}
	
	ofSetWindowTitle(ofToString(ofGetFrameRate()));

	
}



//--------------------------------------------------------------
void ofApp::draw(){
	// draw the incoming, the grayscale, the bg and the thresholded difference
	ofSetHexColor(0xffffff);
	if(_debug){
        ofPushMatrix();
        ofScale(ofGetWidth()/3.0/(float)PWIDTH,ofGetWidth()/3.0/(float)PWIDTH);
        
		_img_resize.draw(0,0);
        _img_contrast.draw(PHEIGHT,0);
		_img_normalize.draw(PHEIGHT*2,0);
        _img_thres.draw(0,PHEIGHT);
        _img_morph.draw(PHEIGHT,PHEIGHT);
        _img_edge.draw(PHEIGHT*2,PHEIGHT);
		
		for(auto& b:_collect_blob) b.drawDebug();

        ofPopMatrix();

		return;
	}
	
    //ofEnableDepthTest();
    //_light.enable();
    //            _postprocess.begin();

    
    
    float ratio_=(float)VHEIGHT/PHEIGHT;
	
	float rw_=_anim_detect.val()*PWIDTH;
	
	ofPushMatrix();
   // ofTranslate(ofGetWidth()/2-VWIDTH/2,ofGetHeight()/2-VHEIGHT/2);
    
  //  ofScale(ratio_,ratio_);
    
    
	switch(_mode){
		case RUN:
			_report1<<"mode:run"<<endl;
            _report2<<"compute histogram"<<endl;
  
            _img_contrast.draw(0,0);
            
            
            break;
		case DETECT:
			_report1<<"mode:detect"<<endl
					 <<"mblob="<<_collect_blob.size()<<endl;
            switch(_idetect_view){
				case 0:
                    _report2<<"resize image"<<endl;
                    _img_contrast.draw(0,0);
					_img_normalize.drawSubsection(0,0,rw_,PHEIGHT,0,0);
					break;
				case 1:
                    _report2<<"resize image"<<endl
                            <<"normalize image"<<endl;
					_img_normalize.draw(0,0);
					_img_thres.drawSubsection(0,0,rw_,PHEIGHT,0,0);
					break;
				case 2:
                    _report2<<"resize image"<<endl
                            <<"normalize image"<<endl
                            <<"adaptive threshold"<<endl;
                    _img_thres.draw(0,0);
					_img_morph.drawSubsection(0,0,rw_,PHEIGHT,0,0);
					break;
                case 3:
                    _report2<<"resize image"<<endl
                    <<"normalize image"<<endl
                    <<"adaptive threshold"<<endl
                    <<"morpholgy structure"<<endl;
                    
                    _img_morph.draw(0,0);
                    _img_edge.drawSubsection(0,0,rw_,PHEIGHT,0,0);
                    break;

                case 4:
                    _report2<<"resize image"<<endl
                            <<"normalize image"<<endl
                            <<"adaptive threshold"<<endl
                            <<"morpholgy structure"<<endl
                            <<"find edge"<<endl;
                    ofPushStyle();
					ofSetColor(255,255.0*(1-_anim_detect.val()));
					_img_edge.draw(0,0);
					ofPopStyle();

					ofPushMatrix();
						for(auto& b:_collect_blob) b.drawDetect(.5*_anim_detect.val(),_font);
					ofPopMatrix();
					break;
				case 5:
                    _report2<<"resize image"<<endl
                        <<"normalize image"<<endl
                        <<"adaptive threshold"<<endl
                        <<"morpholgy structure"<<endl
                        <<"find edge"<<endl
                        <<"apply effect"<<endl;
                    ofPushStyle();
					ofSetColor(255,255.0*_anim_detect.val());

                    if(_next_effect==DEFFECT::EDGE_WALK || _next_effect==DEFFECT::BIRD){
                        _img_edge.draw(0,0);
                    }else{
						_img_resize.draw(0,0);
					}
					ofPopStyle();

					ofPushMatrix();
						for(auto& b:_collect_blob) b.drawDetect(.5+.5*_anim_detect.val(),_font);
					ofPopMatrix();

					break;
			}

			break;
		case EFFECT:
			_report1<<"mode:effect"<<endl;


			switch(_effect){
			case SCAN:        
				_img_contrast.draw(0,0);
				_report1<<"effect:scan"<<endl;
                _report1<<"mtrigger="<<endl;
                    for(auto& b:_collect_blob){
                        b.drawScan(1.0,_font);
                    }
//				ofPushStyle();
//                ofSetColor(DetectBlob::BColor[3]);
//				ofNoFill();
//                ofSetLineWidth(5);
                    
//				if(_scan_dir==SCANDIR::VERT){

//                    ofDrawLine(_anim_scan.val()*PHEIGHT,0,_anim_scan.val()*PHEIGHT,PHEIGHT);
//                    float ehei=PHEIGHT*.7/(float)_scan_range;
//                    float ebegin=PHEIGHT*.3;
//                    ofDrawLine(0,(_scan_range-_scan_pos)*ehei-ebegin,_anim_scan.val()*PHEIGHT,(_scan_range-_scan_pos)*ehei-ebegin);
//                    for(int i=0;i<_scan_pos;++i)
//                        ofDrawLine(0,(_scan_range-i)*ehei,PHEIGHT,(_scan_range-i)*ehei);
//                    
                    
//				}else if(_scan_dir==SCANDIR::RADIAL){
//                    ofPushMatrix();
//                    ofTranslate(PHEIGHT/2,PHEIGHT/2);
                    
//                    float eang=360.0/_scan_range;
//					ofVec2f dir(PHEIGHT/2,0);
//                    dir.rotate(eang*_scan_pos);
//                    
//                    ofDrawLine(0,0,dir.x*_anim_scan.val(),dir.y*_anim_scan.val());
//                    
//                    for(int i=0;i<_scan_pos;++i){
//                        ofVec2f dir_(PHEIGHT/2,0);
//                        dir_.rotate(eang*i);
//                        
//                        ofDrawLine(0,0,dir_.x,dir_.y);
//                    }
//                    ofPopMatrix();
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
//                    ofPolyline arc_;
//                    arc_.arc(0,0,rad_,rad_,0,360.0,100);
//                    ofPushMatrix();
//                    ofTranslate(PHEIGHT/2,PHEIGHT/2);
//                    arc_.draw();
//                    ofPopMatrix();
//				}
//				ofPopStyle();
				break;
			case EDGE_WALK:
//				_img_pacman.draw(0,0,PHEIGHT,PHEIGHT);
             //   _img_contrast.draw(0,0,PWIDTH,PHEIGHT);
//                    ofPushStyle();
//                    ofSetColor(255,120);
                _img_morph.draw(0,0);
                   // ofPushStyle();
				//for(auto& b:_collect_blob) b.draw(1.0,false);
                _fbo_pacman.draw(0,0);
//                for(auto& p:_pacman){
//                    p.draw(_img_pac);
//                }
				_report1<<"effect:pacman"<<endl;
                _report1<<"mpac="<<_pacman.size()<<endl;
				
                    
                break;
			case BLOB_SELECT:
				_img_contrast.draw(0,0);
                    for(auto& seq:_selected){
                        seq.draw(_font);
                    }

				_report1<<"effect:blob"<<endl;
                break;
            case BIRD:
                    _img_edge.draw(0,0);
                    for(auto& b:_bird) b.drawBirdBack();
                    for(auto& b:_bird) b.drawBird();
                    break;
            case BUG:
                    _img_contrast.draw(0,0);
                    for(auto& box:_bug_box) box.draw();
                    for(auto& b:_collect_blob) b.drawBug(_font);
                    break;
			default:
				break;
			}

		
			break;
	}
	ofPopMatrix();


    
    ofPushMatrix();
    ofTranslate(80,650);
    ofSetHexColor(0xffffff);
	
	//_report1<<"fps=" << ofGetFrameRate()<<endl;
    
    
    //ofDrawBitmapString(reportStr.str(),0,0);
    if(_mode==MODE::RUN) drawAnalysis(0,0,VWIDTH*.7,50);
    
    ofScale(ratio_,ratio_);
    
    _font.drawString(_report1.str(),0,0);
    _font.drawString(_report2.str(),60,-6);
    
    
    ofPopMatrix();
    
//            _postprocess.end();

    
    //if(_img_mask.bAllocated()) _img_mask.draw(0,0);
    
    
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
    
    _mat_resize=_mat_grab(cv::Rect(0,0,VWIDTH,VHEIGHT));
	//cv::resize(_mat_scale,_mat_resize,cv::Size(PWIDTH,PHEIGHT));
	//cv::resize(_mat_grab,_mat_resize,cv::Size(VWIDTH,VHEIGHT));

	_img_resize=matToImage(_mat_resize);
    
    stretchContrast(_mat_resize,_mat_contrast,60,180);
    _img_contrast=matToImage(_mat_contrast);
    
	return bNewFrame;
}

// for main opencv processing
void ofApp::cvProcess(cv::Mat& grab_){
	cv::cvtColor(_mat_contrast,_mat_gray,CV_BGR2GRAY);
    cv::blur(_mat_gray,_mat_gray,cv::Size(3,3));

	//cv::equalizeHist(_mat_gray,_mat_normalize);
	cv::normalize(_mat_gray,_mat_normalize,0,255,CV_MINMAX);
	/*auto clahe=cv::createCLAHE();
	clahe->apply(_mat_gray,_mat_normalize);*/

    //if(_effect==DEFFECT::)cv::threshold(_mat_gray,_mat_thres,80,255,cv::THRESH_BINARY_INV);
    cv::adaptiveThreshold(_mat_normalize,_mat_thres,255,cv::ADAPTIVE_THRESH_GAUSSIAN_C,cv::THRESH_BINARY_INV,33,5);
    
    cv::Mat struct_;
    if(_next_effect==DEFFECT::EDGE_WALK) struct_=cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(13,13));
    else struct_=cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(7,7));
    cv::morphologyEx(_mat_thres,_mat_morph,cv::MORPH_CLOSE,struct_);

	float area_=PWIDTH*PHEIGHT;

	cv::Canny(_mat_morph, _mat_edge,50,150,3);
	_contours.clear();
	_hierarchy.clear();

    cv::Mat copy_;
	_mat_morph.copyTo(copy_);

    cv::findContours(copy_,_contours,_hierarchy,CV_RETR_CCOMP,cv::CHAIN_APPROX_SIMPLE,cv::Point(0,0));
	updateBlob(_contours,_hierarchy);
    
    
    

    _img_contrast=matToImage(_mat_contrast);
	_img_gray=matToImage(_mat_gray);
	_img_thres=matToImage(_mat_thres);
	_img_edge=matToImage(_mat_edge);
	_img_normalize=matToImage(_mat_normalize);
    _img_morph=matToImage(_mat_morph);


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
    
	if(_hist.rows<skip_) return;
	
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
        float binVal=_hist.at<float>(ofClamp((int)(i+ofGetFrameNum()+skip_)%(int)(_hist_size-skip_*2),0,_hist.rows));
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
    float frame_=PHEIGHT*PHEIGHT;
    
    
    _collect_blob.clear();
    
    for(int i=0;i<clen;++i){
        Blob b_=contourApproxBlob(contour_[i]);
        /*float d_=ofDist(b_._center.x, b_._center.y,PHEIGHT/2,PHEIGHT/2);
        if(d_>PHEIGHT/2) continue;*/

        if(b_._center.x>VWIDTH || b_._center.x<0 || b_._center.y>VHEIGHT || b_._center.y<0) continue;
		if(b_._rad>VWIDTH*.03) continue;


        float area_=cv::contourArea(contour_[i]);
		if(area_>_param->_blob_small && area_<_param->_blob_large*frame_){
            
            DetectBlob db_;
            db_._id=_collect_blob.size();
            db_._blob=b_;
            _collect_blob.push_back(db_);
            
        }
        
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




#pragma mark OF_UI

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    
    string message_;
    
	switch (key){
		case '0':
            setMode(MODE::RUN);
			break;
		case '9':
			setMode(MODE::DETECT);
			break;
            
		case '1':
			_next_effect=DEFFECT::BIRD;
            _track=0;
            setMode(MODE::DETECT);
            break;
		case '2':
			_next_effect=DEFFECT::SCAN;
            _track=1;
            setMode(MODE::DETECT);
			break;
		case '3':
            _next_effect=DEFFECT::EDGE_WALK;
            _track=2;
			setMode(MODE::DETECT);
			break;
        case '4':
            _next_effect=DEFFECT::BIRD;
            _track=3;
            setMode(MODE::DETECT);
            break;
        case '5':
            _next_effect=DEFFECT::BLOB_SELECT;
            _track=4;
            setMode(MODE::DETECT);
            break;
        
        case '6':
            _next_effect=DEFFECT::BUG;
            _track=3;
            setMode(MODE::DETECT);
            break;
            
        case 'a':
			switch(_effect){
				case SCAN:
                    _anim_scan.setDue(ofClamp(_anim_scan.getDue()-500,_param->_scan_vel/10.0,_param->_scan_vel));
                    ofLog()<<_anim_scan.getDue();
                    break;
				case EDGE_WALK:
					addPacMan(ofRandom(2)<1);
					break;
				case BLOB_SELECT:
                    addSelectKey();
					break;
                case BIRD:
                    DetectBlob::CenterForce=ofClamp(DetectBlob::CenterForce-.5,1,2);
                    
                    DetectBlob::MaxForce=ofClamp(DetectBlob::MaxForce-.02,.1,.5);
                    DetectBlob::MaxSpeed=ofClamp(DetectBlob::MaxSpeed+1,15,20);
                    ofLog()<<"Bird param: "<<DetectBlob::CenterForce<<" "<<DetectBlob::MaxForce<<" "<<DetectBlob::MaxSpeed;
                    break;
                default:
                    break;
			}
			break;
		case 'z':
			switch(_effect){
                case SCAN:
                    _anim_scan.setDue(ofClamp(_anim_scan.getDue()+500,_param->_scan_vel/10.0,_param->_scan_vel));
                    ofLog()<<_anim_scan.getDue();
                    break;
				case EDGE_WALK:
					//addPacMan(true);
                    removePacMan();
					break;
                case BLOB_SELECT:
                    setEffect(DEFFECT::BLOB_SELECT);
                    break;
                case BIRD:
                    DetectBlob::CenterForce=ofClamp(DetectBlob::CenterForce+.5,1,2);
                    
                    DetectBlob::MaxForce=ofClamp(DetectBlob::MaxForce+.02,.1,.5);
                    DetectBlob::MaxSpeed=ofClamp(DetectBlob::MaxSpeed-1,15,20);
                    
                    ofLog()<<"Bird param: "<<DetectBlob::CenterForce<<" "<<DetectBlob::MaxForce<<" "<<DetectBlob::MaxSpeed;
                    break;
                default:
                    break;
			}
            break;
        case 'r':
            switch(_effect){
                case EDGE_WALK:
                    _scan_dir=SCANDIR(((int)_scan_dir+1)%2);
                    break;
                default:
                    break;
            }
			break;
        /* utility */
		case 'd':
			_debug=!_debug;
			break;
		case 's':
			_param->saveParameterFile();
			break;
        case 'f':
            _do_fft=!_do_fft;
            break;
            
        /* test */
        case 'k':
            sendOSC("/live/tempo",vector<float>());
			break;
		case 'l':
			triggerSound();
			break;
            
        /* motor */
        case 'q':
            message_="motor_speedup#";
            _serial.writeBytes((unsigned char*)message_.c_str(),message_.size()+1);
            break;
        case 'w':
            message_="motor_slowdown#";
            _serial.writeBytes((unsigned char*)message_.c_str(),message_.size()+1);
            break;
        case 'e':
            message_="motor_toggle#";
            _serial.writeBytes((unsigned char*)message_.c_str(),message_.size()+1);
            break;
        case 'p':
#ifdef USE_VIDEO
			_video.play();
			_video.setPosition(0);
#endif
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


#pragma mark MODE_AND_EFFECT

void ofApp::setMode(MODE set_){
	switch(set_){
		case DETECT:
            _anim_detect.setDue(250);
			_anim_detect.restart();
			_idetect_view=0;
			cvProcess(_mat_contrast);
			for(auto& b:_collect_blob) b.setTrigger(true);			
			break;
		case EFFECT:
			for(auto& b:_collect_blob) b.setTrigger(false);			
			break;				
	}
	_mode=set_;
}


void ofApp::setEffect(DEFFECT set_){

	setMode(MODE::EFFECT);
	int len_=0;
	switch(set_){
		case EDGE_WALK:
            _img_morph.draw(0,0,PWIDTH,PHEIGHT);
            
            //_mat_morph.convertTo(_mat_pacman,CV_8UC3);
			cv::findNonZero(_mat_morph,_mat_nonzero);
			len_=_mat_nonzero.total();
			_nonzero_point.clear();
			_nonzero_start.clear();
			for(int i=0;i<len_;++i){
                
                cv::Point p_=_mat_nonzero.at<cv::Point>(i);
                
                _nonzero_point.push_back(p_);
                
               // float d_=ofDist(p_.x,p_.y,PHEIGHT/2,PHEIGHT/2);
               // if(d_>=(float)PHEIGHT/2*.7 && d_<=(float)PHEIGHT/2*.85){
				//if(p_.x>PWIDTH*.75 || p_.x<PWIDTH*.25 || p_.y>PHEIGHT*.75 || p_.y<PHEIGHT*.25){
//                    if(p_.y<PHEIGHT/2)
                        _nonzero_start.push_back(p_);
//                    else _nonzero_gstart.push_back(p_);
              //  }
			}
			_pacman.clear();
			for(int i=0;i<1;++i){
				addPacMan(ofRandom(2)<1);
				//addPacMan(true);
			}
            _fbo_pacman.begin();
                ofClear(255,0);
                //_img_morph.draw(0,0);
            _fbo_pacman.end();
            
			break;
		case SCAN:
            _anim_scan.setDue(_param->_scan_vel);
			_anim_scan.restart();
            _scan_pos=0;
            _scan_range=_param->_mscan_region;
			break;
		case BLOB_SELECT:
			_anim_select.restart();

			_selected.clear();
			_not_selected.clear();
			for(auto b:_collect_blob){
				
				_not_selected.push_back(b);
			}
//			random_shuffle(_not_selected.begin(),_not_selected.end());
//            DetectBlob::Center=ofVec2f(_not_selected[0]._blob._center.x,_not_selected[0]._blob._center.y);
//            std::sort(_not_selected.begin(),_not_selected.end());
            
            _select_due=_param->_select_vel;
            addSelectKey();
			
			break;
        case BIRD:
            DetectBlob::MaxSpeed=15;
            DetectBlob::MaxForce=.5;
            DetectBlob::CenterForce=2.0;
            
            _bird.clear();
            for(auto& b:_collect_blob){
                _bird.push_back(b);
            }
            DetectBlob::Center=ofVec2f(VWIDTH/2,VHEIGHT/2);
            for(auto& b:_bird){
                //cv::Vec3b c_=_mat_contrast.at<cv::Vec3b>(b._blob._center.y,b._blob._center.x);
                //b.finit(ofColor((int)c_[2],(int)c_[1],(int)c_[0]));
                b.finit(DetectBlob::BColor[(int)floor(ofRandom(2))]);
            }
            break;
        case BUG:
            _bug_box.clear();
            _mbug_box=mBugBox;
            float eang=(float)360/_mbug_box;
            
            for(int i=0;i<_mbug_box;++i){
                BugBox b_(eang*i,eang*.9,VHEIGHT/2);
                _bug_box.push_back(b_);
            }
            break;
	}
	_effect=set_;
}

#pragma mark CV_UTIL

Blob ofApp::contourApproxBlob(vector<cv::Point>& contour_){
	Blob b;
    vector<cv::Point> poly_;
    cv::Point2f center_;
    cv::Rect bounding_;
	float rad_;

    cv::approxPolyDP(cv::Mat(contour_),poly_,2,true);
    bounding_=cv::boundingRect(cv::Mat(poly_));
	minEnclosingCircle(poly_,center_,rad_);
    //ofLog()<<center_.x<<", "<<center_.y;
    
    for(auto& p:poly_){
        p.x-=center_.x;
        p.y-=center_.y;
    }
    
	b._contours=poly_;
	b._center=center_;
	b._bounding=bounding_;
	b._rad=rad_;
	b._npts=poly_.size();

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



bool ofApp::isSimilar(Blob b1_,Blob b2_){
    if(abs(b1_._bounding.area()-b2_._bounding.area())<200
       && abs(b1_._center.y-b2_._center.y)<20) return true;
    return false;
}

#pragma mark EFFECT_SCAN
bool ofApp::isScanned(DetectBlob detect_){
    
    float ehei=VHEIGHT/_scan_range;
    float ebegin=VHEIGHT*0.1;
    float eang=360.0/_scan_range;
    
    float spos_=(_scan_dir==SCANDIR::VERT)?_anim_scan.val()*VWIDTH:_anim_scan.val()*PHEIGHT/2;
    float rpos_=(_scan_dir==SCANDIR::VERT)?(_scan_range-_scan_pos)*ehei:_scan_pos*eang;
    cv::Rect rec_(0,rpos_,spos_,ehei);
    
    float dist=0;
    float ang=0;
    
    switch(_scan_dir){
        case VERT:
            if((rec_&detect_._blob._bounding).area() >0) return true;
            break;
        case RADIAL:
            dist=ofDist(detect_._blob._center.x,detect_._blob._center.y,PWIDTH/2,PHEIGHT/2);
            ang=ofRadToDeg(fmod(atan2(detect_._blob._center.y-PHEIGHT/2,detect_._blob._center.x-PWIDTH/2)+TWO_PI,TWO_PI));
            
            float brad_=detect_._blob._rad;//max(detect_._blob._bounding.width,detect_._blob._bounding.height)/2;
            if(dist-brad_<spos_ && dist+brad_>spos_ && ang>rpos_-eang/2 && ang<=eang/2+rpos_) return true;
            
            break;
            
    }
    return false;
    
}

void ofApp::checkScan(SCANDIR dir_){
//    float epos_=(_scan_dir==SCANDIR::VERT)?PHEIGHT/(float)_param->_mscan_region
//				:TWO_PI/(float)_param->_mscan_region;
    //bool touched_[MSCANREGION];
//    for(int i=0;i<_scan_touched.size();++i) _scan_touched[i]=false;
    
    float ang=0;
    
    int mtrigger=0;
    int trigger_area=0;
    
    for(auto& b:_collect_blob){
        
        switch(_scan_dir){
            case VERT:
                if(isScanned(b)){
                    if(!b.getTrigger()){
                        b.setTrigger(true);
                        mtrigger++;
                        triggerSound();
                        
                    }
                }
                break;
            case RADIAL:
                if(isScanned(b)){
                    
                    if(!b.getTrigger()){
                        b.setTrigger(true);
                        mtrigger++;
                        triggerSound();
                        
                    }
                    
                }//else b.setTrigger(false);
                break;
                
        }
    }
    _report2<<"mtrigger="<<mtrigger<<endl
    <<"cover area="<<trigger_area<<endl;
    
    
    
    
    
}
#pragma mark EFFECT_PACMAN

void ofApp::updatePacMan(PacMan& p_){
    
    
    if(p_._dead) return;
    
	ofVec2f pos_=p_.getPos();

	//find next white
	ofVec2f next_(0);
    int mturn_=3;
    vector<int> change_(mturn_*2);
    for(int i=1;i<=mturn_;++i){
        change_[2*(i-1)]=i;
        change_[2*(i-1)+1]=-i;
    }
	
	random_shuffle(change_.begin(),change_.end());

	
	//check original direction first
    if(goodStep(p_,p_._dir)){
		next_=p_._dir;
	}else{	
		

		for(int i=0;i<mturn_*2;++i){
            ofVec2f ndir_=p_._dir;
            ndir_.rotate(change_[i]*PacMan::CornerAngle);
            if(goodStep(p_,ndir_)){
				next_=ndir_;
				break;
				//cout<<endl;
			}
						
		}
		//cout<<endl;

	}
	if(next_.length()==0){
		//p_.restart(findWhiteStart(p_._ghost),3);
        p_.goDie();
       // triggerSound();
        
	}else{
		p_.setPos(pos_+next_*PACMANVEL);
		
        if(p_._dir.angle(next_)>PacMan::CornerAngle) triggerSound();
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

void ofApp::removePacMan(){
    
    if(_pacman.size()>0)
        _pacman.erase(_pacman.begin());
}


bool ofApp::checkWhite(int x_,int y_){
	/*if(x_<0 || x_>=_mat_thres.cols || y_<0 || y_>=_mat_thres.rows) return false;
 	return _mat_thres.at<uchar>(y_,x_)>0;*/

    return std::find(_nonzero_point.begin(),_nonzero_point.end(),cv::Point(x_,y_))!=_nonzero_point.end();

}

ofVec2f ofApp::findWhiteStart(bool ghost_){
	
//	if(!ghost_){
//		int len_=_nonzero_start.size();
//		int i_=floor(ofRandom(len_));
//
//		return ofVec2f(_nonzero_start[i_].x,_nonzero_start[i_].y);
//	}else{
//		int len_=_nonzero_gstart.size();
//		int i_=floor(ofRandom(len_));
//
//		return ofVec2f(_nonzero_gstart[i_].x,_nonzero_gstart[i_].y);
//	}
    
    int len_=_nonzero_start.size();
   
    int i_=floor(ofRandom(len_));
    return ofVec2f(_nonzero_start[i_].x,_nonzero_start[i_].y);
   
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
                
               // triggerSound();
                
//                if(_pacman[i]._ghost || _pacman[j]._ghost) triggerSound(3);
//                else triggerSound(3);
            }
            
        }
    }
}
#pragma mark EFFECT_BLOB


void ofApp::addSelectKey(){
    vector<DetectBlob> seq_;
    
    
    //copy not_selected
//    _not_selected.clear();
//    for(auto b:_collect_blob) _not_selected.push_back(b);
    
    int len=_not_selected.size();
    if(len<1) return;
    
    
    int ind=floor(ofRandom(len));
    int step_=_not_selected.size();//floor(ofRandom(1)+_param->_select_len);
    
    cv::Point cur_=_not_selected[ind]._blob._center;
    
    for(int i=0;i<step_;++i){
        vector<DetectBlob> res_=getNextBlob(cur_,_param->_mper_select);
        int rlen=res_.size();
        for(int j=0;j<rlen;++j) seq_.push_back(res_[j]);
    }
    if(seq_.size()<1) return;
    
    SelectSeq s(_param->_select_vel*ofRandom(.75,1.25),seq_.size(),_param->_mper_select);
    s._blobs=seq_;
    
    _selected.push_back(s);
}

vector<DetectBlob> ofApp::getNextBlob(cv::Point cent_,int count_){
    
    vector<DetectBlob> res_;
    
    DetectBlob::Center=ofVec2f(cent_.x,cent_.y);
    std::sort(_not_selected.begin(),_not_selected.end());
    
    for(int i=0;i<min(count_,(int)_not_selected.size());++i){
        res_.push_back(_not_selected[0]);
        _not_selected.erase(_not_selected.begin());
    }
    return res_;
}

void ofApp::updateSelectSeq(SelectSeq& seq){
    
    if(seq._anim.val()<1 || seq._dead) return;

    bool trigger_=false;
    float area_=0;
    for(int i=0;i<_param->_mper_select;++i){
        if(seq._index<seq._count){
            seq._blobs[seq._index].setTrigger(true);
            area_+=seq._blobs[seq._index]._blob._rad;
            
            trigger_=true;
            seq._index++;
            
        }
    }
    if(trigger_){
        triggerSound();
        
        float char_=ofMap(area_,10,PHEIGHT*.1,_select_due*.8,_select_due*1.2);
        seq._anim.setDue(char_);
        seq._anim.restart();
    }else{
        seq._anim.stop();
        seq._dead=true;
        
//        for(auto& b:seq._blobs){
//            b.setTrigger(false);
//            _not_selected.push_back(b);
//        }
        
        
        addSelectKey();
        
    }
    

    
}

#pragma mark EFFECT_BIRD
void ofApp::updateBirdRegion(){

	float width=ofGetWidth()/MBIRDREGION;
	float height=ofGetHeight()/MBIRDREGION;

	for(int i=0;i<MBIRDREGION;++i)
		for(int j=0;j<MBIRDREGION;++j){
			_region_bird[i*MBIRDREGION+j].clear();
		}

	for(auto b:_bird){
		int rx=ofClamp(floor(b._floc.x/(width)),0,MBIRDREGION-1);
		int ry=ofClamp(floor(b._floc.y/(height)),0,MBIRDREGION-1);
		_region_bird[rx*MBIRDREGION+ry].push_back(b);
	}
}

#pragma mark EFFECT_BUG
bool ofApp::checkBugTouch(BugBox& box_){
    
    bool touched=false;
    
    for(auto& b:_collect_blob){
        if(box_.intersect(b._blob)){
            b.setTrigger(true);
            touched=true;
        }
    }
    if(touched && !box_._trigger){
        triggerSound();
    }
    box_._trigger=touched;
    
    return touched;

}


#pragma mark LIVE_TRIGGER
void ofApp::triggerSound(){
	vector<float> p_;
		/*p_.push_back(_track);
		p_.push_back(floor(ofRandom(SoundTrackCount[_track])));*/
		p_.push_back(0);
		p_.push_back(floor(ofRandom(13)));
		sendOSC("/live/play/clip",p_);
}

void ofApp::remoteVolume(int track_,float vol_){
    
    vector<float> p_;
    p_.push_back(track_);
    p_.push_back(vol_);
    sendOSC("/live/volume",p_);
    
}

#pragma mark COMMUNICATION
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


void ofApp::receiveOSC(){
	while(_osc_receiver.hasWaitingMessages()){
		// get the next message
		ofxOscMessage m;
		_osc_receiver.getNextMessage(m);
		//cout<<"recieve message: "<<m.getAddress()<<" ";
        for(int i=0;i<m.getNumArgs();++i){
            auto t_=m.getArgType(i);
          //  if(t_==OFXOSC_TYPE_INT32) cout<<m.getArgAsInt(i)<<" ";
          //  else if(t_==OFXOSC_TYPE_FLOAT) cout<<m.getArgAsFloat(i)<<" ";

        }
		//cout<<endl;
	}

}

void ofApp::updateSerial(){
    
    if(_serial.isInitialized() && _serial.available()){
        vector<string> val=readSerialString(_serial,'#');
        if(val.size()<1) return;
        ofLog()<<"serial read: "<<ofToString(val)<<endl;
        
        
        if(val[0]=="set_0"){
            if(_mode!=MODE::EFFECT || _effect!=DEFFECT::BIRD){
                //_next_effect=DEFFECT::SCAN;
                _next_effect=DEFFECT::BIRD;
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
                addPacMan(ofRandom(2)<1);
            }else if(val[0]=="add_ghost"){
                //addPacMan(true);
                removePacMan();
            }
        }
        if(_effect==DEFFECT::BLOB_SELECT){
            if(val[0]=="blob_reset"){
                setEffect(DEFFECT::BLOB_SELECT);
                //addSelectKey();
                
            }else if(val[0]=="speed_b"){
                float v=ofToFloat(val[1]);
                _anim_select.setDue(_param->_select_vel*(1.0-v/255.0*0.99));
                _select_due=_param->_select_vel*(1.0-v/255.0*0.99);
            }
        }
        if(_effect==DEFFECT::SCAN){
            if(val[0]=="scan_line"){
                _scan_dir=SCANDIR::VERT;
            }else if(val[0]=="scan_radial"){
                _scan_dir=SCANDIR::RADIAL;
            }else if(val[0]=="speed_a"){
                float v=ofToFloat(val[1]);
                _anim_scan.setDue(_param->_scan_vel*(1.0+v/255.0*20.0));
            }
        }
        if(_effect==DEFFECT::BIRD){
            if(val[0]=="speed_a"){
                float v=ofToFloat(val[1]);
                DetectBlob::CenterForce=ofMap(v,0,255,1,3);
                
                DetectBlob::MaxForce=ofMap(v,0,255,.1,.5);
                DetectBlob::MaxSpeed=ofMap(255-v,0,255,10,20);

                
                ofLog()<<"Bird param: "<<DetectBlob::CenterForce<<" "<<DetectBlob::MaxForce<<" "<<DetectBlob::MaxSpeed;
                
            }
        }
        
    
    }
    
    
}







