#pragma once
#ifndef DETECT_BLOB_H
#define DETECT_BLOB_H

#define LIFESPAN 3
#define PACMANVEL 1
#define MBCOLOR 2

#include "ofMain.h"

struct Blob{
    vector<cv::Point> _contours;
	int _npts;

    cv::Point2f _center;
    cv::Rect _bounding;
	float _rad;

};
class DetectBlob{
private:	
	float _walk_pos;



	bool _trigger;
	int _life;
	int _color;
	FrameTimer _danim;
public:
	static ofColor* BColor;
	static ofVec2f _SelectStart;
	int _id;
	Blob _blob;
    bool _draw_text;

	DetectBlob(){
		_trigger=false;
		_walk_pos=0;
		_life=LIFESPAN;
		_color=floor(ofRandom(MBCOLOR));
		_danim=FrameTimer(100,ofRandom(100));
		_danim.setContinuous(true);
		_danim.restart();
        
        _draw_text=ofRandom(5)<1;
	}
	void update(float dt_){
		_danim.update(dt_);
	}
	void setTrigger(bool set_){

		if(_trigger==set_) return;

		_trigger=set_;
		/*if(_trigger) _danim.restart();
		else _danim.stop();*/
	}
	void draw(float p_,bool fill_,ofTrueTypeFont& font_){

		if(!_trigger) return;
        
        drawTriggered(p_,fill_);
        
        if(_draw_text){
            
            ofPushStyle();
            ofSetColor(255,120);
                ofPushMatrix();
                    int i=floor(_blob._contours.size()*p_);
                    if(!fill_) font_.drawString(ofToString(_blob._contours[i].x)+","+ofToString(_blob._contours[i].y),_blob._center.x,_blob._center.y);
                    else font_.drawString(ofToString(_blob._center.x)+","+ofToString(_blob._center.y),_blob._center.x,_blob._center.y);
                ofPopMatrix();
            ofPopStyle();
        }
        
	}
	void drawTriggered(float p_,bool fill_){
		ofPushStyle();
		
		float d_=1.0*_danim.val();
		ofPushMatrix();
		ofTranslate(-d_,-d_);
			ofSetColor(DetectBlob::BColor[1],255.0*p_);
			if(!fill_) ofNoFill();	
			else ofFill();
			drawShape(fill_?1.0:p_);
		ofPopMatrix();
		ofPushMatrix();
		ofTranslate(d_,d_);
			ofSetColor(DetectBlob::BColor[0],255.0*p_);
			if(!fill_) ofNoFill();				
			else ofFill();

			drawShape(fill_?1.0:p_);
		ofPopMatrix();
        ofPushMatrix();
        ofSetColor(255,180);
        if(!fill_) ofNoFill();
        else ofFill();
        drawShape(fill_?1.0:p_);
        ofPopMatrix();
        
		
		ofPopStyle();
	}
	

	void drawDebug(){
		ofPopStyle();
		ofSetColor(255,120);
		ofNoFill();
			drawShape(1.0);
		ofPopStyle();
	}
	void drawShape(float p_){
		int len=(int)((float)_blob._npts*p_);
		ofBeginShape();
		for(int i=0;i<len;++i) ofVertex(_blob._contours[i].x,_blob._contours[i].y);
		ofEndShape();
	}
	void drawBounding(){
		ofDrawRectangle(_blob._bounding.x,_blob._bounding.y,
					    _blob._bounding.width,_blob._bounding.height);		
	}
	bool operator<(const DetectBlob& b) const{
		return _SelectStart.distance(ofVec2f(_blob._center.x,_blob._center.y))
			<_SelectStart.distance(ofVec2f(b._blob._center.x,b._blob._center.y));
	}
};
class PacMan{
private:
	ofVec2f _pos;
public:
	static ofVec2f* Direction;
	static ofVec2f* GDirection;
    static int MPathRecord;
    static ofColor* GColor;

	static float Rad;
    

	/*ofVec2f _vel;
	ofVec2f _acc;*/
	int _dir;
	vector<ofVec2f> _path;

	bool _ghost;
    int _gcolor;
    
    bool _dead;
    FrameTimer _timer_dead;
    
	PacMan(int d_){
		_pos=ofVec2f(0,0);
		_dir=d_;
		_ghost=false;
        _gcolor=floor(ofRandom(4));
        
        _timer_dead=FrameTimer(500);
        _dead=false;
	}
	PacMan(float x_,float y_,bool g_){
		_pos=ofVec2f(x_,y_);
		_ghost=g_;
		_dir=2;
        _gcolor=floor(ofRandom(4));
        
        _timer_dead=FrameTimer(500);
        _dead=false;
    }
    void update(float dt_){
        _timer_dead.update(dt_);
    }
	void draw(){
//        float life_=_path.size();
//        for(int i=0;i<life_;++i){
//            ofPushMatrix();
//            ofTranslate(_path[i].x,_path[i].y);
//
//            ofPushStyle();
//            
//            float alpha_=255;//(float)i/life_*255.0;
//            
//            if(_ghost) ofSetColor(GColor[_gcolor],alpha_);
//            else ofSetColor(255,255,0,alpha_);
//            ofFill();
//
//            ofDrawCircle(0,0,Rad);
//		
//            ofPopStyle();
//            ofPopMatrix();
//        }
    
        
            ofPushMatrix();
            ofTranslate(_pos.x,_pos.y);
            
            ofPushStyle();
        
        if(!_dead){
            
            if(_ghost) ofSetColor(GColor[_gcolor],255);
            else ofSetColor(255,255,0,255);
            ofFill();
            ofDrawCircle(0,0,Rad);
            
        }else{
            
            float a_=255.0*(1.0-_timer_dead.val());
            
            if(_ghost) ofSetColor(GColor[_gcolor],a_);
            else ofSetColor(255,255,0,a_);
            ofFill();
            ofDrawCircle(0,0,Rad*(1.0+2*_timer_dead.val()));
            
        }
        
            ofPopStyle();
            ofPopMatrix();
        
    
	}
	void setPos(ofVec2f p_){
		_path.push_back(p_);
		if(_path.size()>MPathRecord) _path.erase(_path.begin());

		_pos=p_;
	}
	bool alreadyPass(float x_,float y_){
		return std::find(_path.begin(),_path.end(),ofVec2f(x_,y_))!=_path.end();
	}
	ofVec2f getPos(){
		return _pos;
	}
	void restart(ofVec2f p_,int d_){
		_path.clear();
		_pos=p_;
		_dir=d_;
        _dead=false;
        _timer_dead.reset();
	}
    void goDie(){
        if(!_dead){
            _dead=true;
            _timer_dead.restart();
        }
    }
    bool isDead(){
        return _dead && _timer_dead.val()>=1;
    }
	
};


#endif
