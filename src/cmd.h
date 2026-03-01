
#ifndef _CMD_H_
#define _CMD_H_




#define CL ""
#define CR "\n"
#define CS(a)	CL a CR




void cmd_init ();
int cmd_task (const int pulse);


void cmdSendError (const char *err);
void cmdSendResponse (const char *msg);


#ifdef __cplusplus
extern "C" {
#endif

int cmdLoadUbx (const char *filename);


#ifdef __cplusplus
}
#endif




typedef struct {
	struct {
		uint32_t read;
		uint32_t expected;
	}length;
	char *pending;
}file_trans_t;

typedef struct {
	uint8_t sec;   // 0-59
	uint8_t min;   // 0-59
	uint8_t hour;  // 0-23
	uint8_t wday;  // 0-6, 0=sunday
	
	uint8_t mday;  // 1-31
	uint8_t mon;   // 0-11
	uint16_t year;  // 70-206, 70=1970, 206=2106
	
	uint32_t length;	// file length
}__attribute__((packed))cmd_fileMeta_t;

typedef struct {
	const char *cmd;
	void (*func)(char *msg, const int msgLen);
	const char *helpStr;
}cmdstr_t;


#endif
