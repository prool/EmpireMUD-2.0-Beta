/* Code by Prool
http://prool.kharkov.org http://mud.kharkov.org https://github.com/prool/EmpireMUD-2.0-Beta
<proolix@gmail.com>
(CC) 2017-2018

Prool functions
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#include "sysdep.h"
#include "structs.h"
#include "prool.h"

char prool_buf [PROOL_LEN];
char prool_buf_tr [PROOL_LEN];

char *deromanize(unsigned char *in, unsigned char *out)
// reverse function for romanize()
// input: transliterated string
// output: cyrillic UTF-8 string
// return: if ok out string, if error 0
{
if (in==0) {/*printf("prooldebug unromanize error 1\n");*/ return 0;}
if (out==0) {/*printf("prooldebug romanize error 1a\n"); */return 0;}
//printf("prooldebug: unromanize: in='%s'\n", in);
if (*in==0) {/*printf("prooldebug unromanize error 2\n"); */return 0;}
*out=0;
while(*in)
	{
	if (*in=='j')
		{
		switch(*++in)
			{
			case 'g': /* ж  */ strcat(out,"ж"); break;
			case 'c': /* ч  */ strcat(out,"ч"); break;
		        case 's': /* ш  */ strcat(out,"ш"); break;
		        case 'w': /* щ  */ strcat(out,"щ"); break;
		        case 'x': /* ъ  */ strcat(out,"ъ"); break;
		        case 'y': /* ы  */ strcat(out,"ы"); break;
		        case 'i': /* ь  */ strcat(out,"ь"); break;
		        case 'e': /* э  */ strcat(out,"э"); break;
		        case 'u': /* ю  */ strcat(out,"ю"); break;
		        case 'a': /* я  */ strcat(out,"я"); break;
		        case 'o': /* ё  */ strcat(out,"ё"); break;
			default: /*printf("prooldebug unromanize error 3\n"); */return 0;
			}
		}
	else if (*in=='J')
		{
		switch(*++in)
			{
			case 'g': /* ж  */ strcat(out,"Ж"); break;
			case 'c': /* ч  */ strcat(out,"Ч"); break;
		        case 's': /* ш  */ strcat(out,"Ш"); break;
		        case 'w': /* щ  */ strcat(out,"Щ"); break;
		        case 'x': /* ъ  */ strcat(out,"Ъ"); break;
		        case 'y': /* ы  */ strcat(out,"Ы"); break;
		        case 'i': /* ь  */ strcat(out,"Ь"); break;
		        case 'e': /* э  */ strcat(out,"Э"); break;
		        case 'u': /* ю  */ strcat(out,"Ю"); break;
		        case 'a': /* я  */ strcat(out,"Я"); break;
		        case 'o': /* ё  */ strcat(out,"Ё"); break;
			default: /*printf("prooldebug unromanize error 3a\n"); */return 0;
			}
		}
	else
		{
		switch(*in)
		{
                case  /* А A */ 'A': strcat(out,"А"); break;
                case  /* Б B */ 'B': strcat(out,"Б"); break;
                case  /* В V */ 'V': strcat(out,"В"); break;
                case  /* Г G */ 'G': strcat(out,"Г"); break;
                case  /* Д D */ 'D': strcat(out,"Д"); break;
                case  /* Е E */ 'E': strcat(out,"Е"); break;
                case  /* З Z */ 'Z': strcat(out,"З"); break;
                case  /* И I */ 'I': strcat(out,"И"); break;
                case  /* Й Y */ 'Y': strcat(out,"Й"); break;
                case  /* К K */ 'K': strcat(out,"К"); break;
                case  /* Л L */ 'L': strcat(out,"Л"); break;
                case  /* М M */ 'M': strcat(out,"М"); break;
                case  /* Н N */ 'N': strcat(out,"Н"); break;
                case  /* О O */ 'O': strcat(out,"О"); break;
                case  /* П P */ 'P': strcat(out,"П"); break;
                case  /* Р R */ 'R': strcat(out,"Р"); break;
                case  /* С S */ 'S': strcat(out,"С"); break;
                case  /* Т T */ 'T': strcat(out,"Т"); break;
                case  /* У U */ 'U': strcat(out,"У"); break;
                case  /* Ф F */ 'F': strcat(out,"Ф"); break;
                case  /* Х H */ 'H': strcat(out,"Х"); break;
                case  /* Ц C */ 'C': strcat(out,"Ц"); break;
                case  /* а a */ 'a': strcat(out,"а"); break;
                case  /* б b */ 'b': strcat(out,"б"); break;
                case  /* в v */ 'v': strcat(out,"в"); break;
                case  /* г g */ 'g': strcat(out,"г"); break;
                case  /* д d */ 'd': strcat(out,"д"); break;
                case  /* е e */ 'e': strcat(out,"е"); break;
                case  /* з z */ 'z': strcat(out,"з"); break;
                case  /* и i */ 'i': strcat(out,"и"); break;
                case  /* й y */ 'y': strcat(out,"й"); break;
                case  /* к k */ 'k': strcat(out,"к"); break;
                case  /* л l */ 'l': strcat(out,"л"); break;
                case  /* м m */ 'm': strcat(out,"м"); break;
                case  /* н n */ 'n': strcat(out,"н"); break;
                case  /* о o */ 'o': strcat(out,"о"); break;
                case  /* п p */ 'p': strcat(out,"п"); break;
		case  /* р   */ 'r': strcat(out,"р"); break;
		case  /* с   */ 's': strcat(out,"с"); break;
		case  /* т   */ 't': strcat(out,"т"); break;
		case  /* у   */ 'u': strcat(out,"у"); break;
		case  /* ф   */ 'f': strcat(out,"ф"); break;
		case  /* х   */ 'h': strcat(out,"х"); break;
		case  /* ц   */ 'c': strcat(out,"ц"); break;
		default: /*printf("prooldebug unromanize error 3b\n"); */return 0;
		}// end switch
		}
	in++;
	}
