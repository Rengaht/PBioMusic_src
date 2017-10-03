//
//  Pacman.h
//  PMicroMusic
//
//  Created by RengTsai on 03/10/2017.
//
//

#ifndef Pacman_h
#define Pacman_h

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


#endif /* Pacman_h */
