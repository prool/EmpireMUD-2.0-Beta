/* ************************************************************************
*   File: evolve.c                                        EmpireMUD 2.0b5 *
*  Usage: map updater for EmpireMUD                                       *
*                                                                         *
*  The program is called by the EmpireMUD server to evolve the world map  *
*  in a separate process, preventing constant lag in the game itself.     *
*                                                                         *
*  EmpireMUD code base by Paul Clarke, (C) 2000-2024                      *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  EmpireMUD based upon CircleMUD 3.0, bpl 17, by Jeremy Elson.           *
*  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* prool: try to insert evolve to MUD */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "conf.h"
#include "sysdep.h"
#include <signal.h>
#include <math.h>
#include "structs.h"
#include "utils.h"
#include "db.h"

/**
* Table of contents:
*  LOCAL DATA
*  PROTOTYPES
*  EVOLUTIONS
*  MAIN
*  I/O FUNCTIONS
*  CROP I/O
*  SECTOR I/O
*  MAP I/O
*  HELPER FUNCTIONS
*  RANDOM GENERATOR
*/

#define DEBUG_MODE  TRUE // prool: set TRUE


 //////////////////////////////////////////////////////////////////////////////
//// LOCAL DATA //////////////////////////////////////////////////////////////

// global vars (these are changed by command line args)
int nearby_distance_prool = 2;	// for nearby evos
int day_of_year_prool = 180;	// for seasons
int water_crop_distance_prool = 4;	// for crops that require water


// light version of map data for this program
struct map_t_prool {
	room_vnum vnum;	
	int island_id;
	sector_vnum sector_type, base_sector, natural_sector;
	crop_vnum crop_type;
	bitvector_t affects;
	bitvector_t flags;	// EVOLVER_ flags
	long sector_time;	// when it became this sector
	
	struct map_t_prool *next;	// next in land
};

struct map_t_prool world_prool[MAP_WIDTH][MAP_HEIGHT];	// world map grid
struct map_t_prool *land_prool = NULL;	// linked list of land tiles
crop_data *crop_table_prool = NULL;	// crop hash table
sector_data *sector_table_prool = NULL;	// sector hash table
FILE *outfl_prool = NULL;	// file for saving data


// these are the arguments to shift_tile() to shift one tile in a direction, e.g. shift_tile(tile, shift_dir_prool[dir][0], shift_dir_prool[dir][1]) -- NUM_OF_DIRS
const int shift_dir_prool[][2] = {
	{ 0, 1 },	// north
	{ 1, 0 },	// east
	{ 0, -1},	// south
	{-1, 0 },	// west
	{-1, 1 },	// nw
	{ 1, 1 },	// ne
	{-1, -1},	// sw
	{ 1, -1},	// se
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};


 //////////////////////////////////////////////////////////////////////////////
//// PROTOTYPES //////////////////////////////////////////////////////////////

bool check_crop_water(struct map_t_prool *tile);
bool check_near_evo_sector(struct map_t_prool *tile, int type, struct evolution_data *for_evo);
bool check_near_evo_sector_flagged(struct map_t_prool *tile, int type, struct evolution_data *for_evo);
int count_adjacent(struct map_t_prool *tile, sector_vnum sect, bool count_original_sect);
int count_adjacent_flagged(struct map_t_prool *tile, sbitvector_t with_flags, bool count_original_sect);
void empire_srandom_prool(unsigned long initial_seed);
crop_data *find_crop(any_vnum vnum);
sector_data *find_sect(any_vnum vnum);
struct evolution_data *get_evo_by_type(sector_vnum sect, int type);
void index_boot_crops();
void index_boot_sectors();
void load_base_map();
double map_distance(struct map_t_prool *start, struct map_t_prool *end);
int number_prool(int from, int to);
int season(struct map_t_prool *tile);
bool sect_within_distance(struct map_t_prool *tile, sector_vnum sect, double distance, bool count_original_sect);
bool sect_flagged_within_distance(struct map_t_prool *tile, sbitvector_t with_flags, double distance, bool count_original_sect);
struct map_t_prool *shift_tile(struct map_t_prool *origin, int x_shift, int y_shift);
void write_tile(struct map_t_prool *tile, sector_vnum old);

// this allows the inclusion of utils.h
void basic_mud_log_prool(const char *format, ...) { }


#define SECTOR_SIZE 512
int prool_copy(char *path1, char *path2) // code from OS Proolix
{int h1, h2, i1, i2; long n, m;
char buf [SECTOR_SIZE];

if((h1=open(path1,O_RDONLY))==-1)
  {
  printf("cp: can't open source\n");
  return -1;
  }

if (access(path2,2)==0)
  if (unlink(path2))
    {printf("cp: can't unlink target\n"); return -1; }

if((h2=open(path2,O_WRONLY|O_CREAT|O_EXCL, 00600))==-1)
  {
  printf("cp: can't open target\n");
  close(h1);
  return -1;
  }
while(1)
  {
  if((n=read(h1,buf,SECTOR_SIZE))==-1) break;
  if(n==0) {m=0; break;}
  if((m=write(h2,buf,n))==-1) break;
  if (m!=n) break;
  }
i1=close(h1);
i2=close(h2);
if (n==-1) {printf("cp: can't read source\n"); return -1;}
if (m==-1) {printf("cp: can't write target\n"); return -1;}
if (m!=n)  {printf("cp: write error (device full or not ready?)\n");
            return -1;}
if (i1==-1) {printf("cp: can't close source\n");return -1;}
if (i2==-1)
{
printf(
"cp: can't close target (device or directory full or device not ready)\n"
);
return -1;
}
return 0; /* OK! */
}

 //////////////////////////////////////////////////////////////////////////////
//// EVOLUTIONS //////////////////////////////////////////////////////////////

