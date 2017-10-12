#include "ofMain.h"
#include "ofApp.h"



//========================================================================


string GlobalParam::ParamFilePath="_param.xml";
ofVec2f* PacMan::Direction=new ofVec2f[5]{ofVec2f(1,0),ofVec2f(1,1),ofVec2f(0,1),ofVec2f(-1,1),ofVec2f(-1,0)};
ofVec2f* PacMan::GDirection=new ofVec2f[5]{ofVec2f(1,0),ofVec2f(1,-1),ofVec2f(0,-1),ofVec2f(-1,-1),ofVec2f(-1,0)};



float PacMan::Rad=3;
float PacMan::Vel=2;
int PacMan::MPathRecord=20;
ofColor* PacMan::GColor=new ofColor[4]{ofColor(0,255,222),ofColor(255,184,222),ofColor(255,184,71),ofColor(255,0,0)};
float PacMan::CornerAngle=30;

//ofColor* DetectBlob::BColor=new ofColor[5]{ofColor(0,230,240),ofColor(0,255,0),ofColor(255,255,0),ofColor(255,104,0),ofColor(255,20,171)};
ofColor* DetectBlob::BColor=new ofColor[5]{ofColor(20,36,255),ofColor(242,42,78),ofColor(75,255,56),ofColor(85,255,200),ofColor(206,0,245)};
ofVec2f DetectBlob::Center=ofVec2f(PHEIGHT/2,PHEIGHT/2);
float DetectBlob::MaxSpeed=12;
float DetectBlob::MaxForce=.22;
float DetectBlob::CenterForce=2.0;

int* ofApp::SoundTrackCount=new int[5]{10,5,6,13,5};
int ofApp::mSelectBlob=5;

int ofApp::mBugBox=8;
float BugBox::ArcWidth=5;


int main( ){
	ofSetupOpenGL(1280,720,OF_WINDOW);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}
