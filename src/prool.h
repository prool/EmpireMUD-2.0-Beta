/* Code by Prool
http://prool.kharkov.org http://mud.kharkov.org https://github.com/prool/EmpireMUD-2.0-Beta
<proolix@gmail.com>
(C) 2017

Function prototypes
*/

#define PROOL_LEN 2048 /*255*/

extern char prool_buf[];
extern char prool_buf_tr[];
extern int prool_tr; // 1 - translator enable. 0 - disable
extern int prool_tr_w; // 1 - word translator enable. 0 - disable
extern int prool_tr_s; // 1 - string translator enable. 0 - disable

char *deromanize(unsigned char *in, unsigned char *out);
int romanize(unsigned char *in, unsigned char *out);
int tran_s(char *input, char *output);
char *prool_translator_2 (char *input, char *out);
void poisk (char *in, char *out);
void prool_init(void);
