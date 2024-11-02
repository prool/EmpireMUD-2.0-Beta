#12102
Master Baker's Trials~
0 e 0
finishes cooking~
if !%actor.on_quest(12102)% || !%object%
  halt
end
set want_vnums 3357 3358 3359 3360 3361
set num_wanted 5
if %want_vnums% ~= %object.vnum%
  * mark that we got this one
  if !%actor.var(12102_made_%object.vnum%)%
    set 12102_made_%object.vnum% 1
    remote 12102_made_%object.vnum% %actor.id%
    say Ah, a perfect example of %object.shortdesc%! Nicely done.
    * triggers once per feast
    %quest% %actor% trigger 12102
  end
  * check if we got 'em all
  set missing 0
  set list %want_vnums%
  while %list%
    set vnum %list.car%
    set list %list.cdr%
    if !%actor.var(12102_made_%vnum%)%
      eval missing %missing% + 1
    end
  done
  * double-check trigger count
  while %actor.quest_triggered(12102)% < (%num_wanted% - %missing%)
    %quest% %actor% trigger 12102
  done
  while %actor.quest_triggered(12102)% > (%num_wanted% - %missing%)
    %quest% %actor% untrigger 12102
  done
  * did we finish the quest?
  if %actor.quest_triggered(12102)% >= %num_wanted%
    %send% %actor% You've finished all the feasts for the quest!
    * and delete the vars
    set list %want_vnums%
    while %list%
      set vnum %list.car%
      set list %list.cdr%
      rdelete 12102_made_%vnum% %actor.id%
    done
    wait 1
    %quest% %actor% finish 12102
  end
end
~
#12103
Divinely Tasty: Require milk then teach molds~
2 v 0
~
* script to check for milk
set milk_vnum 5
set milk_needed 6
*
set obj %actor.inventory%
set found 0
set low 0
while %obj% && !%found%
  if %obj.type% == DRINKCON && %obj.val2% == %milk_vnum%
    if %obj.val1% >= %milk_needed%
      set found %obj%
    else
      set low 1
    end
  end
  set obj %obj.next_in_list%
done
* ok?
if !%found%
  if %low%
    %send% %actor% You don't have enough milk to complete Divinely Tasty.
  else
    %send% %actor% You need some milk to complete the Divinely Tasty quest.
  end
  * block
  return 0
  halt
end
* otherwise, we are ok
eval amt %found.val1% - %milk_needed%
nop %found.val1(%amt%)%
%send% %actor% You hand over some milk...
nop %actor.add_learned(12142)%
return 1
~
#12141
Learn Imperial Chef recipes from book~
1 c 2
learn~
* Usage: learn <self>
* check targeting
if %actor.obj_target(%arg%)% != %self%
  return 0
  halt
end
*
* configs
set vnum_list 12100 12101 12103 12105 12106 12108 12140
set abil_list 293 270 271 271 271 271 293
set name_list fried porkchop, mom's pecan pie, venison stew pot, chili pot, chicken pot pie, cookoff-winning gumbo, and royal feast
*
* check if the player needs any of them
set any 0
set error 0
set vnum_copy %vnum_list%
while %vnum_copy%
  * pluck first numbers out
  set vnum %vnum_copy.car%
  set vnum_copy %vnum_copy.cdr%
  set abil %abil_list.car%
  set abil_list %abil_list.cdr%
  if !%actor.learned(%vnum%)%
    if %actor.ability(%abil%)%
      set any 1
    else
      set error 1
    end
  end
done
* how'd we do?
if %error% && !%any%
  %send% %actor% You don't have the right ability to learn anything new from @%self%.
  halt
elseif !%any%
  %send% %actor% There aren't any recipes you can learn from @%self%.
  halt
end
* ok go ahead and teach them
while %vnum_list%
  * pluck first numbers out
  set vnum %vnum_list.car%
  set vnum_list %vnum_list.cdr%
  if !%actor.learned(%vnum%)%
    nop %actor.add_learned(%vnum%)%
  end
done
* and report...
%send% %actor% You study @%self% and learn: %name_list%
%echoaround% %actor% ~%actor% studies @%self%.
%purge% %self%
~
#12142
a set of wooden molds~
1 c 6
mold~
set target %arg.car%
set shape %arg.cdr%
set item %actor.obj_target(%target%)%
if !%item%
  %send% %actor% You don't seem to have a '%target%'.
  halt
end
if %item.vnum% != 12129 && %item.vnum% != 12131
  %send% %actor% You can't mold @%item%.
  halt
end
makeuid empire empire %shape%
if %empire%
  set empire_name %empire.name%
