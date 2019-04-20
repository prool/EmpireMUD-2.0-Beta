/* Code by Prool
http://prool.kharkov.org http://mud.kharkov.org https://github.com/prool/EmpireMUD-2.0-Beta
<proolix@gmail.com>
(CC) 2017-2019

Function prototypes
*/

#define PROOL_LEN 12000

extern char prool_buf[];
extern char prool_buf_tr[];
extern int log_status;

#if 0 // prool: obsolete
extern int prool_tr; // 1 - translator enable. 0 - disable
extern int prool_tr_w; // 1 - word translator enable. 0 - disable
extern int prool_tr_s; // 1 - string translator enable. 0 - disable
#endif

char *deromanize(unsigned char *in, unsigned char *out);
int romanize(unsigned char *in, unsigned char *out);
int tran_s(char *input, char *output, char_data *ch);
char *prool_translator_2 (char *input, char *out, char_data *ch);
void poisk (char *in, char *out, int bi);
void prool_init(void);
char *ptime(void);
void prool_log(char *str);

void koi_to_utf8(char *str_i, char *str_o);
void utf8_to_koi(char *str_i, char *str_o);
void utf8_to_win(char *str_i, char *str_o);
void coder (char *str, int table);
void coder2 (char *str, char *out, int table);
