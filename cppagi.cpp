/*
  cppagi.cpp
  Robert Oliveira, 2005.02.03
  olivecoder@gmail.com
*/

#include <string>
#include <cctype>
#include <algorithm>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include "cppagi.h"

agi::agi() 
{
  char line[256];
  int max=200;

  AGI_LOG("agi constructor", AGI_INFO);
  setlinebuf(stdout);
  setlinebuf(stderr);
  while ( cin && max-- ) {
    cin.getline(line,sizeof(line)-1);
    if ( strlen(line)==0 ) break;
    environment.push_back(line);
  }
  hangup_detected=false;
  clear();
}

agi::~agi() 
{
  AGI_LOG("agi destructor", AGI_INFO);
}

char char_toupper(char c)
{
  return std::toupper(c);
}

string agi::upper(const string &s)
{
  string result=s;
  transform(result.begin(), result.end(), result.begin(), char_toupper);
  return result;
}

void agi::log(const string& message, int verbosity, const string &file, 
	      const string &function, int line )
{
  string where, msg;
#ifdef DEBUG
  char strline[16];

  snprintf(strline,sizeof(strline),"%d",line);
  where = file + ", ";
  where += function + "(";
  where += strline;
  where += ")\t";
  msg = where + message;
#else
  msg = message;
#endif
  verbose(agi::ltime() + " " + msg, verbosity);
}

agi_error agi::error( const string& msg, const string& file, 	
		      const string& function, int line) 
{
  agi_error newError(*this, msg, file, function, line);
  return newError;
}

string agi::getEnv(const string &varname)
{
  vector<string>::iterator i;
  string uppervarname;
  size_t p;

  uppervarname = upper(varname);
  for( i=environment.begin(); i!=environment.end(); i++ ) {
    p = i->find(':',0);
    if ( p!=string::npos && p!=0 ) {
      if ( upper(i->substr(0,p))==uppervarname ) {
	p++; p++;
	if ( p < i->size() ) {
	  return i->substr( p, i->size() - p );
	}
      }
    }
  }
  AGI_LOG("variable \""+varname+"\" not found!",AGI_WARNING);
  return "";
}

