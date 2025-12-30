#12800
Celestial Forge: Donate to open portal~
0 c 0 4
L c 12800
L c 12806
L j 12810
L w 5100
donate~
set room %self.room%
set which 0
set dest 0
* validate argument
if !%arg%
  %send% %actor% Donate to which celestial forge? (iron, ...)
elseif iron forge /= %arg% || lodestone forge /= %arg%
  set which 12800
  set dest 12810
  set curr 5100
  set str an iron shard
else
  %send% %actor% Unknown celestial forge.
end
* did we find one?
if !%which% || !%dest%
  halt
end
* validate target
if %room.contents(%which%)%
  %send% %actor% There is already a portal to that celestial forge here.
  halt
elseif %actor.currency(%curr%)% < 1
  eval curname %%currency.%curr%(1)%%
  %send% %actor% You don't even have %curname.ana% %curname% to donate!
  halt
end
set toroom %instance.nearest_rmt(%dest%)%
if !%toroom%
  %send% %actor% The forge is not accepting donations right now. Try again soon.
  halt
end
* create portal-in
%load% obj %which% %room%
set inport %room.contents%
if %inport.vnum% != %which%
  %send% %actor% Something went wrong.
  %log% syslog script Trig 12800 failed to open portal.
  halt
end
* charge
nop %actor.give_currency(%curr%, -1)%
* update portal-in
nop %inport.val0(%toroom.vnum%)%
%send% %actor% You donate %str% to the forge and @%inport% appears!
%echoaround% %actor% ~%actor% donates %str% to the forge and @%inport% appears!
* portal back
%load% obj 12806 %toroom%
set outport %toroom.contents%
if %outport% && %outport.vnum% == 12806
  nop %outport.val0(%room.vnum%)%
  set emp %room.empire_name%
  if %emp%
    %mod% %outport% shortdesc the portal back to %emp%
    %mod% %outport% longdesc The portal back to %emp% is open here.
    %mod% %outport% keywords portal back %emp%
  end
  %at% %toroom% %echo% A portal whirls open!
end
~
#12801
Celestial Forge: Request exit~
2 c 0 5
L c 9680
L c 12800
L c 12806
L e 5195
L j 12810
return~
if %actor.is_npc%
  * possibly immortal trying to return
  return 0
  halt
end
* try adventure summon
if %actor.adventure_summoned_from%
  nop %actor.end_adventure_summon%
  halt
end
* try portal
set cf_return %actor.var(cf_return)%
if %cf_return%
  makeuid toroom room %cf_return%
  if %toroom% && %toroom.building_vnum% == 5195 && %actor.canuseroom_guest(%toroom%)%
    * valid location! check existing portals
    set obj %room.contents%
    while %obj%
      if %obj.type% == PORTAL && %obj.val0% == %toroom.vnum%
        * oops
        %send% %actor% There's already a portal open for you!
        halt
      end
      set obj %obj.next_content%
    done
    * if we got here, make a portal
    switch %room.template%
      case 12810
        set in_vnum 12800
      break
      default
        set in_vnum 0
      break
    done
    * success?
    if %in_vnum%
      * portal out
      %load% obj 12806
      set outport %room.contents%
      if %outport.vnum% == 12806
        nop %outport.val0(%toroom.vnum%)%
        set emp %toroom.empire_name%
        if %emp%
          %mod% %outport% shortdesc the portal back to %emp%
          %mod% %outport% longdesc The portal back to %emp% is open here.
          %mod% %outport% keywords portal back %emp%
          %echo% A portal back to %emp% whirls open!
        else
          %echo% A portal whirls open!
        end
      end
      * portal back here
      %load% obj %in_vnum% %toroom%
      set inport %toroom.contents%
      if %inport.vnum% == %in_vnum%
        nop %inport.val0(%room.vnum%)%
        %at% %toroom% %echo% A portal whirls open!
      end
      halt
    end
  end
end
* if we got this far, explore other options
set tele -1
if %actor.home%
  set tele %actor.home%
else
  set tele %startloc%
