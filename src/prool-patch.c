// prool patch begin //

		if (!strcmp(arg,"prool"))
			{
			msg_to_char(ch,"\b&BСлава Україні! Grüß Gott!&0\n");
			return;
			}
		else if (!strcmp(arg,"prooldebug"))
			{
			if (prool_flag) prool_flag=0;
			else prool_flag=1;
			if (prool_flag) msg_to_char(ch,"Prool Flag is ON\n");
			else msg_to_char(ch,"Prool Flag is OFF\n");
			}
		else if (!strcmp(arg,"duhmada"))
			{int number;
			obj_data *obj;

			msg_to_char(ch, "Duh give to You a goodies.\n");

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