else
end
if %shape% == dove
  %mod% %item% shortdesc a chocolate dove
  %mod% %item% keywords dove chocolate
  %mod% %item% longdesc A chocolate dove has been dropped here.
  %mod% %item% append-lookdesc A solid piece of chocolate has been shaped in the likeness of a dove.
elseif %shape% == bunny
  %mod% %item% keywords bunny chocolate
  %mod% %item% shortdesc a chocolate bunny
  %mod% %item% longdesc An entire chocolate bunny has been dropped here.
  %mod% %item% append-lookdesc A solid piece of chocolate has been molded into the likeness of a bunny.
elseif %shape% == fish
  %mod% %item% keywords fish chocolate
  %mod% %item% shortdesc a chocolate fish
  %mod% %item% longdesc A chocolate fish lies here.
  %mod% %item% append-lookdesc Solid chocolate has been molded into the shape of a fish.
elseif %shape% == wolf
  %mod% %item% keywords wolf chocolate
  %mod% %item% shortdesc a chocolate wolf
  %mod% %item% longdesc A chocolate wolf is lying here, tongue lolling and tail curved as if in mid wag.
  %mod% %item% append-lookdesc This chocolate wolf has been molded from a solid block of chocolate to look like it's been playing and is happily exhausted.
elseif %shape% == bear
  %mod% %item% keywords bear chocolate
  %mod% %item% shortdesc a chocolate bear
  %mod% %item% longdesc A chocolate bear has been left here.
  %mod% %item% append-lookdesc A solid chocolate bear is formed to look as if it's snarling and daring you to eat it.
elseif %shape% == deer
  %mod% %item% keywords deer chocolate
  %mod% %item% shortdesc a chocolate deer
  %mod% %item% longdesc A chocolate deer has been left here.
  %mod% %item% append-lookdesc A chocolate deer has been molded to look like it's grazing on some leaves.
elseif %shape% == sheep
  %mod% %item% keywords sheep chocolate
  %mod% %item% shortdesc a chocolate sheep
  %mod% %item% longdesc A chocolate sheep is lying here.
  %mod% %item% append-lookdesc A solid piece of chocolate has been molded to look like a sheep.
elseif %shape% == goat
  %mod% %item% keywords goat chocolate
  %mod% %item% shortdesc a chocolate goat
  %mod% %item% longdesc A chocolate goat has been dropped here.
  %mod% %item% append-lookdesc A solid piece of chocolate has been molded to resemble a goat.
elseif %shape% == fox
  %mod% %item% shortdesc a chocolate fox
  %mod% %item% keywords fox chocolate
  %mod% %item% longdesc A chocolate fox has been dropped here.
  %mod% %item% append-lookdesc A block of chocolate has been molded to look like a fox.
elseif %shape% == bull
  %mod% %item% keywords bull chocolate
  %mod% %item% shortdesc a chocolate bull
  %mod% %item% longdesc A chocolate bull looks ready to charge.
  %mod% %item% append-lookdesc This bull has been molded from a solid piece of choelseif %shape% == jaguar
  %mod% %item% shortdesc a chocolate jaguar
  %mod% %item% keywords jaguar chocolate
  %mod% %item% longdesc A chocolate jaguar looks poised to pounce.
  %mod% %item% append-lookdesc This block of chocolate has been molded to resemble a jaguar in mid pounce, claws out and ready to shred flesh.
elseif %shape% === badger
  %mod% %item% shortdesc a chocolate badger
  %mod% %item% keywords badger chocolate
  %mod% %item% longdesc A chocolate badger looks to be industriously digging.
  %mod% %item% append-lookdesc A block of chocolate has been molded to resemble a digging badger,.
elseif %shape% == sorcerer
  %mod% %item% shortdesc a chocolate sorcerer
  %mod% %item% keywords sorcerer chocolate
  %mod% %item% longdesc A chocolate sorcerer stands here leaning on his staff.
  %mod% %item% append-lookdesc A block of chocolate has been molded to resemble a robed sorcerer bearing a contemplative expression and leaning on a stout staff.
elseif %shape% == longship
  %mod% %item% shortdesc a chocolate longship
  %mod% %item% keywords longship chocolate
  %mod% %item% longdesc A chocolate longship is here.
  %mod% %item% append-lookdesc This block of chocolate is molded to look like a ship of war at full sail, its cannon raised to fire.
elseif %shape% == hulk
  %mod% %item% shortdesc a chocolate hulk
  %mod% %item% keywords hulk chocolate
  %mod% %item% longdesc A chocolate hulk is here.
  %mod% %item% append-lookdesc This block of chocolate resembles a hulk at full sail, with a hint of secured parcels and crates visible in its hold if you look through the top.