end
if %tele% != -1
  %send% %actor% You're whisked away in a flash of starlight!
  %echoaround% %actor% ~%actor% is whisked away in a flash of starlight!
  %teleport% %actor% %tele%
  %echoaround% %actor% ~%actor% arrives in a flash!
  %load% obj 9680 %actor% inv
  * friends
  set ch %room.people%
  while %ch%
    set next_ch %ch.next_in_room%
    if %ch.is_npc% && %ch.leader% == %actor%
      %at% %room% %echo% ~%ch% is whisked away, too!
      %teleport% %ch% %tele%
      %echo% ~%ch% arrives in a flash!
    end
    set ch %next_ch%
  done
else
  %send% %actor% You can't seem to find a way to return. Surely this is a bug.
end
~
#12802
Celestial Forge: Detect player entry, Grant abilities~
2 g 100 6
L e 5195
L i 12800
L j 12810
L j 12815
L o 12810
L q 6
~
if %actor.is_npc%
  halt
end
* Ensure ability first, if over 75 Trade
if %actor.skill(6)% >= 76
  if %room.template% >= 12810 && %room.template% <= 12815
    if !%actor.has_bonus_ability(12810)%
      nop %actor.add_bonus_ability(12810)%
    end
  end
end
* Track origin
if !%was_in%
  halt
end
if %was_in.template% >= 12800 && %was_in.template% <= 12999
  * inside same adventure
  halt
end
if %was_in.building_vnum% == 5195
  * Celestial Forge
  set cf_return %was_in.vnum%
  remote cf_return %actor.id%
else
  * no return location available
  rdelete cf_return %actor.id%
end
~
#12803
Celestial Forge: Time and Weather commands~
2 c 0 1
L j 12810
time weather~
if %cmd.mudcommand% == time
  if %room.template% == 12810
    %send% %actor% It looks like nighttime through the hole at the top of the tunnel.
  else
    %send% %actor% The beautiful night sky overhead tells you it's nighttime.
  end
  return 1
elseif %cmd.mudcommand% == weather
  if %room.template% == 12810
    %send% %actor% It's hard to tell the weather from in here.
  else
    %send% %actor% The night sky is cloudless and vast.
  end
  return 1
else
  * unknown command somehow?
  return 0
end
~
#12805
Celestial Forge: Immortal controller~
1 c 2 1
L j 12810
cforge~
if !%actor.is_immortal%
  %send% %actor% You lack the power to use this.
  halt
end
set mode %arg.car%
set arg2 %arg.cdr%
if goto /= %mode%
  * target handling
  if iron /= %arg2%
    set to_room %instance.nearest_rmt(12810)%
  else
    set to_room %instance.nearest_rmt(%arg2%)%
  end
  *
  if !%to_room%
    %send% %actor% Invalid target Celestial Forge room.
  elseif %to_room.template% < 12800 || %to_room.template% > 12999
    %send% %actor% You can only use this to goto a Celestial Forge room.
  else
    %echoaround% %actor% ~%actor% vanishes!
    %teleport% %actor% %to_room%
    %echoaround% %actor% ~%actor% appears in a flash!
    %force% %actor% look
  end
else
  %send% %actor% &&0Usage: cforge goto iron
  %send% %actor% &&0       cforge goto <template vnum>
end
~
#12806
Celestial Forge: Require permission to enter portal~
1 c 4 0
enter~
return 0
if %actor.obj_target(%arg.argument1%)% != %self% || %self.val0% <= 0
  halt
end
* find target
makeuid toroom room %self.val0%
if !%toroom%
  halt
end
* validate permission
if !%actor.canuseroom_guest(%toroom%)%
  %send% %actor% You don't have permission to enter that portal.
  return 1
  halt
end
* otherwise they're good to go!
~
#12807
Celestial Forge: One-time greet using script1 or script2~
0 gh 100 0
~
* Uses mob custom script1 for a once-ever greeting per character and script2
*   for players who have been to the instance before. Each script1/2 line
*   sent every %line_gap% (7 sec) until it runs out of strings. The mob will
*   be SENTINEL and SILENT during this period.
* usage: .custom add <script1 | script2> <command> <string>
* valid commands: say, emote, do (execute command), echo (script), skip, attackable
* also: vforce <mob vnum in room> <command>
* also: set line_gap <time> sec
* also: mod <field> <value> -- runs %mod% %self%
* NOTE: waits for %line_gap% (7 sec) after all commands EXCEPT do/vforce/set/attackable/mod
set line_gap 7 sec
* begin
if %actor.is_npc%
  halt
