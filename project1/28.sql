select name, avglevel from ((select owner_id, avg(level) as avglevel from ((select id, type from Pokemon where type in ('Normal', 'Electric')) as t1 inner join (select pid, owner_id, level from CatchedPokemon) as  t2 on t1.id = t2.pid) group by owner_id) as t3 inner join (select id, name from Trainer) as t4 on t3.owner_id = t4.id) order by avglevel asc;
