#include "ofMain.h"
#include "ofApp.h"



//========================================================================


string GlobalParam::ParamFilePath="_param.xml";
ofVec2f* PacMan::Direction=new ofVec2f[5]{ofVec2f(1,0),ofVec2f(1,1),ofVec2f(0,1),ofVec2f(-1,1),ofVec2f(-1,0)};
ofVec2f* PacMan::GDirection=new ofVec2f[5]{ofVec2f(1,0),ofVec2f(1,-1),ofVec2f(0,-1),ofVec2f(-1,-1),ofVec2f(-1,0)};



float PacMan::Rad=6;
int PacMan::MPathRecord=6;
ofColor* PacMan::GColor=new ofColor[4]{ofColor(0,255,222),ofColor(255,184,222),ofColor(255,184,71),ofColor(255,0,0)};


//ofColor* DetectBlob::BColor=new ofColor[5]{ofColor(0,230,240),ofColor(0,255,0),ofColor(255,255,0),ofColor(255,104,0),ofColor(255,20,171)};
ofColor* DetectBlob::BColor=new ofColor[2]{ofColor(0,255,255),ofColor(255,38,64)};
ofVec2f DetectBlob::Center=ofVec2f(PHEIGHT/2,PHEIGHT/2);
float DetectBlob::MaxSpeed=8;
float DetectBlob::MaxForce=.2;

int main( ){
	ofSetupOpenGL(1280,720,OF_WINDOW);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}
