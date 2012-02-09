/*
  cppagi.h
  Robert Oliveira, 2005.01.29
  olivecoder@gmail.com
*/
#ifndef _CPPAGI_H_
#define _CPPAGI_H_

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define AGI_CODEOK 200
#define AGI_SOUNDSPATH "/var/lib/asterisk/sounds"
#define AGI_ERROR 3
#define AGI_WARNING 6
#define AGI_INFO 9


#ifdef DEBUG
#define AGI_LOG(msg, verbosity) log(msg,verbosity,__FILE__,__FUNCTION__,__LINE__)
#define AGI_THROW(msg) throw error(agiobj, msg, __FILE__, __FUNCTION__, __LINE__)
#else
#define AGI_LOG(msg, verbosity) log(msg, verbosity)
#define AGI_THROW(msg) throw error(msg)
#endif

using namespace std;

class agi_error;

class agi {

private:
	vector<string> environment;
	ostringstream request;
	string response, dtmf_buffer, execCommand;
	int code, result;
	bool hangup_detected;

public:
	
    // basic methods
	agi();
	virtual ~agi();
	void clear(void);
	int exec(const string &command);
	int read(void);
	int getCode(void);
	int getResult(void);
	string getData(void);
	string getEnv(const string &varname);
	bool isConnected(void);	
	bool fail(void);
	void log(const string& message, int verbosity=AGI_INFO, const string &file="", 
		const string &function="", int line=0);
	agi_error error( const string& msg, const string& file="", 	
		const string& function="", int line=0);

	// class methods
	static string ltime(time_t t=0);
	static string upper(const string &s);
    static string sprintf(const std::string &fmt, ...);

	// agi commands
	void verbose(const string &message, int verbosity=3);
	void answer(void);
	void play(const string &file, 
		const string valid_digits="1234567890*#");
	void record( const string &file, const string &format="gsm", 
		int timeout=5000, int silence=0);
	string getDtmf(int length=1, int timeout=4000, char terminator='#');
	string input(const string &prompt, int length=1, int timeout=4000, 
		char terminator='#');
	char menu( const string& prompt, const string& options, 
		const string& error="", const string& retry_message="", int retry=3);
	void jump(const string &context, const string &extension, int priority=1);
	void sayDigits(const string &digits);
	void sayNumber(const string &number);
	void saySetsOfTen(const string &digits);
	void sayTime(string timestr="NOW");
	bool hangup(int cause=17);
	int getChannelStatus(const string &channel="");
	void setCallerId(const string &cid);
	void setLanguage(const string &language="br");
	string getLanguage(void);
	string dbGet(const string family, const string key);
	void dbPut(const string family, const string key, const string value);
	void dbDel(const string family, const string key);
	string getVar(const string &var);
	void setVar(const string &var, const string &value);
};

// AGI exception class
class agi_exception: public exception 
{
private:
	string msg;
public:
	agi_exception(const string &msg) throw(): exception() {
		this->msg=msg;
	} 
	virtual ~agi_exception() throw() {}
	virtual const char* what() {
		return msg.c_str();
	}
};

class agi_hangup: public agi_exception 
{
public:
	agi_hangup(const string &msg) : agi_exception(msg) {}
};

class agi_error : public agi_exception
{
private:
	string file, function;
	int line;
	bool debug;
	agi *agiobj;
public:
	agi_error( agi& agiobj, const string& msg, const string& file="", 	
		const string& function="", int line=0 ) throw() : 
		agi_exception(msg) 
	{
		this->agiobj = &agiobj;
#ifdef DEBUG
		this->debug = true;
		this->file = file;
		this->function = file;
		this->line = line;
		this->agiobj->log( msg, 0, file, function, line);
#else
		this->debug = false;
		this->file = "";
		this->function = "";
		this->line = line;
		this->agiobj->log( msg, AGI_ERROR );
#endif
	}
	virtual ~agi_error() throw() {}

	string getFile(void) { return file; }
	string getFunction(void) { return function; }
	int getLine(void) { return line; }
};

#endif
