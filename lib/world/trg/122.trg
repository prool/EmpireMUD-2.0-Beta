#12200
Ribbon Serragon: Load~
0 n 100
~
dg_affect #12200 %self% !ATTACK on -1
wait 1
set istart %instance.start%
set iloc %instance.location%
if !%istart%
  halt
end
* first ever load ?
if !%istart.var(loaded)%
  *
  * mark loaded
  set loaded 1
  remote loaded %istart.id%
  *
  * check location
  if %iloc% && %self.room.template% == 12200
    * delete exit portal
    set outport %self.room.contents(12201)%
    if %outport%
      %purge% %outport%
    end
    * add exit door
    %door% %self.room% up room %iloc.vnum%
    * move me out
    mgoto %iloc%
    %echo% ~%self% pokes up from the pit!
  end
end
~
#12201
Ribbon Serragon: Down to enter pit~
1 c 4
down~
%force% %actor% enter pit
~
#12202
Ribbon Serragon: Death trigger~
0 f 100
~
if %self.vnum% != 12200
  if %instance.mob(12200)%
    halt
  end
end
if %self.vnum% != 12201
  if %instance.mob(12201)%
    halt
  end
end
if %self.vnum% != 12202
  if %instance.mob(12202)%
    halt
  end
end
* load delated completer
set inside %instance.start%
if %inside%
  %at% %inside% %load% obj 12200
  * allow loot?
  if !%inside.var(loot)%
    set loot 1
    remote loot %inside.id%
    nop %self.remove_mob_flag(!LOOT)%
    if %self.vnum% != 12200
      * step difficulty back up (was one lower on the babies)
      switch %self.var(difficulty,1)%
        case 2
          nop %self.add_mob_flag(HARD)%
        break
        case 3
          nop %self.remove_mob_flag(HARD)%
          nop %self.add_mob_flag(GROUP)%
        break
        case 4
          nop %self.add_mob_flag(HARD)%
        break
      done
    end
  end
end
~
#12203
Ribbon Serragon: Delayed completion~
1 f 0
~
%adventurecomplete%
~
#12205
Rainbow Serragon: Wander~
0 b 8
~
if %self.fighting% || %self.disabled%
  halt
elseif %self.room.template% == 12200
  %echo% ~%self% slithers up out of the pit.
  mgoto %instance.location%
  %echo% ~%self% slithers up out of the pit.
  %mod% %self% longdesc A tremendous ribbon serragon glimmers as it coils through the trees!
else
  %echo% ~%self% slithers down into the pit.
  mgoto %instance.start%
  %echo% ~%self% slithers down into the pit.
  %mod% %self% longdesc A tremendous ribbon serragon glimmers all around you in the dim light!
end
~
#12206
Ribbon Serragon: Difficulty selector~
0 c 0
difficulty~
if !%arg%
  %send% %actor% You must specify a level of difficulty. (Normal, Hard, Group, or Boss)
  return 1
  halt
end
if %self.fighting%
  %send% %actor% You can't change |%self% difficulty while &%self% is in combat!
  return 1
  halt
end
if normal /= %arg%
  set difficulty 1
elseif hard /= %arg%
  set difficulty 2
elseif group /= %arg%
  set difficulty 3
elseif boss /= %arg%
  set difficulty 4
else
  %send% %actor% That is not a valid difficulty level for this adventure. (Normal, Hard, Group, or Boss)
  return 1
  halt
end
* messaging
%send% %actor% You set the difficulty...
%echoaround% %actor% ~%actor% sets the difficulty...
* Clear existing difficulty flags and set new ones.
nop %self.remove_mob_flag(HARD)%
nop %self.remove_mob_flag(GROUP)%
if %difficulty% == 1
  * Then we don't need to do anything
  %echo% ~%self% has been set to Normal.
elseif %difficulty% == 2
  %echo% ~%self% has been set to Hard.
  nop %self.add_mob_flag(HARD)%
elseif %difficulty% == 3
  %echo% ~%self% has been set to Group.
  nop %self.add_mob_flag(GROUP)%
elseif %difficulty% == 4
  %echo% ~%self% has been set to Boss.
  nop %self.add_mob_flag(HARD)%
  nop %self.add_mob_flag(GROUP)%
end
remote difficulty %self.id%
%restore% %self%
nop %self.unscale_and_reset%
wait 1
* remove no-attack
if %self.affect(12200)%
  dg_affect #12200 %self% off
end
* mark me as scaled
set scaled 1
remote scaled %self.id%
~
#12207
Ribbon Serragon: Instruction to diff-sel~
0 B 0
~
if %self.affect(12200)%
  %send% %actor% You need to choose a difficulty before you can attack ~%self%.
  %send% %actor% Usage: difficulty <normal \| hard \| group \| boss>
  %echoaround% %actor% ~%actor% considers attacking ~%self%.
  return 0
