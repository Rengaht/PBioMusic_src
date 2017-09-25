#pragma once
#ifndef DETECT_BLOB_H
#define DETECT_BLOB_H

#define LIFESPAN 3
#define PACMANVEL 1
#define MBCOLOR 2

#include "ofMain.h"

struct Blob{
	vector<Point> _contours;
	int _npts;

	Point2f _center;
	Rect _bounding;
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

	DetectBlob(){
		_trigger=false;
		_walk_pos=0;
		_life=LIFESPAN;
		_color=floor(ofRandom(MBCOLOR));
		_danim=FrameTimer(100,ofRandom(100));
		_danim.setContinuous(true);
		_danim.restart();
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
	void draw(float p_,bool fill_){

		if(!_trigger) return;
		drawTriggered(p_,fill_);
	}
	void drawTriggered(float p_,bool fill_){
		ofPushStyle();
		
		float d_=1.0*_danim.val();
		ofPushMatrix();
		ofTranslate(-d_,-d_);
			ofSetColor(DetectBlob::BColor[1],120.0*p_);
			if(!fill_) ofNoFill();	
			else ofFill();
			drawShape(fill_?1.0:p_);
		ofPopMatrix();
		ofPushMatrix();
		ofTranslate(d_,d_);
			ofSetColor(DetectBlob::BColor[0],120.0*p_);
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
	bool operator<(const DetectBlob& b){
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

	static float Rad;

	/*ofVec2f _vel;
	ofVec2f _acc;*/
	int _dir;
	vector<ofVec2f> _path;

	bool _ghost;

	PacMan(int d_){
		_pos=ofVec2f(0,0);
		_dir=d_;
		_ghost=false;
	}
	PacMan(float x_,float y_,bool g_){
		_pos=ofVec2f(x_,y_);
		_ghost=g_;
		_dir=2;
	}
	void draw(){
		ofPushMatrix();
		ofTranslate(_pos.x,_pos.y);

		ofPushStyle();
		if(_ghost) ofSetColor(255,0,0);
		else ofSetColor(0,0,255);
		ofFill();

		ofDrawCircle(0,0,Rad);
		
		ofPopStyle();
		ofPopMatrix();
	}
	void setPos(ofVec2f p_){
		_path.push_back(p_);
		if(_path.size()>3) _path.erase(_path.begin());

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
	}
	
};


#endif