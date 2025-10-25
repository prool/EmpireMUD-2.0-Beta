#9229
Feral Dog Pack~
0 n 100
~
if %self.leader%
  halt
end
set num %random.3%
while %num% > 0
  eval num %num% - 1
  %load% mob 9229 ally
  set mob %self.room.people%
  if %mob.vnum% == 9229 && %mob% != %self%
    nop %mob.add_mob_flag(SENTINEL)%
  end
done
~
#9232
Box Jellyfish Combat~
0 k 100
~
switch %random.3%
  case 1
    set part leg
  break
  case 2
    set part arm
  break
  case 3
    set part neck
  break
done
%send% %actor% |%self% tentacles wrap around your %part%... that stings!
%echoaround% %actor% |%self% tentacles wrap around |%actor% %part%!
%dot% #9232 %actor% 200 20 poison 100
~
#9236
Lion and friends combat script~
0 k 33
~
set max_summons 14
set use_cooldown 20
* check cooldown
if %self.cooldown(9236)%
  halt
end
* set cooldown on same-faction mobs and check count
set lion_assist_count 0
set ch %self.room.people%
while %ch%
  if %ch.is_npc% && %ch.allegiance% == %self.allegiance%
    nop %ch.add_cooldown(9236,%use_cooldown%)%
    if %ch.vnum% == %self.vnum% && %ch.var(lion_assist_count)% > %lion_assist_count%
      set lion_assist_count %ch.var(lion_assist_count)%
    end
  end
  set ch %ch.next_in_room%
done
* update count on all mobs with same vnum
eval lion_assist_count %lion_assist_count% + 1
set ch %self.room.people%
while %ch%
  if %ch.is_npc% && %ch.vnum% == %self.vnum%
    remote lion_assist_count %ch.id%
  end
  set ch %ch.next_in_room%
done
* cancel if over the summon cap
if %lion_assist_count% >= %max_summons%
  detach 9236 %self.id%
  halt
end
* summon otherwise
wait 1 s
%load% mob %self.vnum% ally %self.level%
set loaded %self.room.people%
if %loaded.vnum% == %self.vnum% && %loaded% != %self%
  * success
  %echo% Another lion leaps in from out of sight!
  remote lion_assist_count %loaded.id%
  nop %loaded.add_cooldown(9236,%use_cooldown%)%
  %force% %loaded% maggro
end
~
#9242
Load pack animals (vnum + 1)~
0 n 100
~
eval vnum %self.vnum% + 1
set num %random.2%
while %num% > 0
  eval num %num% - 1
  %load% mob %vnum% ally
done
~
#9249
Summon non-following copies on-load (1-2)~
0 n 100
~
* Loads 1-2 copies if I'm the only one of me here
* verify I'm alone
set ch %self.room.people%
while %ch%
  if %ch% != %self% && %ch.vnum% == %self.vnum%
    * two's a crowd
    halt
  end
  set ch %ch.next_in_room%
done
* I think I'm alone now
set num %random.2%
while %num% > 0
  %load% mob %self.vnum%
  eval num %num% - 1
done
* Note: Copies are free-range, not followers
~
#9250
Load Mate (vnum + 1)~
0 n 100
~
eval vnum %self.vnum% + 1
%load% mob %vnum% ally
~
#9252
Summon duplicate followers on load (1-2)~
0 n 100
~
Loads 1-2 copies of me as followers if alone
* verify I'm alone
set ch %self.room.people%
while %ch%
  if %ch% != %self% && %ch.vnum% == %self.vnum%
    * two's a crowd
    halt
  end
  set ch %ch.next_in_room%
done
* I think I'm alone now
set num %random.2%
while %num% > 0
  %load% mob %self.vnum% ally
  eval num %num% - 1
done
* Note: Copies are free-range, not followers
~
#9255
Snake: Deadly Venom~
0 k 100
~
* High-damage-over-time bite. Combine with a NO-ATTACK flag.
%echo% ~%self% lunges forward and bites ~%actor%!
if %actor.has_tech(!Poison)%
  halt
end
%dot% #9255 %actor% 200 30 poison 100
~
#9259
Summon duplicate followers on-load (3-4)~
0 n 100
~
* Loads 3-4 copies if I'm the only one of me here
* verify I'm alone
set ch %self.room.people%
while %ch%
  if %ch% != %self% && %ch.vnum% == %self.vnum%
    * two's a crowd
    halt
  end
  set ch %ch.next_in_room%
done
* I think I'm alone now
eval num %random.2% + 2
while %num% > 0
  %load% mob %self.vnum% ally
  eval num %num% - 1
done
* Note: Copies are free-range, not followers
~
#9274
Chimp troop~
0 n 100
~
set num %random.3%
while %num% > 0
  eval num %num% - 1
  %load% mob 9274 ally
done
~
#9275
Bonobo troop~
0 n 100
~
%load% mob 9276 ally
set num %random.2%
while %num% > 0
  eval num %num% - 1
  %load% mob 9277 ally
done
~
#9284
Elephant herd~
0 n 100
~
eval num 1 + %random.2%
while %num% > 0
  %load% mob 9285 ally
  eval num %num% - 1
done
if %random.2% == 2
  %load% mob 9286 ally
end
~
#9296
Dire-tusked Mammoth War Platform Leaves Mammoth on Death~
5 f 100
~
%load% mob 9296
set mob %self.room.people%
if %mob.vnum% == 9296
  %slay% %mob%
end
~
#9297
Dire-tusk mammoth to War Platform when barded~
0 n 100
~
wait 1
%load% veh 9297
set veh %self.room.vehicles%
if %veh.vnum% == 9297
  * setup
end
%purge% %self%
~
$
