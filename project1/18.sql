select avg(level) from ((select leader_id from Gym) as t1 inner join (select * from CatchedPokemon) as t2 on t1.leader_id = t2.owner_id);
