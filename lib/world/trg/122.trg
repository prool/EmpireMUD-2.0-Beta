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
#12210
Vermilion Serragon combat: Flaming Maw, Crushing Coils, Tail Pin, Rage Spiral~
0 k 100
~
if %self.cooldown(12202)% || %self.disabled%
  halt
end
set room %self.room%
set diff %self.var(difficulty,1)%
set ml %self.var(ml,1 2 3 4)%
set nl %self.var(nl,4)%
eval wh %%random.%nl%%%
set old %ml%
set ml
set mv 0
while %wh% > 0
  set mv %old.car%
  if %wh% != 1
    set ml %ml% %mv%
  end
  set old %old.cdr%
  eval wh %wh% - 1
done
set ml %ml% %old%
eval nl %nl% - 1
remote ml %self.id%
remote nl %self.id%
scfight lockout 12202 25 32
if %mv% == 1
  scfight clear dodge
  %echo% &&l~%self% inhales deeply, jaws glowing faintly with inner fire...&&0
  wait 3 s
  if %self.disabled% || %self.aff_flagged(BLIND)%
    halt
  end
  set targ %random.enemy%
  if !%targ%
    halt
  end
  set targ_id %targ.id%
  if %diff% == 1
    dg_affect #12203 %self% HARD-STUNNED on 20
  end
  %send% %targ% &&l**** Heat shimmers in the air around |%self% mouth as the glow brightens... ****&&0 (dodge)
  %echoaround% %targ% &&lHeat shimmers in the air around |%self% mouth as the glow brightens...&&0
  scfight setup dodge %targ%
  set cycle 0
  eval times %diff% * 2
  eval when 9 - %diff%
  set done 0
  while %cycle% < %times% && !%done%
    wait %when% s
    if %targ.id% != %targ_id%
      set done 1
    elseif %targ.var(did_scfdodge)%
      %send% %targ% &&lYou hurl yourself aside just as a torrent of fire erupts where you stood!&&0
      %echoaround% %targ% &&l~%targ% hurls *%targ%self aside just as a torrent of fire erupts where &%targ% stood!&&0
    else
      %echo% &&lFire engulfs ~%targ% in an instant -- the searing heat scorches flesh and armor alike!&&0
      %send% %targ% That really hurts!
      %damage% %targ% 100 fire
    end
    eval cycle %cycle% + 1
    if %cycle% < %times% && !%done%
      wait 1
      if %targ.id% == %targ_id%
        %send% %targ% &&l**** &&Z|%self% gaping maw blazes with fire -- it's about to strike again! ****&&0 (dodge)
        %echoaround% %targ% &&l|%self% gaping maw blazes with fire -- it's about to strike!&&0
        scfight clear dodge
        scfight setup dodge %targ%
      end
    elseif %done% && %targ.id% == %targ_id% && %diff% == 1
      dg_affect #12208 %targ% TO-HIT 25 20
    end
  done
  scfight clear dodge
  dg_affect #12203 %self% off
elseif %mv% == 2
  scfight clear struggle
  if %room.players_present% > 1
    %echo% &&l**** &&Z~%self% whips around you, its coils locking tight and pinning everyone's arms to their sides! ****&&0 (struggle)
  else
    %echo% &&l**** &&Z~%self% whips around you, its coils locking tight and pinning your arms to your sides! ****&&0 (struggle)
  end
  if %diff% == 1
    dg_affect #12203 %self% HARD-STUNNED on 20
  end
  scfight setup struggle all 20
  set ch %room.people%
  while %ch%
    if %ch.affect(9602)%
      set scf_strug_char You thrash and strain, but the coils only squeeze tighter...
      set scf_strug_room ~%%actor%% thrashes and strains in the coils, but the serragon only squeezes tighter.
      set scf_free_char You wrench an arm free and twist violently, slipping loose from the crushing coils!
      set scf_free_room ~%%actor%% bursts free of the serragon's coils, staggering from the crushing grip!
      remote scf_strug_char %ch.id%
      remote scf_strug_room %ch.id%
      remote scf_free_char %ch.id%
      remote scf_free_room %ch.id%
    end
    set ch %ch.next_in_room%
  done
  set cycle 0
  set ongoing 1
  while %cycle% < 5 && %ongoing%
    wait 4 s
    set ongoing 0
    set ch %room.people%
    while %ch%
      if %ch.affect(9602)%
        set ongoing 1
        if %diff% > 1
          %send% %ch% &&l**** The coils tighten mercilessly, bones creaking as the air is driven from your lungs! ****&&0 (struggle)
          %dot% #12205 %ch% 100 30 physical 5
        else
          %send% %ch% &&l**** The coils tighten mercilessly, bones creaking as the air is driven from your lungs! ****&&0 (struggle)
        end
      end
      set ch %ch.next_in_room%
    done
    eval cycle %cycle% + 1
  done
  scfight clear struggle
  dg_affect #12203 %self% off
