#pragma once
#ifndef DETECT_BLOB_H
#define DETECT_BLOB_H

#define LIFESPAN 3
#define PACMANVEL 2
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

public:
	static ofVec2f* Direction;
	static float Rad;

	ofVec2f _pos;
	/*ofVec2f _vel;
	ofVec2f _acc;*/
	int _dir;

	PacMan(){
		_pos=ofVec2f(0,0);
		_dir=floor(ofRandom(8));
		
	}
	PacMan(float x_,float y_){
		_pos=ofVec2f(x_,y_);
		_dir=floor(ofRandom(8));
	}
	void draw(){
		ofPushMatrix();
		ofTranslate(_pos.x,_pos.y);

		ofPushStyle();
		ofSetColor(255,255,0);
		ofFill();

		ofDrawCircle(0,0,Rad);
		
		ofPopStyle();
		ofPopMatrix();
	}
};


#endif