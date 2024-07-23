// prool patch begin //
		strcpy(proolbuf,argument);
		cc=strchr(proolbuf,' ');
		if (cc!=NULL) *cc=0;

		if (!strcmp(arg,"prool"))
			{
			msg_to_char(ch,"&Yprool commands:&0\nprool, duhmada, пруль\n\nTest of ukr letters: Їжак і ґедзь\nCompilation at: %s %s",__DATE__,__TIME__);
			return;
			}
		else if (!strcmp(proolbuf,"пруль")) // prool: для кириллических команд нужно сравнивать не с arg как например в команде prool, а с proolbuf.  Потому что для кириллицы имя команды в массиве arg не формируется правильно. а proolbuf делаю я сам.
			{
			msg_to_char(ch,"\b&YFirst cyrillic command&0\n");
			msg_to_char(ch,"\b&YПерша кирилична команда Пруля. Слава Україні! Grüß Gott!&0\n");
			return;
			}
		else if (!strcmp(arg,"proole"))
			{
			msg_to_char(ch, "Start Prool's internal evolve\n");

			nearby_distance_prool = config_get_int("nearby_sector_distance");
			day_of_year_prool = DAY_OF_YEAR(main_time_info);
			water_crop_distance_prool = config_get_int("water_crop_distance"); // may be 0 to ignore unwatered crops
			evolve_int();
			}
		else if (!strcmp(arg,"duhmada"))
			{int number;
			obj_data *obj;

			msg_to_char(ch, "Dukh give to You a goodies.\n");

			number=2112; // bottle of water
			obj = read_object(number, TRUE);
			if (CAN_WEAR(obj, ITEM_WEAR_TAKE))
			obj_to_char(obj, ch);
			else
			obj_to_room(obj, IN_ROOM(ch));
			act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
			act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
			act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);

			number=3313; // bread
			obj = read_object(number, TRUE);
			if (CAN_WEAR(obj, ITEM_WEAR_TAKE))
			obj_to_char(obj, ch);
			else
			obj_to_room(obj, IN_ROOM(ch));
			act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
			act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
			act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);

			return;
			}
		else

// prool patch end //