return out;
}

int romanize(unsigned char *in, unsigned char *out)
// input: text string
// output: if input is UTF-8 cyrillic, then output string, converted to latin letters (transliteration)
// return: 0 - it's ok, 1 - error (f.e. input string is not UTF-8 cyrillic)
{int l, i; unsigned char c;

if (in==0) {/*printf("prooldebug romanize error 1\n"); */return 1;}
if (out==0) {/*printf("prooldebug romanize error 1a\n"); */return 1;}
//printf("prooldebug: romanize: in='%s'\n", in);
if (*in==0) {/*printf("prooldebug romanize error 2\n"); */return 1;}
if ((*in!=0xD0) && (*in!=0xD1)) {/*printf("prooldebug romanize error 3\n"); */return 1;}

l=strlen(in);
if (l%2) {/*printf("prooldebug romanize error 4\n"); */return 1;}

for (i=0;i<l;i++)
	{
	c=*in++;
	if (c==0xD0)
		{
		switch (*in++) {
                case 0x81: /* Ё Jo */ *out++='J'; *out++='o'; break;
                case 0x90: /* А A */ *out++='A'; break;
                case 0x91: /* Б B */ *out++='B'; break;
                case 0x92: /* В V */ *out++='V'; break;
                case 0x93: /* Г G */ *out++='G'; break;
                case 0x94: /* Д D */ *out++='D'; break;
                case 0x95: /* Е E */ *out++='E'; break;
                case 0x96: /* Ж Jg */ *out++='J'; *out++='g'; break;
                case 0x97: /* З Z */ *out++='Z'; break;
                case 0x98: /* И I */ *out++='I'; break;
                case 0x99: /* Й Y */ *out++='Y'; break;
                case 0x9A: /* К K */ *out++='K'; break;
                case 0x9B: /* Л L */ *out++='L'; break;
                case 0x9C: /* М M */ *out++='M'; break;
                case 0x9D: /* Н N */ *out++='N'; break;
                case 0x9E: /* О O */ *out++='O'; break;
                case 0x9F: /* П P */ *out++='P'; break;
                case 0xA0: /* Р R */ *out++='R'; break;
                case 0xA1: /* С S */ *out++='S'; break;
                case 0xA2: /* Т T */ *out++='T'; break;
                case 0xA3: /* У U */ *out++='U'; break;
                case 0xA4: /* Ф F */ *out++='F'; break;
                case 0xA5: /* Х H */ *out++='H'; break;
                case 0xA6: /* Ц C */ *out++='C'; break;
                case 0xA7: /* Ч Jc */ *out++='J'; *out++='c'; break;
                case 0xA8: /* Ш Js */ *out++='J'; *out++='s'; break;
                case 0xA9: /* Щ Jw */ *out++='J'; *out++='w'; break;
                case 0xAA: /* Ъ Jx */ *out++='J'; *out++='x'; break;
                case 0xAB: /* Ы Jy */ *out++='J'; *out++='y'; break;
                case 0xAC: /* Ь Ji */ *out++='J'; *out++='i'; break;
                case 0xAD: /* Э Je */ *out++='J'; *out++='e'; break;
                case 0xAE: /* Ю Ju */ *out++='J'; *out++='u'; break;
                case 0xAF: /* Я Ja */ *out++='J'; *out++='a'; break;
                case 0xB0: /* а a */ *out++='a'; break;
                case 0xB1: /* б b */ *out++='b'; break;
                case 0xB2: /* в v */ *out++='v'; break;
                case 0xB3: /* г g */ *out++='g'; break;
                case 0xB4: /* д d */ *out++='d'; break;
                case 0xB5: /* е e */ *out++='e'; break;
                case 0xB6: /* ж jg */ *out++='j'; *out++='g'; break;
                case 0xB7: /* з z */ *out++='z'; break;
                case 0xB8: /* и i */ *out++='i'; break;
                case 0xB9: /* й y */ *out++='y'; break;
                case 0xBA: /* к k */ *out++='k'; break;
                case 0xBB: /* л l */ *out++='l'; break;
                case 0xBC: /* м m */ *out++='m'; break;
                case 0xBD: /* н n */ *out++='n'; break;
                case 0xBE: /* о o */ *out++='o'; break;
                case 0xBF: /* п p */ *out++='p'; break;
		default: *out=0; /*printf("prolldebug romanize error 4b\n"); */return 1;
		} // end of switch
		}
	else if (c==0xD1)
		{
		switch (*in++) {
			case 0x80: /* р  */ *out++='r'; break;
			case 0x81: /* с  */ *out++='s'; break;
			case 0x82: /* т  */ *out++='t'; break;
			case 0x83: /* у  */ *out++='u'; break;
			case 0x84: /* ф  */ *out++='f'; break;
			case 0x85: /* х  */ *out++='h'; break;
			case 0x86: /* ц  */ *out++='c'; break;
			case 0x87: /* ч  */ *out++='j'; *out++='c'; break;
			case 0x88: /* ш  */ *out++='j'; *out++='s'; break;
			case 0x89: /* щ  */ *out++='j'; *out++='w'; break;
			case 0x8A: /* ъ  */ *out++='j'; *out++='x'; break;
			case 0x8B: /* ы  */ *out++='j'; *out++='y'; break;
			case 0x8C: /* ь  */ *out++='j'; *out++='i'; break;
			case 0x8D: /* э  */ *out++='j'; *out++='e'; break;
			case 0x8E: /* ю  */ *out++='j'; *out++='u'; break;
			case 0x8F: /* я  */ *out++='j'; *out++='a'; break;
			case 0x91: /* ё  */ *out++='j'; *out++='o'; break;
		default: *out=0; /*printf("prolldebug romanize error 4c\n"); */return 1;
		} // end of switch
		}
	else if (c==0) break;
	else {/*printf("prooldebug romanize error 4 c='%c' [%02X]\n",c,c); */return 1;}
	}

*out=0;
return 0;
}