end
set room %self.room%
* let everyone arrive
wait 0
if %self.fighting% || %self.disabled%
  halt
end
* check for someone who needs the greeting
set any_first 0
set any_repeat 0
set ch %room.people%
while %ch%
  if %ch.is_pc%
    * check first visit
    set 12800_visits %ch.var(12800_visits)%
    if !(%12800_visits% ~= %self.vnum%)
      set any_first 1
      set 12800_visits %12800_visits% %self.vnum%
      remote 12800_visits %ch.id%
    end
    * check repeat visit
    set varname greet_%ch.id%
    if !%self.varexists(%varname%)%
      set any_repeat 1
      set %varname% 1
      remote %varname% %self.id%
    end
  end
  set ch %ch.next_in_room%
done
* did anyone need the intro
if %any_first%
  set stype script1
elseif %any_repeat%
  set stype script2
else
  halt
end
* greeting detected: prepare (storing as variables prevents reboot issues)
if !%self.mob_flagged(SENTINEL)%
  set no_sentinel 1
  remote no_sentinel %self.id%
  nop %self.add_mob_flag(SENTINEL)%
end
if !%self.mob_flagged(SILENT)%
  set no_silent 1
  remote no_silent %self.id%
  nop %self.add_mob_flag(SILENT)%
end
* Show the scripted text
* tell story
set pos 0
set msg %self.custom(%stype%,%pos%)%
while !%msg.empty%
  * check early end
  if %self.disabled% || %self.fighting%
    halt
  end
  * next message
  set mode %msg.car%
  set msg %msg.cdr%
  if %mode% == say
    say %msg%
    set waits 1
  elseif %mode% == do
    %msg.process%
    set waits 0
  elseif %mode% == echo
    %echo% %msg.process%
    set waits 1
  elseif %mode% == vforce
    set vnum %msg.car%
    set msg %msg.cdr%
    set targ %self.room.people(%vnum%)%
    if %targ%
      %force% %targ% %msg.process%
    end
    set waits 0
  elseif %mode% == emote
    emote %msg%
    set waits 1
  elseif %mode% == set
    set subtype %msg.car%
    set msg %msg.cdr%
    if %subtype% == line_gap
      set line_gap %msg%
    else
      %echo% ~%self%: Invalid set type '%subtype%' in storytime script.
    end
    set waits 0
  elseif %mode% == skip
    * nothing this round
    set waits 1
  elseif %mode% == attackable
    if %self.aff_flagged(!ATTACK)%
      dg_affect %self% !ATTACK off
    end
    set waits 0
  elseif %mode% == mod
    %mod% %self% %msg.process%
    set waits 0
  else
    %echo% %self.name%: Invalid script message type '%mode%'.
  end
  * fetch next message and check wait
  eval pos %pos% + 1
  set msg %self.custom(%stype%,%pos%)%
  if %waits% && %msg%
    wait %line_gap%
  end
done
* Done: mark as greeted for anybody now present (but NOT first-greet)
set ch %room.people%
while %ch%
  if %ch.is_pc%
    set varname greet_%ch.id%
    set %varname% 1
    remote %varname% %self.id%
  end
  set ch %ch.next_in_room%
done
* Done: cancel sentinel/silent
if %self.varexists(no_sentinel)%
  nop %self.remove_mob_flag(SENTINEL)%
end
if %self.varexists(no_silent)%
  nop %self.remove_mob_flag(SILENT)%
end
~
#12810
Celestial Forge: Mine attempt~
2 c 0 0
mine~
%send% %actor% There doesn't seem to be anything left here to mine.
~
#12811
Celestial Forge: Unique item exclusion~
1 j 0 5
L c 12810
L c 12814
L c 12818
L c 12822
L c 12826
~
set ring_list 12810 12814 12818 12822 12826
set ring_pos rfinger lfinger
set pos_list
*
if %ring_list% ~= %self.vnum%
  set pos_list %ring_pos%
  set vnum_list %ring_list%
end
*
if %pos_list% && %vnum_list%
  while %pos_list%
    set pos %pos_list.car%
    set pos_list %pos_list.cdr%
    set obj %actor.eq(%pos%)%
    if %obj%
      if %vnum_list% ~= %obj.vnum%
        * oops
        %send% %actor% You can't equip @%self% while wearing @%obj%.
        return 0
        halt
      end
    end
  done
