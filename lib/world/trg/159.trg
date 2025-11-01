#15900
Shipwrecked Goblins: Shared load trigger~
0 nA 100
~
set important_vnums 15900 15901 15902 15903
set traps 3
wait 0
set loc %instance.location%
if %loc%
  set start %instance.start%
  if %start% && %important_vnums% ~= %self.vnum%
    eval goblins %start.var(goblins,0)% + 1
    remote goblins %start.id%
  end
else
  * not instanced?
  nop %self.add_mob_flag(SPAWNED)%
  nop %self.remove_mob_flag(IMPORTANT)%
end
* mob-based stuff
switch %self.vnum%
  case 15900
    * scrapboss
    remote traps %self.id%
    if %loc%
      mgoto %loc%
      * load ship
      %load% veh 15900
      set num %random.20%
      if %num% == 1
        set name Brine Basher
      elseif %num% == 2
        set name Splintered Glory
      elseif %num% == 3
        set name Soggy Goblin
      elseif %num% == 4
        set name Boomtide
      elseif %num% == 5
        set name Leakin' Goblin
      elseif %num% == 6
        set name Keelcrack
      elseif %num% == 7
        set name Tidewrecker
      elseif %num% == 8
        set name Splodefish
      elseif %num% == 9
        set name Brineclaw
      elseif %num% == 10
        set name Goblin O' War
      elseif %num% == 11
        set name Boomchop
      elseif %num% == 12
        set name Grand Scuttler
      elseif %num% == 13
        set name Hullsplitter
      elseif %num% == 14
        set name Krunchwave
      elseif %num% == 15
        set name Salty Goblin
      elseif %num% == 16
        set name Stormsnout
      elseif %num% == 17
        set name Gobbernaut
      elseif %num% == 18
        set name Golden Driftwood
      elseif %num% == 19
        set name Foam Reaver
      else
        set name Brine Reaper
      end
      * rename
      set veh %loc.vehicles%
      if %veh.vnum% == 15900
        %mod% %veh% keywords shipwreck wreck salvage %name%
        %mod% %veh% shortdesc the wreck of the %name%
        %mod% %veh% longdesc The wreck of the %name% rises from the shore, half salvaged.
      end
    end
  break
  case 15901
  case 15902
  case 15903
    if %loc%
      mgoto %loc%
      mmove
      mmove
      mmove
    end
  break
done
~
#15901
Shipwrecked Goblins: Shared death trigger~
0 f 100
~
* This only belongs on goblins listed as %important_vnums% in trig 15900
set start %instance.start%
if %start%
  eval goblins %start.var(goblins,1)% - 1
  remote goblins %start.id%
  if %goblins% <= 0
    %load% obj 15900 %instance.location%
  end
end
~
#15902
Shipwrecked Goblins: Leash script~
0 i 100
~
*
set max_dist 6
set avoid_sects 5 6 8 30 32 33 52 57 251 253 10190 10191 10192 12368 18450
*
set start %instance.location%
set room %self.room%
return 1
if %method% != move || !%start%
  halt
end
* distance check
if %room.distance(%start%)% >= %max_dist%
  return 0
  halt
end
* sector check
if %avoid_sects% ~= %room.sector_vnum%
  return 0
  halt
end
~
#15903
Shipwrecked Goblins: Scrapboss build and chop~
0 ab 10
~
* This script terraforms the area and builds defenses
set easy_forest 2 3 4 24 37 38 45 47 81 89 212 10563 10564 10565
set trap_sects 0 20 21 40 50 51 56 58 70 71 72 73 74 75 76 79 82 200 204 230 231 234 240 241 243 244 250 252 10301 10302 10303 10305 10306 10308 10311
set start %instance.location%
if !%start%
  halt
end
set room %self.room%
set dist %room.distance(%start%)%
if %room.empire% || %self.fighting% || %self.disabled%
  * skip
  halt
elseif %room.building_vnum% == 15901
  %restore% %room%
  %echo% ~%self% repairs the barricades.
elseif %room.sector_vnum% == 57
  %echo% ~%self% uses some of the wreckage to construct a barricade.
  %build% %room% 15901
elseif %room.sector_vnum% == 1
  %echo% ~%self% chops down the last tree.
  %terraform% %room% 0
elseif %easy_forest% ~= %room.sector_vnum%
  %echo% ~%self% chops down a tree.
  eval vnum %room.sector_vnum% - 1
  %terraform% %room% %vnum%
elseif %room.sector_vnum% == 9
  %echo% ~%self% pulls up the road.
  %terraform% %room% %room.base_sector_vnum%
elseif %room.sector_vnum% == 23
  %echo% ~%self% burns the stumps.
  %terraform% %room% 20
elseif %room.sector_vnum% == 26
  %echo% ~%self% chops down the last tree.
  %terraform% %room% 20
elseif %room.sector_vnum% == 36
  %echo% ~%self% burns the stumps.
  %terraform% %room% 0
elseif %room.sector_vnum% == 39
  %echo% ~%self% chops down the last tree.
  %terraform% %room% 0