int tran_s(char *input, char *output, char_data *ch) // foolish translator (interpreter)
// input: input - english string
// output - translated string, or original, if can't translate (not found in dictionary)
// return value - 0 if ok, 1 if no translate
{
FILE *ff;
char *cc;
char osnova [PROOL_LEN];
int l,crlf;

//printf("prool tr() input='%s'\n", input);

/*
cc=getcwd(prool_buf, PROOL_LEN);
//printf("prool tr() pwd='%s'\n", cc);
*/

if (ch->player_specials->prooltran[2]==0) {
	strcpy(output, input);
	return 1;
}

ff=fopen("slovar.csv","r"); // open dictionary file
if (ff==0) {
	printf("slovar not opened\n");
	strcpy(output, input);
	return 1;
}
else {
//printf("slovar open!\n");
}

crlf=0;
strcpy(osnova,input);
l=strlen(osnova);
if (osnova[l-1]=='\n') {osnova[l-1]=0; crlf=1;}
l=strlen(osnova);
if (osnova[l-1]=='\r') {osnova[l-1]=0; crlf=1;}
l=strlen(osnova);

while(fgets(prool_buf_tr,PROOL_LEN,ff)) {
cc=strchr(prool_buf_tr,0x0A);
if (cc) *cc=0;
if (prool_buf_tr[0]==0) continue;
//printf("fgets='%s'\n", prool_buf_tr);
if (!memcmp(osnova,prool_buf_tr,l)) {
	cc=strchr(prool_buf_tr,',');
	if (cc) if (*++cc) {
	strcpy(output, cc);
	if (crlf) strcat(output,"\r\n");
	return 0;
	}
}
} // end while

fclose(ff);
//printf("prool tr_s() can't translate '%s'\n", osnova);
strcpy(output, input);
return 1;
} // end tr()