/**
* attempt to evolve a single tile
*
* @param struct map_t_prool *tile The tile to evolve.
*/
void evolve_one(struct map_t_prool *tile) {
	sector_data *original;
	struct evolution_data *evo;
	sector_vnum become;
	
	if (IS_SET(tile->affects, ROOM_AFF_NO_EVOLVE)) {
		return;	// never
	}
	
	// find sect
	if (!(original = find_sect(tile->sector_type)) || tile->sector_type == BASIC_OCEAN || !GET_SECT_EVOS(original)) {
		return;	// no sector to evolve
	}
	
	// ok prepare to find one...
	become = NOTHING;
	
	// run some evolutions!
	if (become == NOTHING && IS_SET(tile->flags, EVOLVER_OWNED) && (evo = get_evo_by_type(tile->sector_type, EVO_OWNED))) {
		become = evo->becomes;
	}
	if (become == NOTHING && !IS_SET(tile->flags, EVOLVER_OWNED) && (evo = get_evo_by_type(tile->sector_type, EVO_UNOWNED))) {
		become = evo->becomes;
	}
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_TIMED)) && time(0) > tile->sector_time + evo->value * SECS_PER_REAL_MIN) {
		become = evo->becomes;
	}
	
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_RANDOM))) {
		become = evo->becomes;
	}
	
	// seasons
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_SPRING)) && season(tile) == TILESET_SPRING) {
		become = evo->becomes;
	}
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_SUMMER)) && season(tile) == TILESET_SUMMER) {
		become = evo->becomes;
	}
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_AUTUMN)) && season(tile) == TILESET_AUTUMN) {
		become = evo->becomes;
	}
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_WINTER)) && season(tile) == TILESET_WINTER) {
		become = evo->becomes;
	}
	
	// adjacent-one
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_ADJACENT_ONE))) {
		if (count_adjacent(tile, evo->value, TRUE) >= 1) {
			become = evo->becomes;
		}
	}
	
	// not adjacent
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_NOT_ADJACENT))) {
		if (!check_near_evo_sector(tile, EVO_NOT_ADJACENT, evo)) {
		//	formerly:	if (count_adjacent(tile, evo->value, TRUE) < 1) {
			become = evo->becomes;
		}
	}
	
	// adjacent-many
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_ADJACENT_MANY))) {
		if (count_adjacent(tile, evo->value, TRUE) >= 6) {
			become = evo->becomes;
		}
	}
	
	// near sector
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_NEAR_SECTOR))) {
		if (sect_within_distance(tile, evo->value, nearby_distance_prool, TRUE)) {
			become = evo->becomes;
		}
	}
	
	// not near sector
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_NOT_NEAR_SECTOR))) {
		if (!check_near_evo_sector(tile, EVO_NOT_NEAR_SECTOR, evo)) {
		// formerly: if (!sect_within_distance(tile, evo->value, nearby_distance_prool, TRUE)) {
			become = evo->becomes;
		}
	}
	
	// adjacent/not-adjacent to flagged sector
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_ADJACENT_SECTOR_FLAG))) {
		if (count_adjacent_flagged(tile, evo->value, TRUE) >= 1) {
			become = evo->becomes;
		}
	}
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_NOT_ADJACENT_SECTOR_FLAG))) {
		if (!check_near_evo_sector_flagged(tile, EVO_NOT_ADJACENT_SECTOR_FLAG, evo)) {
			become = evo->becomes;
		}
	}
	
	// near/not-near flagged sector
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_NEAR_SECTOR_FLAG))) {
		if (sect_flagged_within_distance(tile, evo->value, nearby_distance_prool, TRUE)) {
			become = evo->becomes;
		}
	}
	if (become == NOTHING && (evo = get_evo_by_type(tile->sector_type, EVO_NOT_NEAR_SECTOR_FLAG))) {
		if (!check_near_evo_sector_flagged(tile, EVO_NOT_NEAR_SECTOR_FLAG, evo)) {
			become = evo->becomes;
		}
	}
	
	// if no other changes occurred, also check crop water (50% chance)
	if (become == NOTHING && water_crop_distance_prool > 0 && !check_crop_water(tile) && !number_prool(0, 1)) {
		become = tile->base_sector;
	}
	
	// DONE: now change it
	if (become != NOTHING && find_sect(become)) {
		tile->sector_type = become;
	}
}


/**
* Evolutions that spread TO other tiles, rather than FROM. These do not use
* get_evo_by_type because more than one of them is allowed to run at once.
*
* @param struct map_t_prool *tile The tile to spread from.
* @return int The number of adjacent tiles that changed.
*/
int spread_one(struct map_t_prool *tile) {
	sector_vnum become = NOTHING, old;
	struct evolution_data *evo;
	sector_data *st;
	bool done_dir[NUM_2D_DIRS];
	struct map_t_prool *to_room;
	int dir, changed = 0;
	
	
	// load the sector
	if (!tile || !(st = find_sect(tile->sector_type))) {
		return 0;
	}
	
	// initialize done_dir: This keeps us from evolving any adjacent tile more than once
	for (dir = 0; dir < NUM_2D_DIRS; ++dir) {
		done_dir[dir] = FALSE;
	}
	
	// this iterates instead of using get_evo_by_type because more than one 100%
	// evolution can run here
	LL_FOREACH(GET_SECT_EVOS(st), evo) {
		if (evo->type != EVO_SPREADS_TO || (number_prool(1, 10000) > ((int) 100 * evo->percent))) {
			continue;	// wrong type or missed % chance
		}
		
		// try the evo
		for (dir = 0; dir < NUM_2D_DIRS; ++dir) {
			if (done_dir[dir]) {
				continue;	// don't re-evolve a dir we already evolved
			}
			if (!(to_room = shift_tile(tile, shift_dir_prool[dir][0], shift_dir_prool[dir][1]))) {
				continue;	// no room that way
			}
			if (to_room->sector_type != evo->value || IS_SET(to_room->affects, ROOM_AFF_NO_EVOLVE)) {
				continue;	// wrong sect that way, or can't evolve it
			}
			
			// seems valid -- 1 last check
			if (changed > 0 && (number_prool(1, 10000) > ((int) 100 * evo->percent))) {
				continue;	// if this isn't the first valid match, run the percent again for the additional tile
			}
			
			// apply it!
			become = evo->becomes;
			if (become != NOTHING && find_sect(become)) {
				old = to_room->sector_type;
				to_room->sector_type = become;
				write_tile(to_room, old);
				++changed;
				done_dir[dir] = TRUE;
			}
		}
	}
	
	return changed;	// how many were changed
}