string agi::ltime(time_t t) 
{
  tm *ptm;
  char strtime[64];
  if (t==0) t=time(NULL);
  ptm=localtime(&t);
  snprintf(strtime,sizeof(strtime),"%02d.%02d.%02d %02d:%02d:%02d",
	   ptm->tm_year-100,ptm->tm_mon,ptm->tm_mday,
	   ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
  return string(strtime);
}

void agi::clear(void) {
  code=AGI_CODEOK;
  result=0;
  dtmf_buffer="";
}

int agi::exec(const string &command) 
{
  execCommand = command;
  cout << command << endl;
  return read();
}

int agi::read(void) {
  char line[256];
  char *next;

  code = result = 0;
  cin.getline(line,sizeof(line)-1);
  response = line;
  code = strtol(line,&next,10); 
  if ( next!=NULL ) { 
    while (*next && *next!='=') next++;
    if ( *next=='=' ) next++;
    result = strtol(next,&next,10); 
  }
  // throw only on first hangup indication
  if ( result == -1 && ! hangup_detected ) {
    hangup_detected = true;
    throw agi_hangup("Hangup on "+execCommand);
  } 
  return result;
}

int agi::getCode(void) 
{
  return code;
}

int agi::getResult(void) 
{
  return result;
}

bool agi::isConnected(void) 
{	
  getChannelStatus();
  return (! hangup_detected);
}

string agi::getData(void)  // todo: split data
{
  size_t begin, end;
  begin = response.find('(');
  end = response.rfind(')');
  if ( begin!=string::npos && end!=string::npos ) {
    return response.substr(begin+1,end-begin-1);
  } else { 
    return "";
  }
}

bool agi::fail(void) {
  return ( (code!=AGI_CODEOK) || (result==-1) );
}

void agi::verbose(const string &message, int verbosity) 
{
  request.str("");
  request << "VERBOSE \"" << message << "\" " << verbosity;
  exec(request.str());
}

void agi::answer(void) 
{
  exec("ANSWER");
}

void agi::play(const string &file, const string valid_digits ) 
{
  int dtmf;

  request.str("");
  request << "STREAM FILE \"" << file << "\" ";
  if ( valid_digits.size()==0 ) {
    request << " \"\" ";
  } else {
    request << " \"" << valid_digits << "\" "; 
  }
  exec(request.str());
  dtmf = getResult();
  if ( valid_digits.find((char)dtmf,0) != string::npos ) {
    dtmf_buffer += (char)dtmf;
  } 
}


void agi::record(const string &file, const string &format, int timeout, 
		 int silence) 
{
  request.str("");
  request << "RECORD FILE " << file << " " << format << " 1# ";
  request << timeout << " beep";
  if ( silence ) { 
    request << " s=" << silence;
  }
  exec(request.str());
}

string agi::getDtmf(int length, int timeout, char terminator) 
{
  string digits = "";
  char ch=-1;
  
  while ( length!=0 && ch!=0 && ch!=terminator && !fail() ) {
    AGI_LOG("WAIT FOR DIGIT (received: \""+digits+"\")",AGI_INFO);
    if ( dtmf_buffer.size() > 0 ) {
      ch = dtmf_buffer[0];
      dtmf_buffer.erase(0,1);
    } else {
      request.str("");
      request << "WAIT FOR DIGIT " << timeout;
      exec(request.str());
      ch = (char) result; // 0=timeout -1=fail 
    }
    if ( ch!=0 && ch!=terminator && !fail() ) {
      digits += ch;
      length--;
    }
  }
  return digits;
}

string agi::input(const string &prompt, int length, int timeout, 
		  char terminator) 
{
  play(prompt);
  return getDtmf(length, timeout, terminator);
}


char agi::menu( const string &prompt, const string &options, 
		const string &error, const string &retry_message, int retry)
{
  char dtmf='\0';
  bool success=false;

  if ( dtmf_buffer.size() > 0 ) {
    dtmf = getDtmf(1)[0];
    success = ( options.find(dtmf,0) != string::npos );
  }
  if ( ! retry > 0 ) {
    AGI_LOG("menu assertion (retry > 0) failed", AGI_WARNING);
  }
  while ( ! success && retry-- && ! fail() ) {
    play(prompt);
    dtmf = getDtmf(1)[0];
    success = ( options.find(dtmf,0) != string::npos );
    if ( ! success ) {
      if ( error.size()!=0 ) {
	play(error);
      }
      if ( retry && retry_message.size()!=0 ) {
	play(retry_message);
      }
    }
  }
  if ( success ) {
    return dtmf;
  } else {
    return 0;
  }
}


void agi::jump(const string &context, const string &extension, int priority) 
{
  exec("SET CONTEXT " + context);
  if ( ! fail() ) { 
    exec("SET EXTENSION " + extension);
    if ( ! fail() ) {
      request.str("");
      request << "SET PRIORITY " << priority;
      exec(request.str());
    }
  }
}

void agi::sayDigits(const string &digits) 
{
  exec("SAY DIGITS " + digits + " #");
}

void agi::sayNumber(const string &number)
{
  exec("SAY NUMBER " + number + " #");
}

void agi::saySetsOfTen(const string &digits)
{
  string number;
  for (size_t i=0; i<digits.size(); i++, i++) {
    number = digits.substr(i,2);
    if ( number[0]=='0' && number.size()==2 ) {
      sayDigits(number.substr(0,1));
      sayDigits(number.substr(1,1));
    } else if ( number[0]=='1' && number.size()==2 ) {
      sayNumber(number);
    } else if ( number[1]=='0' && number.size()==2 ) {
      sayNumber(number);
    } else if ( number.size()==2 ) {
      sayNumber(number.substr(0,1)+"0");
      play("digits/and");
      sayDigits(number.substr(1,1));
    } else { 
      sayNumber(number);
    }
  }
}

void agi::sayTime(string timestr) 
{
  timestr = upper(timestr);
  if ( timestr=="NOW" ) {
    time_t now;
    now = time(NULL);
    timestr=ctime(&now);
  }
  exec("SAY TIME " + timestr );
}

bool agi::hangup(int cause) 
{
	char cause_str[16];
	snprintf(cause_str,sizeof(cause_str),"%i",cause);
	setVar("PRI_CAUSE",cause_str);
	return (exec("HANGUP")==1);
}

int agi::getChannelStatus(const string &channel) 
{
  string qryChannel;
  if ( channel.size()==0 ) {
    qryChannel = getEnv("agi_channel");
  } else {
    qryChannel = channel;
  }
  exec("CHANNEL STATUS "+qryChannel);
  return result;
}

void agi::setCallerId(const string &cid) 
{
  exec("SET CALLERID "+cid);
}


void agi::setLanguage(const string &language)
{
  exec("EXEC SetLanguage "+language);
}

string agi::getLanguage(void) 
{
  return getEnv("agi_language");
}

string agi::dbGet(const string family, const string key) 
{
  exec("DATABASE GET \"" + family + "\" \"" +key+ "\"");
  return getData();
}

void agi::dbPut(const string family, const string key, const string value) 
{
  exec("DATABASE PUT \"" + family + "\" \"" +key+ "\" \""+value+"\"" );
}

void agi::dbDel(const string family, const string key)
{
  exec("DATABASE DEL \"" + family + "\" \"" +key+ "\"");
}

string agi::getVar(const string &var)
{
  exec("GET VARIABLE \""+var+"\"");
  return getData();
}

void agi::setVar(const string &var, const string &value) 
{
  exec("SET VARIABLE \""+var+"\" \""+value+"\"");
}

string agi::sprintf(const std::string &fmt, ...)
{
	std::string result;
	char buffer[8192];
	va_list ap;

	va_start(ap, fmt);
	memset(buffer,0,sizeof(buffer));
	vsnprintf(buffer,sizeof(buffer),fmt.c_str(),ap);
	va_end(ap);
	result = buffer; 

	return result;
}