/****************************************************/
char *prool_translator_2 (char *input, char *out, char_data *ch)
{
char buffer [PROOL_LEN];
char perevod [PROOL_LEN];
char *cc, *cc2, *cc3;
int bi; // bilingua mode

//return "[prool fool!]";

bi=0;
if (input==0) return "[prool: null string]";
if (*input==0) return "[prool: empty string]";
if (ch) bi=ch->player_specials->prooltran[3]; 

//printf("prool tr2 in='%s' [", input);

#if 0 // debug print
cc=input;
while (*cc) printf ("%02X ",*cc++);
printf(" ] \n");
#endif

buffer[0]=0;
out[0]=0;

if (input[0]=='\b') { // строки с таким символом в начале не переводятся!
	strcpy(out,input+1);
	return out;
}

#if 0 // prool: временно уберем
if (tran_s(input,buffer,ch)==0)
	{// translation ok
	strcpy(out,buffer);
	return out;
	}
#endif

if (ch->player_specials->prooltran[1]==0) {
	strcpy(out,input);
	return out;
}

// деление строки на слова 
cc=input;
while (1) {
cc2=strchr(cc,' ');
if (cc2==0) {// последнее слово
	//printf("prool w last='%s'\n", cc);
	perevod[0]=0;
	poisk(cc,perevod,bi);
	strcat(out,perevod);
	break;
}
strcpy(buffer,cc);
cc3=strchr(buffer,' ');
if (cc3) *cc3=0;
//printf("prool w='%s'\n", buffer);
perevod[0]=0;
poisk(buffer,perevod,bi);
strcat(out,perevod);
strcat(out," ");
cc=cc2+1;
if (*cc==0) break;
}

//printf("prool tr2 out='%s'\n", out);
return out;
} // end prool_translator_2

