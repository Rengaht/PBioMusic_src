#pragma once
#ifndef PARMAMETR_H
#define PARAMETER_H


#include "ofMain.h"
#include "ofxXmlSettings.h"

class GlobalParam{

	static GlobalParam* _instance;	

public:

	static string ParamFilePath;
	
	float _blob_small;
	float _blob_large;

	int _mscan_region;
	float _scan_vel;
	float _scan_width;

	float _select_vel;
	int _mselect;

	string _live_addr;

	GlobalParam(){
		readParam();

	}
	static GlobalParam* GetInstance(){
		if(!_instance)
			_instance=new GlobalParam();
		return _instance;
	}
	void readParam(){
		ofxXmlSettings _param;
		bool file_exist=true;
		if(_param.loadFile(ParamFilePath) ){
			ofLog()<<"mySettings.xml loaded!";
		}else{
			ofLog()<<"Unable to load xml check data/ folder";
			file_exist=false;
		}

		_blob_small=_param.getValue("BLOB_SMALL",20);			
		_blob_large=_param.getValue("BLOB_LARGE",.3);

		_mscan_region=_param.getValue("SCAN_REGION_COUNT",10);
		_scan_vel=_param.getValue("SCAN_VEL",2000);
		_scan_width=_param.getValue("SCAN_WIDTH",2);

		_select_vel=_param.getValue("SELECT_VEL",200);
		_mselect=_param.getValue("MSELECT",5);

		 _live_addr=_param.getValue("LIVE_ADDRESS","127.0.0.1");

		if(!file_exist) saveParameterFile();

	
	}
	void saveParameterFile(){


		ofxXmlSettings _param;

		_param.setValue("BLOB_SMALL",_blob_small);
		_param.setValue("BLOB_LARGE",_blob_large);
		_param.setValue("SCAN_REGION_COUNT",_mscan_region);
		_param.setValue("SCAN_VEL",_scan_vel);
		_param.setValue("SCAN_WIDTH",_scan_width);

		_param.setValue("MSELECT",_mselect);
		_param.setValue("SELECT_VEL",_select_vel);
		_param.setValue("LIVE_ADDRESS",_live_addr);

		_param.save(ParamFilePath);


	}


};





#endif