end
* ok
return 1
~
#12832
Lodestone Firefly: Northward pull~
0 bt 8 0
~
set message %random.3%
set ch %self.room.people%
while %ch%
  if %ch.is_pc% && %ch.can_see(%self%)%
    switch %message%
      case 1
        %send% %ch% ~%self% tries to draw you to the %ch.dir(north)%.
      break
      case 2
        %send% %ch% ~%self% darts toward the %ch.dir(north)%.
      break
      case 3
        %send% %ch% ~%self% seems drawn to the %ch.dir(north)%.
      break
    done
  end
  set ch %ch.next_in_room%
done
~
#12834
Celestial Forge: Buy shard companion~
1 n 100 7
L b 12834
L b 12844
L w 5100
L w 5101
L w 5102
L w 5103
L w 5104
~
* TODO fix cost
set cost 111
*
* list in order from highest to lowest, count=max
set comp_list 12844 12834
set comp_count 2
*
* init
set tier 1
set error 0
set has_tier 0
set has_vnum 0
*
* Determine player stats
set actor %self.carried_by%
if !%actor%
  %purge% %self%
  halt
end
*
* Determine my stats
switch %self.vnum%
  case 12834
    set tier 1
    set new_vnum 12834
    set upgrade tank
  break
  case 12835
    set tier 1
    set new_vnum 12834
    set upgrade dps
  break
  case 12836
    set tier 1
    set new_vnum 12834
    set upgrade caster
  break
  default
    %send% %actor% This companion is not yet implemented.
    set error 1
  break
done
*
* Determine existing
while %comp_list% && !%has_tier%
  set vnum %comp_list.car%
  set comp_list %comp_list.cdr%
  if %actor.has_companion(%vnum%)%
    set has_tier %comp_count%
    set has_vnum %vnum%
    if !%actor.companion% || %actor.companion.vnum% != %vnum%
      %mod% %actor% companion %vnum%
    end
  end
  eval comp_count %comp_count% - 1
done
if %has_tier% && (!%actor.companion% || %actor.companion.vnum% != %vnum%)
  %send% %actor% There was an error updating your shard. Please report this as a bug (1).
  set error 1
end
*
* Validate
if !%error%
  if %has_tier% > %tier%
    %send% %actor% You already have a higher level Celestial Forge companion!
    set error 1
  elseif %has_tier% == %tier% && %actor.companion.var(%upgrade%,0)%
    %send% %actor% You already have that upgrade.
    set error 1
  end
end
*
* check new companion
if !%error% && %tier% > %has_tier%
  nop %actor.remove_companion(%has_vnum%)%
  nop %actor.add_companion(%new_vnum%)%
  %mod% %actor% companion %new_vnum%
  if !%actor.companion% || %actor.companion.vnum% != %new_vnum%
    %send% %actor% There was an error updating your shard. Please report this as a bug (2).
    set error 1
  end
end
*
* upgrades
if !%error%
  set %upgrade% 1
  remote %upgrade% %actor.companion.id%
  attach 12837 %actor.companion.id%
end
*
* Refund?
if %error%
  eval genv 5100 + %tier% - 1
  nop %actor.give_currency(%genv%,%cost%)%
  eval curname %%currency.%genv%(%cost%)%%
  %send% %actor% You receive a refund of %cost% %curname%.
end
*
* And purge
%purge% %self%
~
#12835
Celestial Forge: Shard companion load script~
0 nt 100 0
~
%echo% ~%self% assembles itself and whirs to life!
~
#12836
Celestial Forge: Shard companion dies~
0 ft 100 7
L b 12834
L b 12844
L w 5100
L w 5101
L w 5102
L w 5103
L w 5104
~
set actor %self.companion%
if !%actor% || %actor.is_npc%
  halt
end
*
* check for refund?
switch %self.vnum%
  case 12834
    set tier 1
  break
  case 12844
    set tier 2
  break
  default
    nop %actor.remove_companion(%self.vnum%)%
    halt
  break