elseif %room.sector_vnum% == 44
  %echo% ~%self% chops down the last tree.
  %terraform% %room% 40
elseif %room.sector_vnum% == 46
  %echo% ~%self% burns the stumps.
  %terraform% %room% 40
elseif %room.sector_vnum% == 54
  %echo% ~%self% chops down the last tree.
  %terraform% %room% 50
elseif %room.sector_vnum% == 59
  %echo% ~%self% burns the stumps.
  %terraform% %room% 50
elseif %room.sector_vnum% == 80
  %echo% ~%self% chops down the last tree.
  %terraform% %room% 21
elseif %room.sector_vnum% == 90
  %echo% ~%self% chops down a massive tree.
  %terraform% %room% 4
  %load% obj 137 %room%
elseif %room.sector_vnum% == 200
  %echo% ~%self% chops down a tree.
  %terraform% %room% 200
elseif %room.sector_vnum% == 211 || %room.sector_vnum% == 222
  %echo% ~%self% burns the stumps.
  %terraform% %room% 204
elseif %room.sector_vnum% == 220
  %echo% ~%self% chops down a tree.
  %terraform% %room% 221
elseif %room.sector_vnum% == 221 || %room.sector_vnum% == 223 || %room.sector_vnum% == 224
  %echo% ~%self% chops down a tree.
  %terraform% %room% 204
elseif %room.sector_vnum% == 232 || %room.sector_vnum% == 233
  %echo% ~%self% chops down a tree.
  %terraform% %room% 231
elseif %room.sector_vnum% == 10562
  %echo% ~%self% chops down the last tree.
  %terraform% %room% 0
elseif %room.sector_vnum% == 10566
  %echo% ~%self% burns the stumps.
  %terraform% %room% 0
elseif %trap_sects% ~= %room.sector_vnum% && %self.var(traps,0)% > 0 && %dist% >= 4
  %echo% ~%self% arranges some debris into piles.
  %build% %room% 15905
  eval traps %self.var(traps,1)% - 1
  remote traps %self.id%
else
  * look for something to fix
  set veh %room.vehicles%
  while %veh%
    if %veh.vnum% >= 15900 && %veh.vnum% <= 15910
      if %veh.health% < %veh.maxhealth%
        %echo% ~%self% repairs @%veh%.
        %restore% %veh%
      end
    end
    set veh %veh.next_in_room%
  done
  halt
end
~
#15904
Shipwrecked Goblins: Tent setup and leash~
0 i 100
~
* this builds tents for each mob in a specific range, mob vnums 15901 15902 15903
set tent_sects 0 7 9 12 13 14 20 21 50 51 53 56 58 70 71 73 74 77 78 83 84 91 200 202 203 204 230 231 236 237 238 239 240 241 243 244 246 247 248 249 250 601 602 603 604 606 607 608 611 612 613 614 615 616 617 618 621 622 623
set avoid_sects 5 6 8 30 32 33 52 251 253 10190 10191 10192 12368 18450
eval range %self.vnum% - 15901 + 2
return 1
*
set room %self.room%
set loc %instance.location%
if !%loc%
  return 0
  halt
end
set dist %room.distance(%loc%)%
if %avoid_sects% ~= %room.sector_vnum%
  return 0
  halt
elseif %dist% >= %range% && %dist% < %range% + 1
  if %tent_sects% ~= %room.sector_vnum%
    * tent here!
    nop %self.add_mob_flag(SENTINEL)%
    wait 2 s
    %echo% ~%self% sets up ^%self% tent.
    %load% veh %self.vnum%
    wait 1 s
    enter tent
    detach 15904 %self.id%
  end
elseif %dist% > %range%
  return 0
  halt
end
~
#15905
Shipwrecked Goblins: Random Namer~
0 nA 100
~
* Namelists: Both 20 names in length, update size if you change this
set male_names Zalkaba Greenchops Gobtholomew Gidd Gobstede Goblarossa Balben Rek Rekker Brokka Elvo Goona Seascar Stabman Shivnow Bitesal Fyunk Diyed Axer Bilj Rotbelleh Grimspit Bonegryn Bonepocket Charmspit Grubsnuff Murcant Glummire Pokeskull Bleedsout
set female_names Merkilda Nilbagga Thilxa Bongy Reeda Gobshih Killigobba Myain Pimba Crunchbones Rattlebones Steala Drooner Vence Bitta Bythe Onner Nelbog Meener Warze Rotlik Moldthumb Wormbitar Stitchgut Sewrot Eetchfang Peeleye Moonspork Blinkgone Hakbone
set size 30
*
* Exit early on resurrect
if %self.room.template% != 15900
  halt
end
*
* Pick name
set start %instance.start%
if %start%
  set found 0
  set used_names %start.var(used_names,0)%
  while !%found%
    eval pos %%random.%size%%%
    if !(%used_names% ~= %pos%)
      set found 1
      set used_names %used_names% %pos%
      remote used_names %start.id%
    end
  done