elseif %empire%
  %mod% %item% shortdesc a chocolate map of %empire_name%
  %mod% %item% keywords map chocolate %empire_name%
  %mod% %item% longdesc A chocolate map of %empire_name% lies here.
  %mod% %item% append-lookdesc An intricate chocolate map of %empire_name% has been molded from this solid block of chocolate, with key locations and terrain features marked.
  set shape map of %empire_name%
else
  %send% %actor% You don't have a mold for that.
  halt
end
%send% %actor% You pour the chocolate into the mold and retrieve %shape.ana% %shape% when it cools.
%echoaround% %actor% ~%actor% molds %shape.ana% %shape%.
~
#12143
a set of halloween-themed wooden molds~
1 c 6
mold~
set target %arg.car%
set shape %arg.cdr%
set item %actor.obj_target(%target%)%
if !%item%
  %send% %actor% You don't seem to have a '%target%'.
  halt
end
if %item.vnum% != 12129 && %item.vnum% != 12131
  %send% %actor% You can't mold @%item%.
  halt
end
makeuid empire empire %shape%
if %empire%
  set empire_name %empire.name%
end
if %shape% == witch's hat
  %mod% %item% shortdesc a chocolate witch's hat
  %mod% %item% keywords hat witch witch's chocolate
  %mod% %item% longdesc A chocolate witch's hat stands here.
  %mod% %item% lookdesc A block of chocolate has been shaped into a witch's hat, held open as if it contained someone's head.
elseif %shape% == ghost
  %mod% %item% keywords ghost hollow chocolate
  %mod% %item% shortdesc a chocolate ghost
  %mod% %item% longdesc A chocolate ghost is here.
  %mod% %item% lookdesc A chocolate block has been hollowed out into the shape of a ghost.
elseif %shape% == banshee
  %mod% %item% keywords banshee chocolate
  %mod% %item% shortdesc a chocolate banshee
  %mod% %item% longdesc A chocolate banshee lies here.
  %mod% %item% lookdesc A block of chocolate has been hollowed out into the shape of a banshee with mouth open as if screaming.
elseif %shape% == werewolf
  %mod% %item% keywords werewolf chocolate
  %mod% %item% shortdesc a chocolate werewolf
  %mod% %item% longdesc A chocolate wolf is half transformed here, neither fully human nor wolf.
  %mod% %item% lookdesc This solid block of chocoate is molded into a muscular werewolf, caught between human and wolf form.
elseif %shape% == pumpkin
  %mod% %item% shortdesc a chocolate pumpkin
  %mod% %item% longdesc A chocolate pumpkin has been left here.
  %mod% %item% lookdesc A hollow chocolate pumpkin has been molded as if someone is just starting to carve it, with only the barest suggestion of a distorted face on one side.
elseif %shape% == skull
  %mod% %item% keywords skull chocolate
  %mod% %item% shortdesc a chocolate skull
  %mod% %item% longdesc A chocolate skull is lying here.
  %mod% %item% lookdesc A block of chocolate has been hollowed out and molded into the shape of a skull.
elseif %shape% == cat
  %mod% %item% keywords cat chocolate
  %mod% %item% shortdesc a chocolate cat
  %mod% %item% longdesc A chocolate cat has been dropped here.
  %mod% %item% lookdesc A block of chocolate has been hollowed out and molded into the shape of a cat.
elseif %shape% == spider
  %mod% %item% shortdesc a chocolate spider
  %mod% %item% keywords spider chocolate
  %mod% %item% longdesc A chocolate spider has been dropped here, its delicate web spread around it.
  %mod% %item% lookdesc A block of chocolate has been hollowed out into the shape of a spider with delicate webbing around it.
elseif %shape% == broom
  %mod% %item% keywords broom chocolate
  %mod% %item% shortdesc a chocolate broom
  %mod% %item% longdesc A chocolate broom is balanced on its bristles.
  %mod% %item% lookdesc This block of chocolate is molded to resemble a broom, its de
elseif %empire%
  %mod% %item% shortdesc a chocolate memorial to %empire_name%
  %mod% %item% keywords memorial chocolate %empire_name%
  %mod% %item% longdesc A chocolate memorial to %empire_name% lies here.
  %mod% %item% lookdesc An intricate map of %empire_name% has been molded from this solid block of chocolate, with locations of key cities marked by tombstones indicating mass death.
  set shape memorial to %empire_name%
else
  %send% %actor% You don't have a mold for that.
  halt
end
%send% %actor% You pour the chocolate into the mold and hear an eerie cackle in the air before %shape.ana% %shape% emerges, seemingly of its own accord.
%echoaround% %actor% ~%actor% pours some chocolate and %shape.ana% %shape% seems to pop briskly out of the mold all by itself.
~
$
