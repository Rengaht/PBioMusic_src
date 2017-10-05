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
	int _id;
	Blob _blob;
    bool _draw_text;
    
    
    //flocking
    ofVec2f _floc,_fvel,_facc;
    
    static float MaxSpeed;
    static float MaxForce;
    static ofVec2f Center;
    
    ofColor _center_color;

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

		//if(_trigger==set_) return;

		_trigger=set_;
		/*if(_trigger) _danim.restart();
		else _danim.stop();*/
	}
    bool getTrigger(){
        return _trigger;
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
        
        ofPushMatrix();
        ofTranslate(_blob._center.x,_blob._center.y);
        
        if(fill_) ofScale(p_,p_);
        
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
        
        ofPopMatrix();
		
		ofPopStyle();
	}
    void fdraw(){
        
        ofPushStyle();
        float d_=2.0*_danim.val();
        
        
        if(_trigger){
            ofPushMatrix();
            ofTranslate(_blob._center.x,_blob._center.y);
            
            ofPushMatrix();
            ofTranslate(-d_,-d_);
            
            ofSetColor(DetectBlob::BColor[1],120.0);
            ofFill();
                drawShape(1.0);
            ofPopMatrix();
            
            ofPushMatrix();
            ofTranslate(d_,d_);
            ofSetColor(DetectBlob::BColor[0],120.0);
            ofFill();
                drawShape(1.0);
            ofPopMatrix();
            
            ofPopMatrix();
        }
        
        ofPushMatrix();
        ofTranslate(_floc.x,_floc.y);
        
        ofRotate(ofRadToDeg(atan2(_floc.y,_floc.x)));
        
            ofPushMatrix();
            ofTranslate(0,0);
            if(_trigger) ofSetColor(255,120);
            else ofSetColor(_center_color,120);
            ofFill();
                drawShape(1.0);
            ofPopMatrix();
        
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
		return Center.distance(ofVec2f(_blob._center.x,_blob._center.y))
			<Center.distance(ofVec2f(b._blob._center.x,b._blob._center.y));
	}
    
    void finit(ofColor color_){
        _floc=ofVec2f(_blob._center.x,_blob._center.y);
        _fvel=ofVec2f(0,0);
        //_fvel.rotate(ofRandom(360));
        
        _trigger=false;
        
        _center_color=color_;
    }
    void fupdate(vector<DetectBlob>& boids,float dt_){
        
        update(dt_);
        
         flock(boids);
        
        _fvel+=_facc;
        _fvel.limit(MaxSpeed);
        _floc+=_fvel;
        
        _facc*=0;
    }

    /* flocking */
    void flock(vector<DetectBlob>& boids){
        ofVec2f sep=seperate(boids);
        ofVec2f ali=align(boids);
        ofVec2f coh=cohesion(boids);
        
        sep*=1.2;
        ali*=1.0;
        coh*=1.0;
        
        applyForce(sep);
        applyForce(ali);
        applyForce(coh);
        
        
        // boundary
//        ofVec2f desired;
//         if(_floc.x>PHEIGHT){
//            // desired=ofVec2f(-MaxSpeed,_fvel.y);
//             applyForce(ofVec2f(-MaxForce*3,0));
//         }else if(_floc.x<0){
//            // desired=ofVec2f(MaxSpeed,_fvel.y);
//             applyForce(ofVec2f(MaxForce*3,0));
//         }
//         if(_floc.y>PHEIGHT){
//            // desired=ofVec2f(_fvel.x,-MaxSpeed);
//             applyForce(ofVec2f(0,-MaxForce*3));
//         }else if(_floc.y<0){
//           //  desired=ofVec2f(_fvel.x,MaxSpeed);
//             applyForce(ofVec2f(0,MaxForce*3));
//         }
//         if(desired.length()>0){
//             ofVec2f steer=desired-_fvel;
//             steer.limit(MaxForce*5);
//             applyForce(steer);
//         }
        
        //center
        ofVec2f cent_desired=Center-_floc;
        ofVec2f cent_steer=cent_desired-_fvel;
        cent_steer.limit(MaxForce*2);
        applyForce(cent_steer);
    }
    
    void applyForce(ofVec2f force){
        _facc+=force;
    }
    
    ofVec2f align(vector<DetectBlob>& boids){
        
        float neighbor_dist=_blob._rad*4;
        ofVec2f sum(0,0);
        
        for(auto &b:boids){
            float d=_floc.distance(b._floc);
            if(d>0 && d<neighbor_dist){
                sum+=b._fvel;
            }
        }
        sum.normalize();
        sum*=MaxSpeed;
        
        ofVec2f steer=sum-_fvel;
        steer.limit(MaxForce);
        return steer;
    }
    ofVec2f cohesion(vector<DetectBlob>& boids){
        
        float neighbor_dist=_blob._rad*3;
        ofVec2f sum;
        int count=0;
        for(auto &b:boids){
            float d=_floc.distance(b._floc);
            if(d>0 && d<neighbor_dist){
                sum+=b._floc;
                count++;
            }
        }
        if(count>0){
            sum/=count;
            sum.normalize();
            sum*=MaxSpeed;
            ofVec2f steer=sum-_fvel;
            steer.limit(MaxForce);
            return steer;
        }
        return ofVec2f(0,0);
        
        
    }
    ofVec2f seperate(vector<DetectBlob>& boids){
        float desired_separataion=_blob._rad*2;
        ofVec2f sum(0,0);
        int count=0;
        for(auto &b:boids){
            float d=_floc.distance(b._floc);
            if(d>0 && d<desired_separataion){
                ofVec2f diff=_floc-b._floc;
                diff.normalize();
                diff/=d;
                sum+=diff;
                count++;
            }
        }
        if(count>0){
            sum/=count;
            sum.normalize();
            sum*=MaxSpeed;
            ofVec2f steer=sum-_fvel;
            steer.limit(MaxForce);
            return steer;
        }
        return ofVec2f(0,0);
    }
};


#endif