/**
* runs the evolutions on the whole map and writes the hint file
*/
void evolve_map(void) {
	struct map_t_prool *tile;
	sector_vnum old;
	int changed = 0;
	
	LL_FOREACH(land_prool, tile) {
		// main evos
		old = tile->sector_type;
		evolve_one(tile);
		
		if (tile->sector_type != old) {
			write_tile(tile, old);
			++changed;
		}
		else {
			// spreader-style evos
			changed += spread_one(tile);
		}
	}
	
	if (DEBUG_MODE) {
		printf("Changed %d tile%s\n", changed, PLURAL(changed));
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// MAIN ////////////////////////////////////////////////////////////////////

int evolve_int() {
	struct map_t_prool *tile;
	int num = 0;

	printf("prool debug: evolve2 internal\n"); // prool
	
	empire_srandom_prool(time(0));

// prool: initialization
land_prool = NULL;	// linked list of land tiles
crop_table_prool = NULL;	// crop hash table
sector_table_prool = NULL;	// sector hash table
outfl_prool = NULL;	// file for saving data
	
			nearby_distance_prool = config_get_int("nearby_sector_distance");
			day_of_year_prool = DAY_OF_YEAR(main_time_info);
			water_crop_distance_prool = config_get_int("water_crop_distance"); // may be 0 to ignore unwatered crops
	//nearby_distance_prool = atoi(argv[1]);
	//day_of_year_prool = atoi(argv[2]);
	//water_crop_distance_prool = atoi(argv[3]);	// may be 0 to ignore unwatered crops
	
	if (DEBUG_MODE) {
		printf("prool int evolve: Using nearby distance of: %d\n", nearby_distance_prool);
		printf("Using day of year: %d\n", day_of_year_prool);
		printf("Using water crop distance: %d\n", water_crop_distance_prool);
	}
	
	// load data
	index_boot_crops();
	index_boot_sectors();
	if (DEBUG_MODE) {
		printf("prool int evolve: Loaded: %d crops\n", HASH_COUNT(crop_table_prool));
		printf("Loaded: %d sectors\n", HASH_COUNT(sector_table_prool));
	}
	
	load_base_map();
	
	if (DEBUG_MODE) {
		LL_COUNT(land_prool, tile, num); 
		printf("prool int evolve: Loaded: %d land tiles\n", num);
	}
	
	// open file for writing
	if (!(outfl_prool = fopen(EVOLUTION_FILE TEMP_SUFFIX, "wb"))) {
		printf("prool int evolve: ERROR: Unable to open evolution file %s\n", EVOLUTION_FILE);
		exit(1);
	}
	
	// evolve data
	evolve_map();
	
	// close file
	fclose(outfl_prool);
	rename(EVOLUTION_FILE TEMP_SUFFIX, EVOLUTION_FILE);
	
	return 0;
}


 //////////////////////////////////////////////////////////////////////////////
//// I/O FUNCTIONS ///////////////////////////////////////////////////////////

/**
* This converts data file entries into bitvectors, where they may be written
* as "abdo" in the file, or as a number.
*
* - a-z are bits 1-26
* - A-Z are bits 27-52
* - !"#$%&'()*=, are bits 53-64
*
* @param char *flag The input string.
* @return bitvector_t The bitvector.
*/
bitvector_t asciiflag_conv_prool(char *flag) {
	bitvector_t flags = 0;
	bool is_number = TRUE;
	char *p;

	for (p = flag; *p; ++p) {
		// skip numbers
		if (isdigit(*p)) {
			continue;
		}
		
		is_number = FALSE;
		
		if (islower(*p)) {
			flags |= BIT(*p - 'a');
		}
		else if (isupper(*p)) {
			flags |= BIT(26 + (*p - 'A'));
		}
		else {
			flags |= BIT(52 + (*p - '!'));
		}
	}

	if (is_number) {
		flags = strtoull(flag, NULL, 10);
	}

	return (flags);
}


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string_prool(FILE * fl, char *error) {
	char buf[MAX_STRING_LENGTH], tmp[MAX_INPUT_LENGTH], *rslt;
	register char *point;
	int done = 0, length = 0, templength;

	*buf = '\0';

	do {
		if (!fgets(tmp, 512, fl)) {
			printf("ERROR: fread_string_prool: format error at or near %s\n", error);
			exit(1);
		}
				
		// upgrade to allow ~ when not at the end
		point = strchr(tmp, '\0');
		
		// backtrack past any \r\n or trailing space or tab
		if (point > tmp) {
			for (--point; point > tmp && (*point == '\r' || *point == '\n' || *point == ' ' || *point == '\t'); --point);
		}
		
		// look for a trailing ~
		if (*point == '~') {
			*point = '\0';
			done = 1;
		}
		else {
			strcpy(point+1, "\r\n");
		}

		templength = strlen(tmp);

		if (length + templength >= MAX_STRING_LENGTH) {
			printf("ERROR: fread_string_prool: string too large (db.c)\n");
			printf("%s\n", error);
			exit(1);
		}
		else {
			strcat(buf + length, tmp);
			length += templength;
		}
	} while (!done);

	/* allocate space for the new string and copy it */
	if (strlen(buf) > 0) {
		CREATE(rslt, char, length + 1);
		strcpy(rslt, buf);
	}
	else {
		rslt = NULL;
	}

	return (rslt);
}


/*
 * get_line_prool reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line_prool(FILE *fl, char *buf) {
	char temp[256];
	int lines = 0;

	do {
		fgets(temp, 256, fl);
		if (feof(fl)) {
			return (0);
		}
		lines++;
	} while (*temp == '*' || *temp == '\n');
	
	if (temp[strlen(temp) - 1] == '\n') {
		temp[strlen(temp) - 1] = '\0';
	}
	strcpy(buf, temp);
	return (lines);
}


 //////////////////////////////////////////////////////////////////////////////
//// CROP I/O ////////////////////////////////////////////////////////////////

/**
* Read one crop from file.
*
* WARNING: This is a near-copy of the version in db.lib.c and all changes to
* the crop format must be made in both places.
*
* @param FILE *fl The open .crop file
* @param crop_vnum vnum The crop vnum
*/
void parse_crop_prool(FILE *fl, crop_vnum vnum) {
	crop_data *crop, *find;
	int int_in[4];
	char line[256], str_in[256], str_in2[256], error[256];
	char *tmp;
	
	// create
	CREATE(crop, crop_data, 1);
	crop->vnum = vnum;

	if ((find = find_crop(vnum))) {
		log("WARNING: Duplicate crop vnum #%d", vnum);
		// but have to load it anyway to advance the file
	}
	else {
		HASH_ADD_INT(crop_table_prool, vnum, crop);
	}
		
	// for error messages
	sprintf(error, "crop vnum %d", vnum);
	
	// lines 1-2
	GET_CROP_NAME(crop) = fread_string_prool(fl, error);
	GET_CROP_TITLE(crop) = fread_string_prool(fl, error);
	
	// line 3: mapout, climate, flags
	if (!get_line_prool(fl, line) || sscanf(line, "%d %s %s", &int_in[0], str_in, str_in2) != 3) {
		log("SYSERR: Format error in line 3 of %s", error);
		exit(1);
	}
	
	GET_CROP_MAPOUT(crop) = int_in[0];
	GET_CROP_CLIMATE(crop) = asciiflag_conv_prool(str_in);
	GET_CROP_FLAGS(crop) = asciiflag_conv_prool(str_in2);
			
	// line 4: x_min, x_max, y_min, y_max
	if (!get_line_prool(fl, line) || sscanf(line, "%d %d %d %d", &int_in[0], &int_in[1], &int_in[2], &int_in[3]) != 4) {
		log("SYSERR: Format error in line 4 of %s", error);
		exit(1);
	}
	
	GET_CROP_X_MIN(crop) = int_in[0];
	GET_CROP_X_MAX(crop) = int_in[1];
	GET_CROP_Y_MIN(crop) = int_in[2];
	GET_CROP_Y_MAX(crop) = int_in[3];
		
	// optionals
	for (;;) {
		if (!get_line_prool(fl, line)) {
			log("SYSERR: Format error in %s, expecting alphabetic flags", error);
			exit(1);
		}
		switch (*line) {
			case 'D': {	// tile sets -- unneeded
				tmp = fread_string_prool(fl, error);
				free(tmp);
				tmp = fread_string_prool(fl, error);
				free(tmp);
				break;
			}
			case 'I': {	// interaction item -- unneeded
				// nothing to do -- these are 1-line types
				break;
			}
			case 'M': {	// mob spawn -- unneeded
				get_line_prool(fl, line);	// 1 extra line
				break;
			}
			case 'U': {	// custom messages -- unneeded
				get_line_prool(fl, line);	// message number line
				tmp = fread_string_prool(fl, error);	// message itself
				free(tmp);
				break;
			}
			case 'X': {	// extra desc -- unneeded
				tmp = fread_string_prool(fl, error);
				free(tmp);
				tmp = fread_string_prool(fl, error);
				free(tmp);
				break;
			}

			// end
			case 'S': {
				return;
			}
			
			default: {
				log("SYSERR: Format error in %s, expecting alphabetic flags", error);
				exit(1);
			}
		}
	}
}


/**
* Discrete load processes a data file, finds #VNUM records in it, and then
* sends them to the parser.
*
* @param FILE *fl The file to read.
* @param char *filename The name of the file, for error reporting.
*/
void discrete_load_crop(FILE *fl, char *filename) {
	any_vnum nr = -1, last;
	char line[256];
	
	for (;;) {
		if (!get_line_prool(fl, line)) {
			if (nr == -1) {
				printf("ERROR: crop file %s is empty!\n", filename);
			}
			else {
				printf("ERROR: Format error in %s after crop #%d\n...expecting a new crop, but file ended!\n(maybe the file is not terminated with '$'?)\n", filename, nr);
			}
			exit(1);
		}
		
		if (*line == '$') {
			return;	// done
		}

		if (*line == '#') {	// new entry
			last = nr;
			if (sscanf(line, "#%d", &nr) != 1) {
				printf("ERROR: Format error after crop #%d\n", last);
				exit(1);
			}

			parse_crop_prool(fl, nr);
		}
		else {
			printf("ERROR: Format error in crop file %s near crop #%d\n", filename, nr);
			printf("ERROR: ... offending line: '%s'\n", line);
			exit(1);
		}
	}
}


/**
* Loads all the crops into memory from the index file.
*/
void index_boot_crops(void) {
	char buf[512], filename[1024];
	FILE *index, *db_file;
	
	sprintf(filename, "%s%s", CROP_PREFIX, INDEX_FILE);
	
	if (!(index = fopen(filename, "r"))) {
		printf("ERROR: opening index file '%s': %s\n", filename, strerror(errno));
		exit(1);
	}
	
	fscanf(index, "%s\n", buf);
	while (*buf != '$') {
		safe_snprintf(filename, sizeof(filename), "%s%s", CROP_PREFIX, buf);
		if (!(db_file = fopen(filename, "r"))) {
			printf("ERROR: %s: %s\n", filename, strerror(errno));
			exit(1);
		}
		
		discrete_load_crop(db_file, buf);
		
		fclose(db_file);
		fscanf(index, "%s\n", buf);
	}
	fclose(index);
}


 //////////////////////////////////////////////////////////////////////////////
//// SECTOR I/O //////////////////////////////////////////////////////////////

/**
* Read one sector from file.
*
* WARNING: This is a near-copy of the version in db.lib.c and all changes to
* the sector format must be made in both places.
*
* @param FILE *fl The open sector file
* @param sector_vnum vnum The sector vnum
*/
void parse_sector_prool(FILE *fl, sector_vnum vnum) {
	char line[256], str_in[256], str_in2[256], str_in3[256], char_in[2], error[256], *tmp;
	struct evolution_data *evo, *last_evo = NULL;
	sector_data *sect, *find;
	double dbl_in;
	int int_in[4];
	sbitvector_t bit_in;
		
	// for error messages
	sprintf(error, "sector vnum %d", vnum);
	
	// create
	CREATE(sect, sector_data, 1);
	sect->vnum = vnum;
	
	if ((find = find_sect(vnum))) {
		printf("WARNING: Duplicate sector vnum #%d\n", vnum);
		// but have to load it anyway to advance the file
	}
	else {
		HASH_ADD_INT(sector_table_prool, vnum, sect);
	}
	
	// lines 1-2
	GET_SECT_NAME(sect) = fread_string_prool(fl, error);
	GET_SECT_TITLE(sect) = fread_string_prool(fl, error);
	
	// line 3: roadside, mapout, climate, movement, flags, build flags
	if (!get_line_prool(fl, line) || sscanf(line, "'%c' %d %s %d %s %s", &char_in[0], &int_in[0], str_in3, &int_in[2], str_in, str_in2) != 6) {
		printf("ERROR: Format error in line 3 of %s\n", error);
		exit(1);
	}
	
	GET_SECT_ROADSIDE_ICON(sect) = char_in[0];
	GET_SECT_MAPOUT(sect) = int_in[0];
	GET_SECT_CLIMATE(sect) = asciiflag_conv_prool(str_in3);
	GET_SECT_MOVE_LOSS(sect) = int_in[2];
	GET_SECT_FLAGS(sect) = asciiflag_conv_prool(str_in);
	GET_SECT_BUILD_FLAGS(sect) = asciiflag_conv_prool(str_in2);
		
	// optionals
	for (;;) {
		if (!get_line_prool(fl, line)) {
			printf("ERROR: Format error in %s, expecting alphabetic flags\n", error);
			exit(1);
		}
		switch (*line) {
			case 'E': {	// evolution
				if (!get_line_prool(fl, line) || sscanf(line, "%d %lld %lf %d", &int_in[0], &bit_in, &dbl_in, &int_in[2]) != 4) {
					printf("ERROR: Bad data in E line of %s\n", error);
					exit(1);
				}
				
				CREATE(evo, struct evolution_data, 1);
				evo->type = int_in[0];
				evo->value = bit_in;
				evo->percent = dbl_in;
				evo->becomes = int_in[2];
				evo->next = NULL;
				
				if (last_evo) {
					last_evo->next = evo;
				}
				else {
					GET_SECT_EVOS(sect) = evo;
				}
				last_evo = evo;
				break;
			}
			
			case 'C': {	// commands -- unneeded
				tmp = fread_string_prool(fl, error);
				free(tmp);
				break;
			}
			case 'D': {	// tile sets -- unneeded
				tmp = fread_string_prool(fl, error);
				free(tmp);
				tmp = fread_string_prool(fl, error);
				free(tmp);
				break;
			}
			
			case 'I': {	// interaction item -- unneeded
				// nothing to do -- these are 1-line types
				break;
			}
			case 'M': {	// mob spawn -- unneeded
				get_line_prool(fl, line);	// 1 extra line
				break;
			}
			case 'U': {	// custom messages -- unneeded
				get_line_prool(fl, line);	// message number line
				tmp = fread_string_prool(fl, error);	// message itself
				free(tmp);
				break;
			}
			case 'X': {	// extra desc -- unneeded
				tmp = fread_string_prool(fl, error);
				free(tmp);
				tmp = fread_string_prool(fl, error);
				free(tmp);
				break;
			}
			case 'Z': {	// Z: misc data
				if (line[1] && isdigit(line[1])) {
					switch (atoi(line + 1)) {
						case 1: {	// Z1: temperature type
							if (sscanf(line, "Z1 %d", &int_in[0]) == 1) {
								GET_SECT_TEMPERATURE_TYPE(sect) = int_in[0];
							}
							else {
								log("SYSERR: Format error in Z1 section of %s: %s", error, line);
								exit(1);
							}
							break;
						}
						default: {
							log("SYSERR: Format error in Z section of %s, bad Z number %d", error, atoi(line+1));
							exit(1);
						}
					}
				}
				else {
					log("SYSERR: Format error in Z section of %s", error);
					exit(1);
				}
				break;
			}
			
			case '_': {	// notes -- unneeded
				tmp = fread_string_prool(fl, error);
				free(tmp);
				break;
			}

			// end
			case 'S': {
				return;
			}
			
			default: {
				printf("ERROR: Format error in %s, expecting alphabetic flags\n", error);
				exit(1);
			}
		}
	}
}


/**
* Discrete load processes a data file, finds #VNUM records in it, and then
* sends them to the parser.
*
* @param FILE *fl The file to read.
* @param char *filename The name of the file, for error reporting.
*/
void discrete_load_sector(FILE *fl, char *filename) {
	any_vnum nr = -1, last;
	char line[256];
	
	for (;;) {
		if (!get_line_prool(fl, line)) {
			if (nr == -1) {
				printf("ERROR: sector file %s is empty!\n", filename);
			}
			else {
				printf("ERROR: Format error in %s after sector #%d\n...expecting a new sector, but file ended!\n(maybe the file is not terminated with '$'?)\n", filename, nr);
			}
			exit(1);
		}
		
		if (*line == '$') {
			return;	// done
		}

		if (*line == '#') {	// new entry
			last = nr;
			if (sscanf(line, "#%d", &nr) != 1) {
				printf("ERROR: Format error after sector #%d\n", last);
				exit(1);
			}

			parse_sector_prool(fl, nr);
		}
		else {
			printf("ERROR: Format error in sector file %s near sector #%d\n", filename, nr);
			printf("ERROR: ... offending line: '%s'\n", line);
			exit(1);
		}
	}
}


/**
* Loads all the sectors into memory from the index file.
*/
void index_boot_sectors(void) {
	char buf[512], filename[1024];
	FILE *index, *db_file;
	
	sprintf(filename, "%s%s", SECTOR_PREFIX, INDEX_FILE);
	
	if (!(index = fopen(filename, "r"))) {
		printf("ERROR: opening index file '%s': %s\n", filename, strerror(errno));
		exit(1);
	}
	
	fscanf(index, "%s\n", buf);
	while (*buf != '$') {
		safe_snprintf(filename, sizeof(filename), "%s%s", SECTOR_PREFIX, buf);
		if (!(db_file = fopen(filename, "r"))) {
			printf("ERROR: %s: %s\n", filename, strerror(errno));
			exit(1);
		}
		
		discrete_load_sector(db_file, buf);
		
		fclose(db_file);
		fscanf(index, "%s\n", buf);
	}
	fclose(index);
}


 //////////////////////////////////////////////////////////////////////////////
//// MAP I/O /////////////////////////////////////////////////////////////////


/**
* This loads the world array from file. This is optional.
*/
void load_base_map(void) {
	char sys[256];
	struct map_t_prool *last_land = NULL;
	struct map_file_header header;
	map_file_data store;
	int x, y;
	FILE *fl;
	
	// init
	land_prool = NULL;
	for (x = 0; x < MAP_WIDTH; ++x) {
		for (y = 0; y < MAP_HEIGHT; ++y) {
			world_prool[x][y].vnum = (y * MAP_WIDTH) + x;
			world_prool[x][y].island_id = NO_ISLAND;
			world_prool[x][y].sector_type = BASIC_OCEAN;
			world_prool[x][y].base_sector = BASIC_OCEAN;
			world_prool[x][y].natural_sector = BASIC_OCEAN;
			world_prool[x][y].crop_type = NOTHING;
			world_prool[x][y].affects = NOBITS;
			world_prool[x][y].flags = NOBITS;
			world_prool[x][y].sector_time = 0;
			world_prool[x][y].next = NULL;
		}
	}
	
	// first create a duplicate...
	/*
	sprintf(sys, "cp %s %s.copy", BINARY_MAP_FILE, BINARY_MAP_FILE);
	system(sys);*/

	char proolfile2[512];
	snprintf(proolfile2, sizeof (proolfile2), "%s.copy", BINARY_MAP_FILE);
	prool_copy(BINARY_MAP_FILE, proolfile2);

	if (!(fl = fopen(BINARY_MAP_FILE ".copy", "r+b"))) {
		printf("ERROR: No binary map file '%s.copy' to evolve\n", BINARY_MAP_FILE);
		exit(0);
	}
	
	// read header
	fread(&header, sizeof(struct map_file_header), 1, fl);
	
	// read in
	for (y = 0; y < header.height && !feof(fl); ++y) {
		for (x = 0; x < header.width && !feof(fl); ++x) {
			// read no matter what
			fread(&store, sizeof(map_file_data), 1, fl);
			
			// only apply if it's within the map size
			if (y < MAP_HEIGHT && x < MAP_WIDTH) {
				world_prool[x][y].island_id = store.island_id;
				world_prool[x][y].sector_type = store.sector_type;
				world_prool[x][y].base_sector = store.base_sector;
				world_prool[x][y].natural_sector = store.natural_sector;
				world_prool[x][y].affects = store.affects;
				world_prool[x][y].flags = store.misc;
				world_prool[x][y].sector_time = store.sector_time;
				world_prool[x][y].crop_type = store.crop_type;
				
				if (world_prool[x][y].sector_type != BASIC_OCEAN) {
					if (last_land) {
						last_land->next = &world_prool[x][y];
					}
					else {
						land_prool = &world_prool[x][y];
					}
					last_land = &world_prool[x][y];
				}
			}
		}
	}
	
	fclose(fl);
	unlink(BINARY_MAP_FILE ".copy");
}


// writes a single tile to the open output file
void write_tile(struct map_t_prool *tile, sector_vnum old) {
	struct evo_import_data dat;
	
	if (!outfl_prool) {
		printf("Error: called write_tile() with out the output file open\n");
		exit(0);
	}
	
	dat.vnum = tile->vnum;
	dat.old_sect = old;
	dat.new_sect = tile->sector_type;
	
	fwrite(&dat, sizeof(struct evo_import_data), 1, outfl_prool);
}


 //////////////////////////////////////////////////////////////////////////////
//// HELPER FUNCTIONS ////////////////////////////////////////////////////////

/**
* Checks if a crop tile needs and has water, and returns FALSE if it needs to
* be removed.
*
* @param struct map_t_prool *tile The tile to check.
* @return bool TRUE if ok, FALSE if it's a crop tile that should be removed
*/
bool check_crop_water(struct map_t_prool *tile) {
	crop_data *crop;
	
	if (tile->crop_type == NOTHING) {
		return TRUE;	// ok: not a crop
	}
	else if (!(crop = find_crop(tile->crop_type))) {
		return TRUE;	// ok: no crop data
	}
	else if (!CROP_FLAGGED(crop, CROPF_REQUIRES_WATER)) {
		return TRUE;	// ok: does not require water
	}
	else if (sect_flagged_within_distance(tile, SECTF_FRESH_WATER, water_crop_distance_prool, TRUE)) {
		return TRUE;	// ok: requires water, found water
	}
	else {
		// FAIL: requires water, does not have water
		return FALSE;
	}
}


/**
* Ensures the tile is not near ANY of the NOT-NEAR-SECTOR or NOT-ADJACENT tiles
* for an evolution to trigger. This is only used if the sector doesn't have the
* SEPARATE-NOT-NEARS or SEPARATE-NOT-ADJACENTS flags.
*
* @param struct map_t_prool *tile The tile to check evolutions and neighbors for.
* @param int type The EVO_ type to use (EVO_NOT_NEAR_SECTOR, EVO_NOT_ADJACENT).
* @param struct evolution_data *for_evo Optional: If the flagged SEPARATE-NOT-*, it will only check this one evolution rule. May be NULL.
* @return bool TRUE if it has at least 1 matching sector near it, FALSE if it has none.
*/
bool check_near_evo_sector(struct map_t_prool *tile, int type, struct evolution_data *for_evo) {
	struct evolution_data *evo;
	sector_data *sect;
	bool found = FALSE;
	
	if (!(sect = find_sect(tile->sector_type))) {
		return FALSE;
	}
	
	// iterate over evolutions checking all of them
	LL_FOREACH(GET_SECT_EVOS(sect), evo) {
		if (for_evo && for_evo != evo && SECT_FLAGGED(sect, (type == EVO_NOT_NEAR_SECTOR ? SECTF_SEPARATE_NOT_NEARS : SECTF_SEPARATE_NOT_ADJACENTS))) {
			// skip all but for-evo if these are checked separately
			continue;
		}
		if (evo->type == type && sect_within_distance(tile, evo->value, evo->type == EVO_NOT_NEAR_SECTOR ? nearby_distance_prool : 1.5, TRUE)) {
			found = TRUE;
			break;
		}
	}
	
	return found;
}


/**
* Ensures the tile is not near ANY of the NOT-NEAR-SECTOR-FLAGGED or
* NOT-ADJACENT-SECTOR-FLAGGED tiles for an evolution to trigger. This is only
* used if the sector doesn't have the SEPARATE-NOT-NEARS or
* SEPARATE-NOT-ADJACENTS flags.
*
* @param struct map_t_prool *tile The tile to check evolutions and neighbors for.
* @param int type The EVO_ type to use (EVO_NOT_NEAR_SECTOR_FLAG, EVO_NOT_ADJACENT_SECTOR_FLAG).
* @param struct evolution_data *for_evo Optional: If the flagged SEPARATE-NOT-*, it will only check this one evolution rule. May be NULL.
* @return bool TRUE if it has at least 1 matching sector near it, FALSE if it has none.
*/
bool check_near_evo_sector_flagged(struct map_t_prool *tile, int type, struct evolution_data *for_evo) {
	struct evolution_data *evo;
	sector_data *sect;
	bool found = FALSE;
	
	if (!(sect = find_sect(tile->sector_type))) {
		return FALSE;
	}
	
	// iterate over evolutions checking all of them
	LL_FOREACH(GET_SECT_EVOS(sect), evo) {
		if (for_evo && for_evo != evo && SECT_FLAGGED(sect, (type == EVO_NOT_NEAR_SECTOR_FLAG ? SECTF_SEPARATE_NOT_NEARS : SECTF_SEPARATE_NOT_ADJACENTS))) {
			// skip all but for-evo if these are checked separately
			continue;
		}
		if (evo->type == type && sect_flagged_within_distance(tile, evo->value, evo->type == EVO_NOT_NEAR_SECTOR_FLAG ? nearby_distance_prool : 1.5, TRUE)) {
			found = TRUE;
			break;
		}
	}
	
	return found;
}


/**
* Counts how many adjacent tiles have the given sector type...
*
* @param struct map_t_prool *tile The location to check.
* @param sector_vnum The sector vnum to find.
* @param bool count_original_sect If TRUE, also checks BASE_SECT
* @return int The number of matching adjacent tiles.
*/
int count_adjacent(struct map_t_prool *tile, sector_vnum sect, bool count_original_sect) {
	int iter, count = 0;
	struct map_t_prool *to_room;
	
	for (iter = 0; iter < NUM_2D_DIRS; ++iter) {
		to_room = shift_tile(tile, shift_dir_prool[iter][0], shift_dir_prool[iter][1]);
		
		if (to_room && (to_room->sector_type == sect || (count_original_sect && to_room->base_sector == sect))) {
			++count;
		}
	}
	
	return count;
}


/**
* Counts how many adjacent tiles have the given sector flags...
*
* @param struct map_t_prool *tile The location to check.
* @param sbitvector_t with_flags The sector flag(s) to find (all must match).
* @param bool count_original_sect If TRUE, also checks BASE_SECT
* @return int The number of matching adjacent tiles.
*/
int count_adjacent_flagged(struct map_t_prool *tile, sbitvector_t with_flags, bool count_original_sect) {
	int iter, count = 0;
	struct map_t_prool *to_room;
	sector_data *st;
	
	for (iter = 0; iter < NUM_2D_DIRS; ++iter) {
		if (!(to_room = shift_tile(tile, shift_dir_prool[iter][0], shift_dir_prool[iter][1]))) {
			continue;	// no room
		}
		
		// test sector / base
		if ((st = find_sect(to_room->sector_type)) && (GET_SECT_FLAGS(st) & with_flags) == with_flags) {
			// main type
			++count;
		}
		else if (count_original_sect && to_room->base_sector != to_room->sector_type && (st = find_sect(to_room->base_sector)) && (GET_SECT_FLAGS(st) & with_flags) == with_flags) {
			// base sect
			++count;
		}
	}
	
	return count;
}


/**
* Gets a crop from the table, by vnum.
*
* @param any_vnum vnum The crop vnum.
* @return crop_data* The found crop, or NULL for none.
*/
crop_data *find_crop(any_vnum vnum) {
	crop_data *crop;
	HASH_FIND_INT(crop_table_prool, &vnum, crop);
	return crop;
}


/**
* Gets a sector from the table, by vnum.
*
* @param any_vnum vnum The sector vnum.
* @return sector_data* The found sector, or NULL for none.
*/
sector_data *find_sect(any_vnum vnum) {
	sector_data *sect;
	HASH_FIND_INT(sector_table_prool, &vnum, sect);
	return sect;
}


/**
* This function takes a set of coordinates and finds another location in
* relation to them. It checks the wrapping boundaries of the map in the
* process.
*
* @param int start_x The initial X coordinate.
* @param int start_y The initial Y coordinate.
* @param int x_shift How much to shift X by (+/-).
* @param int y_shift How much to shift Y by (+/-).
* @param int *new_x A variable to bind the new X coord to.
* @param int *new_y A variable to bind the new Y coord to.
* @return bool TRUE if a valid location was found; FALSE if it's off the map.
*/
bool get_coord_shift_prool(int start_x, int start_y, int x_shift, int y_shift, int *new_x, int *new_y) {
	// clear these
	*new_x = -1;
	*new_y = -1;
	
	if (start_x < 0 || start_x >= MAP_WIDTH || start_y < 0 || start_y >= MAP_HEIGHT) {
		// bad location
		return FALSE;
	}
	
	// process x
	start_x += x_shift;
	if (start_x < 0) {
		if (WRAP_X) {
			start_x += MAP_WIDTH;
		}
		else {
			return FALSE;	// off the map
		}
	}
	else if (start_x >= MAP_WIDTH) {
		if (WRAP_X) {
			start_x -= MAP_WIDTH;
		}
		else {
			return FALSE;	// off the map
		}
	}
	
	// process y
	start_y += y_shift;
	if (start_y < 0) {
		if (WRAP_Y) {
			start_y += MAP_HEIGHT;
		}
		else {
			return FALSE;	// off the map
		}
	}
	else if (start_y >= MAP_HEIGHT) {
		if (WRAP_Y) {
			start_y -= MAP_HEIGHT;
		}
		else {
			return FALSE;	// off the map
		}
	}
	
	// found a valid location
	*new_x = start_x;
	*new_y = start_y;
	return TRUE;
}


/**
* Gets an evolution of the given type, if its percentage passes. If more than
* one of a type exists, the first one that matches the percentage will be
* returned.
*
* @param sector_vnum sect The sector to check.
* @param int type The EVO_ type to get.
* @return struct evolution_data* The found evolution, or NULL.
*/
struct evolution_data *get_evo_by_type(sector_vnum sect, int type) {
	struct evolution_data *evo, *found = NULL;
	sector_data *st;
	
	if (!(st = find_sect(sect))) {
		return NULL;
	}
	
	// this iterates over matching types checks their percent chance until it finds one
	for (evo = GET_SECT_EVOS(st); evo && !found; evo = evo->next) {
		if (evo->type == type && (number_prool(1, 10000) <= ((int) 100 * evo->percent))) {
			found = evo;
		}
	}
	
	return found;
}


// quick distance computation
double map_distance(struct map_t_prool *start, struct map_t_prool *end) {
	double dist;
	double x1 = MAP_X_COORD(start->vnum), x2 = MAP_X_COORD(end->vnum);
	double y1 = MAP_Y_COORD(start->vnum), y2 = MAP_Y_COORD(end->vnum);
	
	dist = ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
	dist = (int) sqrt(dist);
	
	return dist;
}


/**
* Calculates which season it is at a given location.
*
* Note: This is copied from weather.c determine_seasons() (with only minor logical changes).
*
* @param struct map_t_prool *tile The tile to find a season for.
* @return int TILESET_ const for the chosen season.
*/
int season(struct map_t_prool *tile) {
	double a_slope, b_slope, y_arctic, y_tropics, half_y;
	int ycoord = MAP_Y_COORD(tile->vnum), y_max;
	double latitude = Y_TO_LATITUDE(ycoord);
	bool northern = (latitude >= 0);
	int month = day_of_year_prool / 30;
	
	// month 0 is january; year is 0-359 days
	
	// tropics? -- take half the tropic value, convert to percent, multiply by map height
	if (ABSOLUTE(latitude) < TROPIC_LATITUDE) {
		if (month < 2) {
			return TILESET_SPRING;
		}
		else if (month >= 10) {
			return TILESET_AUTUMN;
		}
		else {
			return TILESET_SUMMER;
		}
	}
	
	// arctic? -- take half the arctic value, convert to percent, check map edges
	if (ABSOLUTE(latitude) > ARCTIC_LATITUDE) {
		return TILESET_WINTER;
	}
	
	// all other regions: first split the map in half (we'll invert for the south)
	y_max = 90;	// based on 90 degrees latitude
	
	if (northern) {
		y_arctic = ARCTIC_LATITUDE;
		y_tropics = TROPIC_LATITUDE;
		a_slope = ((y_arctic - 1) - (y_tropics + 1)) / 120.0;	// basic slope of the seasonal gradient
		b_slope = ((y_arctic - 1) - (y_tropics + 1)) / 90.0;
		half_y = latitude - y_tropics; // simplify by moving the y axis to match the tropics line
	}
	else {
		y_arctic = y_max - ARCTIC_LATITUDE;
		y_tropics = y_max - TROPIC_LATITUDE;
		a_slope = ((y_tropics - 1) - (y_arctic + 1)) / 120.0;	// basic slope of the seasonal gradient
		b_slope = ((y_tropics - 1) - (y_arctic + 1)) / 90.0;
		half_y = y_max - ABSOLUTE(latitude) - y_arctic;	// adjust to remove arctic
	}
	
	if (day_of_year_prool < 6 * 30) {	// first half of year
		if (half_y >= (day_of_year_prool + 1) * a_slope) {	// first winter line
			return northern ? TILESET_WINTER : TILESET_SUMMER;
		}
		else if (half_y >= (day_of_year_prool - 89) * b_slope) {	// spring line
			return northern ? TILESET_SPRING : TILESET_AUTUMN;
		}
		else {
			return northern ? TILESET_SUMMER : TILESET_WINTER;
		}
	}
	else {	// 2nd half of year
		if (half_y >= (day_of_year_prool - 360) * -a_slope) {	// second winter line
			return northern ? TILESET_WINTER : TILESET_SUMMER;
		}
		else if (half_y >= (day_of_year_prool - 268) * -b_slope) {	// autumn line
			return northern ? TILESET_AUTUMN : TILESET_SPRING;
		}
		else {
			return northern ? TILESET_SUMMER : TILESET_WINTER;
		}
	}
	
	// fail? we should never reach this
	return TILESET_SUMMER;
}


/**
* This determines if tile is close enough to a given sect. Ignores the tile
* itself (for detecting the same sect nearby).
*
* @param struct map_t_prool *tile
* @param sector_vnum sect Sector vnum
* @param double distance how far away to check
* @param bool count_original_sect If TRUE, also checks BASE_SECT
* @return bool TRUE if the sect is found
*/
bool sect_within_distance(struct map_t_prool *tile, sector_vnum sect, double distance, bool count_original_sect) {
	bool found = FALSE;
	struct map_t_prool *shift;
	int x, y;
	
	for (x = -1 * distance; x <= distance && !found; ++x) {
		for (y = -1 * distance; y <= distance && !found; ++y) {
			shift = shift_tile(tile, x, y);
			if (shift && shift != tile && (shift->sector_type == sect || (count_original_sect && shift->base_sector == sect)) && map_distance(tile, shift) <= distance) {
				found = TRUE;
			}
		}
	}
	
	return found;
}


/**
* This determines if tile is close enough to a tile with given sector flags.
* Ignores the tile itself; only checks nearby ones.
*
* @param struct map_t_prool *tile Source tile.
* @param sbitvector_t with_flags The sector flag(s) to find (all must match)
* @param double distance how far away to check
* @param bool count_original_sect If TRUE, also checks BASE_SECT
* @return bool TRUE if the sect is found
*/
bool sect_flagged_within_distance(struct map_t_prool *tile, sbitvector_t with_flags, double distance, bool count_original_sect) {
	bool found = FALSE;
	struct map_t_prool *shift;
	int x, y;
	sector_data *st;
	
	for (x = -1 * distance; x <= distance && !found; ++x) {
		for (y = -1 * distance; y <= distance && !found; ++y) {
			if (!(shift = shift_tile(tile, x, y)) || shift == tile) {
				continue;	// invalid location
			}
			if (map_distance(tile, shift) > distance) {
				continue;	// too far (we're using circles)
			}
			
			// test sect / base
			if ((st = find_sect(shift->sector_type)) && (GET_SECT_FLAGS(st) & with_flags) == with_flags) {
				// current sector passed
				found = TRUE;
			}
			else if (count_original_sect && shift->base_sector != shift->sector_type && (st = find_sect(shift->base_sector)) && (GET_SECT_FLAGS(st) & with_flags) == with_flags) {
				found = TRUE;
			}
		}
	}
	
	return found;
}


/**
* Find something relative to another tile.
* This function returns NULL if there is no valid shift.
*
* @param struct map_t_prool *origin The start location
* @param int x_shift How far to move east/west
* @param int y_shift How far to move north/south
* @return struct map_t_prool* The new location on the map, or NULL if the location would be off the map
*/
struct map_t_prool *shift_tile(struct map_t_prool *origin, int x_shift, int y_shift) {
	int x_coord, y_coord;
	
	// sanity?
	if (!origin) {
		return NULL;
	}
	
	if (get_coord_shift_prool(MAP_X_COORD(origin->vnum), MAP_Y_COORD(origin->vnum), x_shift, y_shift, &x_coord, &y_coord)) {
		return &world_prool[x_coord][y_coord];
	}
	return NULL;
}


 //////////////////////////////////////////////////////////////////////////////
//// RANDOM GENERATOR ////////////////////////////////////////////////////////

// borrowed from the EmpireMUD/CircleMUD base,
/* This program is public domain and was written by William S. England (Oct 1988) */

#define	mM  (unsigned long)2147483647
#define	qQ  (unsigned long)127773
#define	aA (unsigned int)16807
#define	rR (unsigned int)2836
static unsigned long seed;

void empire_srandom_prool(unsigned long initial_seed) {
    seed = initial_seed;
}

unsigned long empire_random_prool(void) {
	register int lo, hi, test;

	hi = seed/qQ;
	lo = seed%qQ;

	test = aA*lo - rR*hi;

	if (test > 0)
		seed = test;
	else
		seed = test+ mM;

	return (seed);
}

// core number generator
int number_prool(int from, int to) {
	// shortcut -paul 12/9/2014
	if (from == to) {
		return from;
	}
	
	/* error checking in case people call number_prool() incorrectly */
	if (from > to) {
		int tmp = from;
		from = to;
		to = tmp;
	}

	return ((empire_random_prool() % (to - from + 1)) + from);
}