done
eval genv 5100 + %tier% - 1
eval cost %random.100%
nop %actor.give_currency(%genv%,%cost%)%
eval curname %%currency.%genv%(%cost%)%%
%send% %actor% You scavenge %cost% %curname% as ~%self% falls apart.
*
* and delete me
nop %actor.remove_companion(%self.vnum%)%
~
#12837
Celestial Forge: Companion renamer~
0 bt 100 2
L b 12834
L b 12844
~
set order 1
*
* upgrade portion
set tank %self.var(tank,0)%
set dps %self.var(dps,0)%
set caster %self.var(caster,0)%
if %tank%
  nop %self.add_mob_flag(CHAMPION)%
  nop %self.add_mob_flag(TANK)%
end
if %dps%
  nop %self.remove_mob_flag(NO-ATTACK)%
end
if %caster%
  nop %self.add_mob_flag(CASTER)%
end
if %caster% && %dps%
  nop %self.add_mob_flag(DPS)%
end
*
* determine names
if %tank% && %dps% && %caster%
  set name maelstone
  set pose whirls chaotically around you
  set order 2
elseif %tank% && %dps%
  set name shrapnel
  set pose bristles and creaks before you
  set order 2
elseif %tank% && %caster%
  set name bulwark
  set pose stands immovable before you
  set order 2
elseif %dps% && %caster%
  set name stormcoil
  set pose crackles and sparks above you
  set order 2
elseif %tank%
  set name plated
  set pose towers grimly over you
elseif %dps%
  set name jagged
  set pose gnashes and roils before you
elseif %caster%
  set name magnetic
  set pose whirls in the air above you
else
  set name rough
  set pose is here
end
*
* type portion
switch %self.vnum%
  case 12834
    set metal iron
  break
  case 12844
    set metal imperium
  break
  default
    set metal tin
  break
done
*
* actual naming
%mod% %self% keyword elemental %name% %metal%
if %order% == 1
  %mod% %self% shortdesc %name.ana% %name% %metal% elemental
  %mod% %self% longdesc &Z%name.ana% %name% %metal% elemental %pose%.
elseif %order% == 2
  %mod% %self% shortdesc %metal.ana% %metal% %name% elemental
  %mod% %self% longdesc &Z%metal.ana% %metal% %name% elemental %pose%.
end
*
* and detach
%echo% ~%self% clacks and clangs as it upgrades itself.
detach 12837 %self.id%
~
#12838
Celestial Forge: Set up training dummy with use~
1 c 6 1
L b 12838
use~
* List of dummies to exclude here
set dummy_list 12838
*
if %actor.obj_target(%arg.argument1%)% != %self%
  return 0
  halt
end
* validate
set room %actor.room%
if %actor.fighting% || %actor.disabled%
  %send% %actor% You're a little busy for that right now.
  halt
elseif %actor.position% != Standing
  %send% %actor% You need to be standing to set up @%self%.
  halt
elseif !%actor.canuseroom_guest(%room%)%
  %send% %actor% You don't have permission to set up training dummies here.
  halt
end
* check other dummies
set ch %room.people%
while %ch%
  if %ch.is_npc% && %dummy_list% ~= %ch.vnum%
    %send% %actor% There is already ~%ch% set up here.
    halt
  end
  set ch %ch.next_in_room%
done
* ok!
%load% mob %self.val0%
set mob %room.people%
if %mob.vnum% == %self.val0%
  %send% %actor% You set up ~%mob%!
  %echoaround% %actor% ~%actor% sets up ~%mob%!
  set time %timestamp%
  remote time %mob.id%
else
  %send% %actor% It looks like the dummy failed when you try to set it up. Report this as a bug.
end
%purge% %self%
~
#12839
Celestial Forge: Expire training dummy~
0 b 2 0
~
if %self.fighting%
  halt
end
set ch %self.room.people%
while %ch%
  if %ch.is_pc%
    halt
  end
  set ch %ch.next_in_room%
done
if (%timestamp% - %self.var(time,0)%) > 64800
  %echo% ~%self% breaks down and collapses in a heap.
  %purge% %self%
  halt
end
~
#12840
Celestial Forge: Mobs can't start combat with dummies~
0 B 0 0
~
if %actor.is_npc% && !%self.fighting%
  %send% %actor% Mobs can't start combat against this dummy.
  return 0
  halt
else
  return 1
end
~
$