elseif %mv% == 3
  scfight clear struggle
  set targ %random.enemy%
  if !%targ%
    halt
  end
  set targ_id %targ.id%
  %send% %targ% &&l**** &&Z~%self% slams its tail down across your legs, pinning you hard against the ground! ****&&0 (struggle)
  %echoaround% %targ% &&l~%self% slams its tail down across |%targ% legs, pinning *%targ% against the ground!&&0
  if %diff% == 1
    dg_affect #12203 %self% HARD-STUNNED on 20
  end
  scfight setup struggle %targ% 20
  if %targ.affect(9602)%
    set scf_strug_char You strain to push the tail away, but it presses down even harder!
    set scf_strug_room ~%%actor%% struggles under the serragon's tail, but it presses down even harder, crushing *%%actor%% into the ground!
    set scf_free_char You twist violently, slipping free just as the tail crashes down again!
    set scf_free_room ~%%actor%% twists violently, slipping free just as the serragon's tail crashes down again!
    remote scf_strug_char %targ.id%
    remote scf_strug_room %targ.id%
    remote scf_free_char %targ.id%
    remote scf_free_room %targ.id%
  end
  eval punish -1 * %diff%
  set cycle 0
  set ongoing 1
  while %cycle% < 5 && %ongoing%
    wait 4 s
    set ongoing 0
    if %targ.id% != %targ_id%
      set done 1
    else
      if %targ.affect(9602)%
        set ongoing 1
        if %diff% > 1
          %send% %targ% &&l**** The heavy tail grinds into your legs, crushing muscle and bone beneath its weight! ****&&0 (struggle)
          %echoaround% %targ% &&lThe serragon's massive tail grinds into |%targ% legs, holding *%targ% helpless!&&0
          dg_affect #12206 %targ% BONUS-PHYSICAL %punish% 20
          dg_affect #12206 %targ% BONUS-MAGICAL %punish% 20
        else
          %send% %targ% &&l**** You're still trapped under the tail! ****&&0 (struggle)
        end
      end
    end
    eval cycle %cycle% + 1
  done
  scfight clear struggle
  dg_affect #12203 %self% off
elseif %mv% == 4
  scfight clear dodge
  %echo% &&l~%self% coils into a tight spiral as it trembles with violent energy...&&0
  eval dodge %diff% * 40
  dg_affect #12207 %self% DODGE %dodge% 20
  if %diff% == 1
    nop %self.add_mob_flag(NO-ATTACK)%
  end
  scfight setup dodge all
  wait 5 s
  if %self.disabled%
    dg_affect #12207 %self% off
    nop %self.remove_mob_flag(NO-ATTACK)%
    halt
  end
  %echo% &&l**** &&Z~%self% blurs into a raging crimson spiral -- it's about to unleash a devastating strike! ****&&0 (dodge)
  set cycle 1
  set hit 0
  eval wait 9 - %diff%
  while %cycle% <= %diff%
    scfight setup dodge all
    wait %wait% s
    set ch %room.people%
    while %ch%
      set next_ch %ch.next_in_room%
      if %self.is_enemy(%ch%)%
        if !%ch.var(did_scfdodge)%
          set hit 1
          %echo% &&lThe spiraling coil slams into ~%ch% with crushing force -- scales rake deep as it hurls *%ch% across the ground!&&0
          %send% %ch% That really hurt!
          dg_affect #12209 %ch% IMMOBILIZED on 10
          %damage% %ch% 100 physical
        elseif %ch.is_pc%
          %send% %ch% &&lYou throw yourself aside as the spinning mass corkscrews past!&&0
          if %diff% == 1
            dg_affect #12208 %ch% TO-HIT 25 10
          end
        end
        if %cycle% < %diff%
          %send% %ch% &&l**** The blazing spiral whips past and wheels around -- it's coming back again! ****&&0 (dodge)
        end
      end
      set ch %next_ch%
    done
    scfight clear dodge
    eval cycle %cycle% + 1
  done
  dg_affect #12207 %self% off
  if !%hit% && %diff% == 1
    %echo% &&l~%self% trashes weakly as it stumbles out of its spin and struggles to regain control.&&0
    dg_affect #12203 %self% HARD-STUNNED on 10
  end
  wait 8 s
end
nop %self.remove_mob_flag(NO-ATTACK)%
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