else
  detach 12207 %self.id%
  return 1
end
~
#12208
Ribbon Serragon: Phase change~
0 l 50
~
* loads and sets up mobs 12201, 12202
%echo% &&lThe ribbon serragon rises up and splits -- it's not one serragon, it's two!&&0
set vnum 12201
while %vnum% <= 12202
  %load% mob %vnum%
  set mob %self.room.people%
  if %mob.vnum% == %vnum%
    set difficulty %self.var(difficulty,2)%
    remote difficulty %mob.id%
    * step down 1 difficulty
    if %difficulty% == 2
      nop %mob.add_mob_flag(TANK)%
    elseif %difficulty% == 3
      nop %mob.add_mob_flag(HARD)%
    elseif %difficulty% == 4
      nop %mob.add_mob_flag(GROUP)%
    end
    %restore% %mob%
    nop %mob.unscale_and_reset%
    %scale% %mob% %self.level%
    if %self.fighting%
      %force% %mob% maggro %self.fighting%
    end
    if !%mob.fighting%
      %force% %mob% maggro
    end
  end
  eval vnum %vnum% + 1
done
%purge% %self%
~
#12212
Ribbon Serragon: Pickpocket rejection strings~
0 p 100
~
if %ability% != 142
  * not pickpocket
  return 1
  halt
else
  return 0
  * after messaging...
end
if !%self.aff_flagged(!ATTACK)%
  %send% %actor% You can't imagine which part of it might be the "pocket" but it doesn't matter... you've attracted too much attention!
  %aggro% %actor%
else
  %send% %actor% You can't imagine which part of it might be the "pocket".
end
~
#12213
Ribbon Serragon: Environmental echoes~
0 b 8
~
if %self.fighting%
  halt
end
if %self.room.template% == 12200
  * in the pit
  set above %instance.location%
  switch %random.7%
    case 1
      %echo% The pit quakes with an excruciating grating noise as the serragon grinds its twin ribbons together.
      if %above%
        %at% %above% %echo% An excruciating grating noise echoes up from the pit.
      end
    break
    case 2
      %echo% The ground around you quivers as the serragon coils around the edges and scrapes at the dirt and rock.
      if %above%
        %at% %above% echo The ground quivers as the serragon briefly emerges from the pit and showers you with dirt and rocks.
      end
    break
    case 3
      %echo% The serragon's jagged scapes rasp against the stone walls, sending a shiver through the pit.
      if %above%
        %at% %above% %echo% Jagged scales rasp against stone, sending a shiver through the forest floor.
      end
    break
    case 4
      if %above%
        %at% %above% %echo% A frilled head rises suddenly from the pit, jaws opening wide before sinking back down.
      end
    break
    case 5
      %echo% The ribbons of the serragon spark faintly as they scrape together.
    break
    case 6
      %echo% The serragon's hiss echoes like cloth tearing, sharp and sudden in the forest silence.
    break
    case 7
      %echo% A scale fragment crunches underfoot, sharp as broken glass.
    break
  done
else
  * on the surface
  set below %instance.start%
  switch %random.5%
    case 1
      %echo% Strips of bark and leaves rain down into the pit as the serragon twists through the trees.
      if %below%
        %at% %below% %echo% Strips of bark and leaves rain down into the pit as the creature twists through the trees above.
      end
    break
    case 2
      %echo% A flash of vermillion and veridian whips past, too fast to follow with your eyes.
      if %below%
        %at% %below% %echo% Dirt trickles down the gouged walls, as if something is stirring above.
      end
    break
    case 3
      %echo% The ground quivers as the the serragon slides into the pit and reemerges with a shower of dirt and rocks.
      if %below%
        %at% %below% %echo% The ground around you quivers as the serragon descends into the pit, scraping at the sides as it coils, then writhes back out.
      end
    break
    case 4
      %echo% The ribbons of the serragon spark faintly as they scrape together.
    break
    case 5
      %echo% The serragon's hiss echoes like cloth tearing, sharp and sudden in the forest silence.
    break
  done
end
~
#12214
Ribbon Serragons: Recombine if out of combat~
0 ab 33
~
wait 30 s
if %self.fighting% || %self.disabled%
  halt
end
if %self.vnum% == 12201
  set other %instance.mob(12202)%
else
  set other %instance.mob(12201)%
end
if !%other%
  detach 12214 %self.id%
  halt
end
if %other.fighting% || %other.disabled%
  halt
end
if %other.room% != %self.room%
  %at% %other.room% %echo% ~%other% slithers away.
  %teleport% %other% %self.room%
  %echo% The second serragon slithers in.
end
%echo% &&lThe two serragons nuzzle up against each other and begin to coil together again. You cover your ears as the grinding noise shreds through the air!&&0
%load% mob 12200 %self.level%
%purge% %other%
%purge% %self%
~
$