void poisk (char *in, char *out, int bi) // поиск слова в словаре Мюллера
{
FILE *fp;
char buf[PROOL_LEN];
char buf2[PROOL_LEN];
char prefix[PROOL_LEN];
char suffix[PROOL_LEN];
char slovo[PROOL_LEN];
char *cc;
int i, l, count, index, busstop, ampersend, first_upper, all_upper;

// слова, которые в принципе не переводятся
l=strlen(in);

if (in[0]==0) goto l1;
if (in[0]=='|') goto l1;
if (in[0]=='-') goto l1;
if (in[0]=='+') goto l1;
if ((in[0]=='&')&&(l==2)) goto l1;
if (strchr(in,'|')) goto l1;
if (strchr(in,'/')) goto l1;
if (strchr(in,'~')) goto l1;
if (strchr(in,'^')) goto l1;
if (strchr(in,'\\')) goto l1;

count=0; 
for (i=0;i<l;i++) if (in[i]=='.') count++;
if (count>1) goto l1;

count=0; 
for (i=0;i<l;i++) if (in[i]=='i') count++;
if (count>2) goto l1;

count=0; 
for (i=0;i<l;i++) if (in[i]=='o') count++;
if (count>2) goto l1;

count=0; 
for (i=0;i<l;i++) if (in[i]==':') count++;
if (count>2) goto l1;

// выделение префикса (предшествующей слову небуквенной строки, например кода смены цвета или кавычек
l=strlen(in);
index=0;
prefix[0]=0;
suffix[0]=0;
slovo[0]=0;
ampersend=0;

for (i=0;i<l;i++)
	{
	if (ampersend)
		{// после амперсенда идет цифра или буква, но это код цвета, то это часть префикса! хоть и буква!!
		prefix[index++]=in[i];
		ampersend=0;
		}
	else if (((in[i]>='a')&&(in[i]<='z')) || ((in[i]>='A')&&(in[i]<='Z')))
		{// єто буква
		prefix[index]=0;
		break;
		}
	else
		{// это не буква: значит это часть префикса
		prefix[index++]=in[i];
		if (in[i]=='&') ampersend=1;
		}
	}

prefix[index]=0;
busstop=i;

// выделение слова
index=0;
for (i=busstop; i<l; i++)
	{
	if (((in[i]>='a')&&(in[i]<='z')) || ((in[i]>='A')&&(in[i]<='Z')))
		{// єто буква
		slovo[index++]=in[i];
		}
	else
		{// это не буква
		slovo[index]=0;
		break;
		}
	}

slovo[index]=0;

busstop=i;

//выделение суффикса (следующей за словом небуквенной строки, например, точки, или кода смены цвета или точки и кода смены цвета

index=0;
for (i=busstop; i<l; i++)
	{
	if (((in[i]>='a')&&(in[i]<='z')) || ((in[i]>='A')&&(in[i]<='Z')))
		{// єто буква
		suffix[index]=0;
		break;
		}
	else
		{// это не буква
		suffix[index++]=in[i];
		}
	}
suffix[index]=0;

//printf("Деление слова '%s' == '%s' '%s' '%s'\n", in, prefix, slovo, suffix);

if (slovo[0]==0) goto l1; // слова нет, это небуквенное "псевдослово", не переводим

// анализ первой буквы слова, большая ли она
first_upper=0;
if (isupper(slovo[0])) first_upper=1;

// а может всё слово большими (заглавными) буквами?

l=strlen(slovo);
all_upper=1;
for (i=0;i<l;i++) if (islower(slovo[i])==0) all_upper=0;

// приводим всё слово к нижнему регистру
for (i=0;i<l;i++) slovo[i]=tolower(slovo[i]);


fp=fopen("slovar2.txt","r");
if (fp==NULL) {/*printf("Can't open slovar2.txt\n");*/ goto l1;}

while(1)
	{
	buf[0]=0;
	fgets(buf,PROOL_LEN,fp);
	if (buf[0]==0) break;
	cc=strchr(buf,0x0A);
	if (cc) *cc=0;
	cc=strchr(buf,'#');
	if (cc==0) continue;
	strcpy(buf2,cc+1);
	*cc=0;
	if (!strcmp(slovo/*in*/,buf))
		{
		//strcpy(out,buf2);
		out[0]=0;
		if (prefix[0]) strcpy(out,prefix);
		if (bi)
			{
			if (buf2[0]) strcat(out,buf2);
			strcat(out,"(");
			strcat(out,slovo);
			strcat(out,")");
			}
		else
			if (buf2[0]) strcat(out,buf2);
		if (suffix[0]) strcat(out,suffix);
		printf("'%s'->'%s'\n", in, out);
		return;
		}
	}

fclose(fp);
printf("'%s'-> ????\n", in);
l1:;
strcpy(out,in);

} // end poisk

void prool_init(void)
{FILE *f;

//printf("prool: cwd=%s\n", get_current_dir_name());
#define ACC_NAME "lib/players/accounts/index"

f=fopen(ACC_NAME,"r");

if (!f)
	{// file not found. create
	//printf("prool: acc file not found\n");
	f=fopen(ACC_NAME,"w");
	if (f)	{
		fputs("$",f);
		fclose(f);
		}
	}

#define EMPIRE_NAME "lib/empires/index"

f=fopen(EMPIRE_NAME,"r");

if (!f)
	{// file not found. create
	f=fopen(EMPIRE_NAME,"w");
	if (f)	{
		fputs("$",f);
		fclose(f);
		}
	}

} // end of prool_init()

char *ptime(void) // Возвращаемое значение: ссылка на текстовую строку с текущим временем // from Virtustan MUD
	{
	char *tmstr;
	time_t mytime;

	mytime = time(0);

	tmstr = (char *) asctime(localtime(&mytime));
	*(tmstr + strlen(tmstr) - 1) = '\0';

	return tmstr;

	}

void prool_log(char *str) // from Virtustan MUD
{
FILE *fp;
fp=fopen("empire-prool.log", "a");
fprintf(fp,"%s %s\n",ptime(),str);
fclose(fp);
}