else
  * no start
  set found 1
  eval pos %%random.%size%%%
end
if %self.sex% == female
  set list %female_names%
else
  set list %male_names%
end
while %pos% > 0 && %found%
  set name %list.car%
  set list %list.cdr%
  eval pos %pos% - 1
done
if !%name%
  * failure to find a name means it's ok to use the one they came with
  halt
end
* mob type
switch %self.vnum%
  case 15900
    %mod% %self% keywords %name% goblin scrapboss boss
    %mod% %self% shortdesc Scrapboss %name%
    * The goblin scrapboss is dragging a bag of nails.
  break
  case 15901
    %mod% %self% keywords %name% goblin wavewhisperer whisperer wave-whisperer
    %mod% %self% shortdesc Wavewhisperer %name%
    %mod% %self% longdesc Wavewhisperer %name% stares serenely off into the distance.
  break
  case 15902
    %mod% %self% keywords %name% goblin crabherd crab-herd herd
    %mod% %self% shortdesc %name% the crabherd
    %mod% %self% longdesc %name% is looking for one of the crabs.
  break
  case 15903
    %mod% %self% keywords %name% goblin raidscribe raid-scribe scribe
    %mod% %self% shortdesc Raidscribe %name%
    %mod% %self% longdesc Raidscribe %name% is carving notches into a long pole.
  break
  case 15904
    %mod% %self% keywords %name% goblin ropesnarl rope-snarl snarl
    %mod% %self% shortdesc Ropesnarl %name%
    * A goblin ropesnarl is twirling a net.
  break
  case 15905
    %mod% %self% keywords %name% goblin bonecounter bone-counter counter
    %mod% %self% shortdesc Bonecounter %name%
    * A goblin bonecounter is eyeing you up and down.
  break
  case 15906
    %mod% %self% keywords %name% goblin shellpicker shell-picker picker
    %mod% %self% shortdesc Shellpicker %name%
    * A goblin shellpicker is gleaming in the light.
  break
  case 15907
    %mod% %self% keywords %name% goblin lootcaller loot-caller caller
    %mod% %self% shortdesc Lootcaller %name%
    * A goblin lootcaller is barking out orders.
  break
  case 15908
    %mod% %self% keywords %name% goblin banner-lugger bannerlugger lugger
    %mod% %self% shortdesc Banner-lugger %name%
    * A goblin banner-lugger is standing here.
  break
  case 15909
    %mod% %self% keywords %name% goblin flame-skulker flameskulker skulker
    %mod% %self% shortdesc Flame-skulker %name%
    * A goblin flame-skulker is lurking nearby.
  break
  case 15910
    %mod% %self% keywords %name% goblin tarflinger tar-flinger flinger
    %mod% %self% shortdesc Tarflinger %name%
    * A goblin tarflinger is holding a bucket.
  break
  case 15911
    %mod% %self% keywords %name% goblin deadwright dead-wright wright
    %mod% %self% shortdesc Deadwright %name%
    * A goblin deadwright seems to be chanting something.
  break
done
~
#15906
Shipwrecked Goblin: Another trap~
1 n 100
~
* Restores a trap to the scrapboss, if possible
set mob %instance.mob(15900)%
if %mob%
  eval traps %mob.var(traps,0)% + 1
  remote traps %mob.id%
end
%purge% %self%
~
#15907
Shipwrecked Goblins: Trap go boom~
2 g 100
~
if %actor.is_npc%
  halt
elseif %actor.aff_flagged(FLYING)%
  halt
end
wait 0
%send% %actor% You trip over some of the debris... there's a strange clicking noise.
%echoaround% %actor% ~%actor% trips over some debris... there's a strange clicking noise.
wait 1 s
%load% obj 15902
set trap %room.contents%
if %trap.vnum% == 15902
  %scale% %trap% %actor.level%
end
* and reset
set start %instance.nearest_rmt(15900)%
if %room.distance(%start%)% < 10
  %load% obj 15901 %start%
end
~
#15908
Goblin Shipwreck: Trap damage~
1 n 100
~
wait 0
%echo% &&r**** The debris piles EXPLODE! ****&&0
%aoe% 500 physical
%build% %self.room% demolish
%purge% %self%
~
#15911
Shipwrecked Goblins: Deadwright resurrection~
0 b 10
~
* find and resurrect a goblin
if %self.disabled%
  halt
end
set obj %self.room.contents%
while %obj%
  if %obj.type% == CORPSE && %obj.val0% >= 15904 && %obj.val0% <= 15911
    %load% mob %obj.val0%
    set mob %self.room.people%
    if %mob.vnum% == %obj.val0%
nop %mob.add_mob_flag(!LOOT)%
      %echo% ~%self% waves ^%self% hands over a corpse... There's a purple glow and ~%mob% is resurrected!
      %mod% %mob% append-lookdesc There's a dull purple glow in %mob.hisher% eyes and a bit of drool on the chin.
      %purge% %obj%
      halt
    end
  end
  set obj %obj.next_in_list%
done
~
$
