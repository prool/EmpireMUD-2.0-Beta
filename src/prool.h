/* Code by Prool
http://prool.kharkov.org http://mud.kharkov.org https://github.com/prool/EmpireMUD-2.0-Beta
<proolix@gmail.com>
(C) 2017

Function prototypes
*/

#define PROOL_LEN 255

extern char prool_buf[];
extern char prool_buf_tr[];

char *deromanize(unsigned char *in, unsigned char *out);
int romanize(unsigned char *in, unsigned char *out);
char *tr(char *str);
