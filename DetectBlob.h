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

class BugBox{
public:
    static float ArcWidth;
    bool _trigger;
    //cv::RotatedRect _bounding;
    float _angle;
    float _arc_angle;
    float _rad;
    
    BugBox(float ang_,float arc_,float rad_){
         _trigger=false;
        _angle=ang_;
        _arc_angle=arc_;
        _rad=rad_-ArcWidth/2;
        
    }
    void draw(){
        ofPushStyle();
        if(_trigger) ofSetColor(255,0,0);
        else ofSetColor(255);
        ofFill();
        
//        ofPath arc_;
//        //arc_.moveTo(PHEIGHT/2,PHEIGHT/2);
//        arc_.arc(PHEIGHT/2,PHEIGHT/2,_rad,_rad,_angle-_arc_angle/2,_angle+_arc_angle/2);
        
        ofPolyline arc_;
        arc_.arc(PHEIGHT/2,PHEIGHT/2,_rad,_rad,_angle-_arc_angle/2,_angle+_arc_angle/2,500);
////
        ofPushMatrix();
//        ofTranslate(PHEIGHT/2,PHEIGHT/2);
//        ofRotate(_angle-_arc_angle/2);
        if(_trigger) ofSetColor(255,0,0,120);
        else ofSetColor(255,120);
        ofSetLineWidth(ArcWidth);
        
        arc_.draw();
        
        
        
        ofPopMatrix();
        
        ofPopStyle();
    }
    bool intersect(Blob& b){
//        cv::Point corner_[4];
//        corner_[0]=cv::Point(rec_.x-rec_.width/2,rec_.y-rec_.height/2);
//        corner_[1]=cv::Point(rec_.x-rec_.width/2,rec_.y+rec_.height/2);
//        corner_[2]=cv::Point(rec_.x+rec_.width/2,rec_.y+rec_.height/2);
//        corner_[3]=cv::Point(rec_.x+rec_.width/2,rec_.y-rec_.height/2);
        int len=b._contours.size();
        for(int i=0;i<len;++i){
            if(ofDist(b._center.x+b._contours[i].x,b._center.y+b._contours[i].y,PHEIGHT/2,PHEIGHT/2)>=_rad){
                float a_=ofRadToDeg(fmod(TWO_PI+atan2(b._center.y+b._contours[i].y-PHEIGHT/2,b._center.x+b._contours[i].x-PHEIGHT/2),TWO_PI));
                if(abs(a_-_angle)<=_arc_angle/2.0){
                    ofLog()<<"ang_ "<<a_<<" at "<<_angle;
                    return true;
                }
            }
            
        }
        return false;
        
//        if(ofDist(b._center.x,b._center.y,PHEIGHT/2,PHEIGHT/2)>=_rad-b._rad){
//            float a_=ofRadToDeg(fmod(TWO_PI+atan2(b._center.y-PHEIGHT/2,b._center.x-PHEIGHT/2),TWO_PI));
//            if(abs(a_-_angle)<=_arc_angle/2.0){
//                ofLog()<<"ang_ "<<a_<<" at "<<_angle;
//                return true;
//            }
//        }
//        return false;
    
    
    }

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
    static float CenterForce;
    
    //ofColor _center_color;

