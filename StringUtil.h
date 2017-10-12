
#ifndef StringUtil_h
#define StringUtil_h


inline std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}
inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

// trim trailing spaces
inline string trimStringRight(string str) {
    size_t endpos = str.find_last_not_of(" \t\r\n");
    return (string::npos != endpos) ? str.substr( 0, endpos+1) : str;
}

// trim trailing spaces
inline string trimStringLeft(string str) {
    size_t startpos = str.find_first_not_of(" \t\r\n");
    return (string::npos != startpos) ? str.substr(startpos) : str;
}

inline string trimString(string str) {
    return trimStringLeft(trimStringRight(str));;
}


inline vector<string> readSerialString(ofSerial &serial, char until){
    
    
    static string str;
    stringstream ss;
    char ch;
    int ttl=1000;
    while ((ch=serial.readByte())>0 && ttl-->0 && ch!=until) {
        if(ch!='\n' && ch!='\r') ss << ch;
    }
    str+=ss.str();
    //    cout<<"serial read:"<<str<<endl;
    
    if (ch==until) {
        string tmp=str;
        str="";
        
        string read_string=trimString(tmp);
        /*size_t startpos=output.find_first_of("1234567890");
         if(string::npos != startpos) output=output.substr(startpos);
         */
        //cout<<"get: "<<output<<endl;
        vector<string> string_val=split(read_string,'|');
        
        return string_val;
    } else {
        return vector<string>();
    }
}


#endif /* StringUtil_h */