	DetectBlob(){
		_trigger=false;
		_walk_pos=0;
		_life=LIFESPAN;
		_color=floor(ofRandom(2));
		_danim=FrameTimer(100+ofRandom(50),ofRandom(100));
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
//	void draw(float p_,bool fill_,ofTrueTypeFont& font_){
//
//		if(!_trigger) return;
//        
//        drawTriggered(p_,fill_);
//        if(_draw_text) drawText(p_,fill_,font_);
//    }
    void drawText(float p_,bool fill_,ofTrueTypeFont& font_){
        
            ofPushStyle();
            ofSetColor(255,120);
                ofPushMatrix();
                ofTranslate(_blob._center.x,_blob._center.y);
                    int i=ofClamp(floor(_blob._contours.size()*p_),0,_blob._contours.size()-1);
                    if(fill_) font_.drawString(ofToString(_blob._contours[i].x,2)+","+ofToString(_blob._contours[i].y,2),0,0);
                    else font_.drawString(ofToString(_blob._center.x,2)+","+ofToString(_blob._center.y,2),0,0);
                ofPopMatrix();
            ofPopStyle();
        
        
	}
	void drawSelect(float p_,ofTrueTypeFont& font_){
        
        if(!_trigger) return;
        
		ofPushStyle();
        
        ofPushMatrix();
        ofTranslate(_blob._center.x,_blob._center.y);
        
        //if(fill_) ofScale(p_,p_);
        
		float d_=1.0*_danim.val();
		
        ofPushMatrix();
		ofTranslate(-d_,-d_);
			ofFill();
            ofSetColor(DetectBlob::BColor[1],255.0*p_);
            drawShape(1.0);
		ofPopMatrix();
		
        ofPushMatrix();
		ofTranslate(d_,d_);
			ofFill();
            ofSetColor(DetectBlob::BColor[2],255.0*p_);
        
			drawShape(1.0);
		ofPopMatrix();

        
        ofPopMatrix();
		
		ofPopStyle();
        
        if(_draw_text) drawText(p_,true,font_);
        
	}
    void drawDetect(float p_,ofTrueTypeFont& font_){
        ofPushStyle();
        
        ofPushMatrix();
        ofTranslate(_blob._center.x,_blob._center.y);
        
        //if(fill_) ofScale(p_,p_);
        
        float d_=1.0*_danim.val();
        
        
        ofPushMatrix();
        ofTranslate(d_,d_);
        ofNoFill();
        ofSetColor(DetectBlob::BColor[0],255.0);
        
        drawShape(p_);
        ofPopMatrix();
        
        
        ofPopMatrix();
        
        ofPopStyle();
        
        if(_draw_text) drawText(p_,true,font_);
        
    }
    void drawScan(float p_,ofTrueTypeFont& font_){
        
        if(!_trigger) return;
        
        ofPushStyle();
        
        ofPushMatrix();
        ofTranslate(_blob._center.x,_blob._center.y);
        
        //if(fill_) ofScale(p_,p_);
        
        float d_=1.0*_danim.val();
        
        ofPushMatrix();
        ofTranslate(d_,d_);
        ofFill();
        ofSetColor(DetectBlob::BColor[4],255.0*p_);
        
        drawShape(1);
        ofPopMatrix();
        
        ofPushMatrix();
        ofTranslate(-d_,-d_);
        ofFill();
        ofSetColor(DetectBlob::BColor[3],255.0*p_);
        
        drawShape(1);
        ofPopMatrix();
        
        ofPopMatrix();
        
        ofPopStyle();
        
        if(_draw_text) drawText(p_,true,font_);
        
    }

    void drawBirdBack(){
        
        if(!_trigger) return;
        
        ofPushStyle();
        float d_=2.0*_danim.val();
        
            ofPushMatrix();
            ofTranslate(_blob._center.x,_blob._center.y);
            
            ofPushMatrix();
            ofTranslate(-d_,-d_);
            
            ofSetColor(DetectBlob::BColor[1]);
            ofFill();
                drawShape(1.0);
            ofPopMatrix();
            
            ofPushMatrix();
            ofTranslate(d_,d_);
            ofSetColor(DetectBlob::BColor[0]);
            ofFill();
                drawShape(1.0);
            ofPopMatrix();
            
            ofPopMatrix();
        
        ofPopStyle();
    }
    void drawBird(){
        
        ofPushStyle();
        float d_=1.0*_danim.val();
        
        ofPushMatrix();
        ofTranslate(_floc.x,_floc.y);
        
        ofRotate(ofRadToDeg(atan2(_floc.y,_floc.x)));
        
            ofPushMatrix();
            ofTranslate(0,0);
            ofFill();
            if(_trigger) ofSetColor(DetectBlob::BColor[_color]);
            else ofSetColor(DetectBlob::BColor[(_color+1)%2]);
                drawShape(1.0);
            ofTranslate(d_,-d_);
            if(!_trigger) ofSetColor(DetectBlob::BColor[_color]);
            else ofSetColor(DetectBlob::BColor[(_color+1)%2]);
                drawShape(1.0);
            ofPopMatrix();
        
        ofPopMatrix();
        
        
        ofPopStyle();
    }
    
    void drawBug(ofTrueTypeFont& font_){
       
        ofPushStyle();
        
       
        
        
        ofPushMatrix();
        ofTranslate(_blob._center.x,_blob._center.y);
        if(_trigger) ofSetColor(255,0,0);
        else ofSetColor(255);
        ofNoFill();
            drawShape(1.0);
            //ofDrawRectangle(_blob._bounding.x,_blob._bounding.y,_blob._bounding.width,_blob._bounding.height);
        ofPopMatrix();
        
        ofPopStyle();
        
        drawText(1.0,true,font_);

    }
    
    
	void drawDebug(){
		ofPushStyle();
		ofSetColor(255,0,0);
		ofNoFill();
        
        ofPushMatrix();
        ofTranslate(_blob._center.x,_blob._center.y);
			drawShape(1.0);
        ofPopMatrix();
        
		ofPopStyle();
	}
	void drawShape(float p_){
		int len=_blob._contours.size();//s(int)((float)_blob._npts*p_);
        
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
        
        //_center_color=color_;
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
        
        sep*=2.5;
        ali*=1.0;
        coh*=1.0;
        
        applyForce(sep);
        applyForce(ali);
        applyForce(coh);
        
        
         //boundary
        ofVec2f desired;
         if(_floc.x>PWIDTH){
             applyForce(ofVec2f(-MaxForce,0));
         }else if(_floc.x<0){
             applyForce(ofVec2f(MaxForce,0));
         }
         if(_floc.y>PHEIGHT){
             applyForce(ofVec2f(0,-MaxForce));
         }else if(_floc.y<0){
             applyForce(ofVec2f(0,MaxForce));
         }
        //center
        ofVec2f cent_desired=Center-_floc;
        ofVec2f cent_steer=cent_desired-_fvel;
        cent_steer.limit(MaxForce*CenterForce);
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

class SelectSeq{
public:
    int _count;
    ofVec2f _dir;
    vector<DetectBlob> _blobs;
    FrameTimer _anim;
    int _index;
    int _mselect;
    bool _dead;
    
    SelectSeq(float time_,int count_,int msel_){
        _anim=FrameTimer(time_,time_*ofRandom(.3));
        _count=count_;
        _index=0;
        _anim.restart();
        _dead=false;
        
        _mselect=msel_;
    }
    void update(float dt_){
        _anim.update(dt_);
        for(auto& b:_blobs) b.update(dt_);
    }
    void draw(ofTrueTypeFont _font){
        int len=_blobs.size();
        for(int i=0;i<len;++i){
            _blobs[i].drawSelect((i>_index-_mselect-1)?_anim.val():1.0,_font);
        }
    }
};


#endif
